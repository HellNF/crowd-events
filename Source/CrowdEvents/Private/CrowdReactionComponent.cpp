#include "CrowdReactionComponent.h"

#include "CrowdEvent.h"
#include "CrowdEventsLibrary.h"
#include "CrowdEventsSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	const double UnitsToMeters = 0.01;
}

UCrowdReactionComponent::UCrowdReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCrowdReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UCrowdEventsSubsystem* Subsystem = World->GetSubsystem<UCrowdEventsSubsystem>())
		{
			Subsystem->RegisterReaction(this);
		}
	}
}

void UCrowdReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UCrowdEventsSubsystem* Subsystem = World->GetSubsystem<UCrowdEventsSubsystem>())
		{
			Subsystem->UnregisterReaction(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UCrowdReactionComponent::ResetReaction(double BaseDesiredSpeedMS)
{
	BaseSpeedMS = BaseDesiredSpeedMS > 0.0 ? BaseDesiredSpeedMS : 1.5;
	// Velocità di panico uniforme in 2,5-3 m/s (specifica §5.5).
	PanicSpeedMS = 2.75;
	if (UWorld* World = GetWorld())
	{
		if (UCrowdEventsSubsystem* Subsystem = World->GetSubsystem<UCrowdEventsSubsystem>())
		{
			PanicSpeedMS = Subsystem->RandomInRange(2.5, 3.0);
		}
	}
	HandledEvents.Reset();
	SafeBonusGiven.Reset();
	bPendingSafeBonus = false;
	ClearReaction();
}

void UCrowdReactionComponent::SetReaction(ECrowdReactionState NewState, ECrowdReactionRole NewRole, ACrowdEvent* NewEvent)
{
	State = NewState;
	Role = NewRole;
	Event = NewEvent;
	LingerRemainingS = 0.0;
	LingerElapsedS = 0.0;
	bArrived = false;
	// Il termine di fuga confronta due distanze dallo stesso evento.
	PreviousDistanceM = -1.0;
}

void UCrowdReactionComponent::ClearReaction()
{
	SetReaction(ECrowdReactionState::None, ECrowdReactionRole::Normal, nullptr);
}

ACrowdEvent* UCrowdReactionComponent::GetEvent() const
{
	return Event.Get();
}

int32 UCrowdReactionComponent::GetEventId() const
{
	const ACrowdEvent* Current = Event.Get();
	return Current ? Current->GetEventId() : -1;
}

double UCrowdReactionComponent::GetDistanceToEventM() const
{
	const ACrowdEvent* Current = Event.Get();
	const AActor* Owner = GetOwner();
	if (!Current || !Owner)
	{
		return -1.0;
	}
	return FVector::Dist2D(Owner->GetActorLocation(), Current->GetActorLocation()) * UnitsToMeters;
}

double UCrowdReactionComponent::GetEffectiveBeta() const
{
	return Role == ECrowdReactionRole::Rescuer ? 0.0 : Beta;
}

double UCrowdReactionComponent::GetSpeedMS() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetVelocity().Size2D() * UnitsToMeters : 0.0;
}

double UCrowdReactionComponent::GetLingerTargetRadiusM() const
{
	const ACrowdEvent* Current = Event.Get();
	if (!Current || Role == ECrowdReactionRole::Rescuer)
	{
		return Settings.ContactDistanceM;
	}
	return UCrowdEventsLibrary::RingRadius(Current->Params.Field, Alpha, Beta, Settings.ContactDistanceM);
}

TArray<float> UCrowdReactionComponent::GetEventObservations() const
{
	TArray<float> Observations;
	Observations.Init(0.f, 12);

	const ACrowdEvent* Current = Event.Get();
	const AActor* Owner = GetOwner();
	const bool bReacting = State != ECrowdReactionState::None && Current && Owner;

	if (bReacting || !Settings.bNeutralStateAsZeros)
	{
		Observations[static_cast<int32>(State)] = 1.f;
	}
	if (!bReacting)
	{
		return Observations;
	}

	Observations[4] = Role == ECrowdReactionRole::Rescuer ? 1.f : 0.f;
	Observations[5] = 1.f;
	Observations[6 + static_cast<int32>(Current->Params.EventClass)] = 1.f;

	const double PerceptionRadiusM = FMath::Max(Current->Params.PerceptionRadiusM, 0.01);
	const FVector ToEvent = (Current->GetActorLocation() - Owner->GetActorLocation()) * UnitsToMeters;
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = Owner->GetActorRightVector().GetSafeNormal2D();
	// Dopo una fuga l'evento può essere oltre il raggio di percezione: il valore si ferma a ±2.
	Observations[9] = static_cast<float>(FMath::Clamp(FVector::DotProduct(ToEvent, Forward) / PerceptionRadiusM, -2.0, 2.0));
	Observations[10] = static_cast<float>(FMath::Clamp(FVector::DotProduct(ToEvent, Right) / PerceptionRadiusM, -2.0, 2.0));

	const double ReferenceM = UCrowdEventsLibrary::ReferenceRadius(State, Role, Current->Params.Field, Alpha, GetEffectiveBeta(),
		Settings.SafeRadiusM, Settings.ContactDistanceM, Settings.RingWidthM);
	if (ReferenceM >= 0.0)
	{
		Observations[11] = static_cast<float>(FMath::Clamp((ToEvent.Size2D() - ReferenceM) / PerceptionRadiusM, -2.0, 2.0));
	}
	return Observations;
}

FCrowdRewardMultipliers UCrowdReactionComponent::GetRewardMultipliers() const
{
	return UCrowdEventsLibrary::RewardMultipliers(State, Role, Settings.PanicK);
}

double UCrowdReactionComponent::GetEventReward()
{
	const UWorld* World = GetWorld();
	const double NowS = World ? World->GetTimeSeconds() : 0.0;
	if (LastRewardTimeS >= 0.0 && FMath::IsNearlyEqual(NowS, LastRewardTimeS, 1e-6))
	{
		return LastReward;
	}
	const double DeltaTimeS = LastRewardTimeS >= 0.0 ? NowS - LastRewardTimeS : 0.0;
	LastRewardTimeS = NowS;

	double Reward = 0.0;
	if (bPendingSafeBonus)
	{
		Reward += Settings.SafeBonus;
		bPendingSafeBonus = false;
	}

	const ACrowdEvent* Current = Event.Get();
	const AActor* Owner = GetOwner();
	if (State == ECrowdReactionState::None || !Current || !Owner)
	{
		PreviousDistanceM = -1.0;
		LastReward = Reward;
		return Reward;
	}

	const FCrowdEventParams& Params = Current->Params;
	const double DistanceM = GetDistanceToEventM();
	const double EffectiveBeta = GetEffectiveBeta();

	switch (State)
	{
	case ECrowdReactionState::Avoid:
		if (Role == ECrowdReactionRole::AfterFlight)
		{
			Reward += Settings.EventWeight * Settings.SafeWallWeight * UCrowdEventsLibrary::SafeWallReward(DistanceM, Settings.SafeRadiusM);
		}
		else
		{
			const double BubbleM = UCrowdEventsLibrary::BubbleRadius(Params.Field, Settings.ContactDistanceM, Settings.RingWidthM);
			Reward += Settings.EventWeight * UCrowdEventsLibrary::AvoidReward(Beta, DistanceM, BubbleM, Params.Field.RepulsiveDecayM);
		}
		break;

	case ECrowdReactionState::Linger:
	{
		const double TargetM = GetLingerTargetRadiusM();
		Reward += Settings.EventWeight * UCrowdEventsLibrary::LingerReward(Params.Field, Alpha, EffectiveBeta, DistanceM, TargetM,
			Params.PerceptionRadiusM, Settings.ContactDistanceM);

		const FVector ToEvent = (Current->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		Reward += Settings.LookWeight * FVector::DotProduct(Owner->GetActorForwardVector().GetSafeNormal2D(), ToEvent);

		const double TargetSpeedMS = UCrowdEventsLibrary::LingerSpeedTarget(BaseSpeedMS, DistanceM, TargetM, Settings.LingerLengthM);
		Reward += Settings.DcrSpeedWeight * Settings.LingerSpeedMultiplier
			* UCrowdEventsLibrary::LingerSpeedReward(GetSpeedMS(), TargetSpeedMS, BaseSpeedMS);
		break;
	}

	case ECrowdReactionState::Flee:
		if (PreviousDistanceM >= 0.0)
		{
			Reward += Settings.EventWeight * UCrowdEventsLibrary::FleeReward(DistanceM, PreviousDistanceM, PanicSpeedMS, DeltaTimeS);
		}
		break;

	default:
		break;
	}

	PreviousDistanceM = DistanceM;
	LastReward = Reward;
	return Reward;
}

double UCrowdReactionComponent::GetSpeedTargetMS(double DcrDesiredSpeedMS) const
{
	if (State == ECrowdReactionState::Linger && Event.IsValid())
	{
		return UCrowdEventsLibrary::LingerSpeedTarget(BaseSpeedMS, GetDistanceToEventM(), GetLingerTargetRadiusM(), Settings.LingerLengthM);
	}
	if (State == ECrowdReactionState::Flee)
	{
		return PanicSpeedMS;
	}
	return DcrDesiredSpeedMS;
}

double UCrowdReactionComponent::GetMaxWalkSpeedCm(double DcrDesiredSpeedMS) const
{
	// DCR: MaxWalkSpeed = 200 × DesiredVelocity, al più 500 cm/s.
	if (State == ECrowdReactionState::Linger)
	{
		return FMath::Min(200.0 * BaseSpeedMS, 500.0);
	}
	if (State == ECrowdReactionState::Flee)
	{
		return 500.0;
	}
	return FMath::Min(200.0 * DcrDesiredSpeedMS, 500.0);
}

bool UCrowdReactionComponent::CountsInNeighbourAverage() const
{
	return State == ECrowdReactionState::None || State == ECrowdReactionState::Avoid;
}

UCrowdReactionComponent* UCrowdReactionComponent::GetReactionComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UCrowdReactionComponent>() : nullptr;
}
