#include "CrowdEventsSubsystem.h"

#include "CrowdEvent.h"
#include "CrowdEventsLibrary.h"
#include "CrowdReactionComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

bool UCrowdEventsSubsystem::bDrawStates = true;

namespace
{
	const double UnitsToMeters = 0.01;

	// -1: decide SetRuleSet; 0: dimostrazione; 1: addestramento.
	TAutoConsoleVariable<int32> CVarTraining(
		TEXT("CrowdEvents.Training"),
		-1,
		TEXT("Regole di assegnazione degli stati: -1 come impostato dai Blueprint, 0 dimostrazione, 1 addestramento."));

	double DistanceM(const UCrowdReactionComponent& Reaction, const ACrowdEvent& Event)
	{
		return FVector::Dist2D(Reaction.GetOwner()->GetActorLocation(), Event.GetActorLocation()) * UnitsToMeters;
	}

	FColor StateColor(ECrowdReactionState State, ECrowdReactionRole Role)
	{
		switch (State)
		{
		case ECrowdReactionState::Avoid: return Role == ECrowdReactionRole::AfterFlight ? FColor(255, 140, 0) : FColor(255, 230, 0);
		case ECrowdReactionState::Linger: return Role == ECrowdReactionRole::Rescuer ? FColor(0, 200, 80) : FColor(40, 170, 255);
		case ECrowdReactionState::Flee: return FColor(230, 40, 40);
		default: return FColor(160, 160, 160);
		}
	}
}

bool UCrowdEventsSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Simulate e Play nell'editor sono mondi PIE.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UCrowdEventsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCrowdEventsSubsystem, STATGROUP_Tickables);
}

UCrowdEventsSubsystem* UCrowdEventsSubsystem::GetCrowdEventsSubsystem(const UObject* WorldContextObject)
{
	if (!GEngine)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UCrowdEventsSubsystem>() : nullptr;
}

ECrowdRuleSet UCrowdEventsSubsystem::GetRuleSet() const
{
	const int32 Override = CVarTraining.GetValueOnGameThread();
	if (Override >= 0)
	{
		return Override > 0 ? ECrowdRuleSet::Training : ECrowdRuleSet::Demonstration;
	}
	return RuleSet;
}

void UCrowdEventsSubsystem::RegisterEvent(ACrowdEvent* Event)
{
	if (Event && !Events.Contains(Event))
	{
		Event->EventId = NextEventId++;
		Events.Add(Event);
	}
}

void UCrowdEventsSubsystem::UnregisterEvent(ACrowdEvent* Event)
{
	Events.Remove(Event);
}

void UCrowdEventsSubsystem::RegisterReaction(UCrowdReactionComponent* Reaction)
{
	if (Reaction)
	{
		Reactions.AddUnique(Reaction);
	}
}

void UCrowdEventsSubsystem::UnregisterReaction(UCrowdReactionComponent* Reaction)
{
	Reactions.Remove(Reaction);
}

int32 UCrowdEventsSubsystem::CountInState(ECrowdReactionState State) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<UCrowdReactionComponent>& Weak : Reactions)
	{
		const UCrowdReactionComponent* Reaction = Weak.Get();
		if (Reaction && Reaction->bPerceivesEvents && Reaction->GetState() == State)
		{
			++Count;
		}
	}
	return Count;
}

int32 UCrowdEventsSubsystem::CountRescuers(const ACrowdEvent* Event) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<UCrowdReactionComponent>& Weak : Reactions)
	{
		const UCrowdReactionComponent* Reaction = Weak.Get();
		if (Reaction && Reaction->bPerceivesEvents && Reaction->GetRole() == ECrowdReactionRole::Rescuer && Reaction->GetEvent() == Event)
		{
			++Count;
		}
	}
	return Count;
}

int32 UCrowdEventsSubsystem::CountFleeingSeen(const UCrowdReactionComponent& Reaction) const
{
	const AActor* Owner = Reaction.GetOwner();
	const FVector Location = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const double RadiusUnits = Rules.ContagionRadiusM / UnitsToMeters;

	int32 Count = 0;
	for (const TWeakObjectPtr<UCrowdReactionComponent>& Weak : Reactions)
	{
		const UCrowdReactionComponent* Other = Weak.Get();
		if (!Other || Other == &Reaction || !Other->bPerceivesEvents || Other->GetState() != ECrowdReactionState::Flee || !Other->GetOwner())
		{
			continue;
		}
		const FVector ToOther = Other->GetOwner()->GetActorLocation() - Location;
		if (ToOther.SizeSquared2D() <= RadiusUnits * RadiusUnits && FVector::DotProduct(ToOther, Forward) >= 0.0)
		{
			++Count;
		}
	}
	return Count;
}

void UCrowdEventsSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Events.RemoveAll([](const TWeakObjectPtr<ACrowdEvent>& Weak) { return !Weak.IsValid(); });
	Reactions.RemoveAll([](const TWeakObjectPtr<UCrowdReactionComponent>& Weak) { return !Weak.IsValid(); });

	const bool bTraining = GetRuleSet() == ECrowdRuleSet::Training;
	for (const TWeakObjectPtr<UCrowdReactionComponent>& Weak : Reactions)
	{
		UCrowdReactionComponent* Reaction = Weak.Get();
		if (!Reaction || !Reaction->GetOwner())
		{
			continue;
		}
		if (!Reaction->bPerceivesEvents)
		{
			if (Reaction->GetState() != ECrowdReactionState::None)
			{
				Reaction->ClearReaction();
			}
			continue;
		}
		UpdateReaction(*Reaction, DeltaTime, bTraining);
	}

	if (bDrawStates)
	{
		DrawStates();
	}
}

void UCrowdEventsSubsystem::UpdateReaction(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining)
{
	// Evento finito o tolto: si torna allo stato 0 da qualunque stato.
	if (Reaction.State != ECrowdReactionState::None)
	{
		const ACrowdEvent* Current = Reaction.GetEvent();
		if (!Current || !Current->IsEventActive())
		{
			Reaction.ClearReaction();
		}
	}

	UpdateTransitions(Reaction, DeltaTimeS, bTraining);

	// Chi fugge o è appena fuggito non guarda altri eventi.
	const bool bBusyWithDanger = Reaction.State == ECrowdReactionState::Flee || Reaction.Role == ECrowdReactionRole::AfterFlight;
	if (!bBusyWithDanger)
	{
		LookForEvents(Reaction, DeltaTimeS, bTraining);
	}
}

void UCrowdEventsSubsystem::UpdateTransitions(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining)
{
	ACrowdEvent* Current = Reaction.GetEvent();
	if (Reaction.State == ECrowdReactionState::None || !Current)
	{
		return;
	}

	const FCrowdReactionSettings& Settings = Reaction.Settings;
	const double Distance = DistanceM(Reaction, *Current);

	switch (Reaction.State)
	{
	case ECrowdReactionState::Avoid:
		if (Reaction.Role == ECrowdReactionRole::AfterFlight)
		{
			// Troppo vicino al pericolo: si torna a fuggire.
			if (Distance < Settings.PanicRadiusM)
			{
				Reaction.SetReaction(ECrowdReactionState::Flee, ECrowdReactionRole::Normal, Current);
			}
		}
		else if (Distance > Current->Params.PerceptionRadiusM)
		{
			Reaction.ClearReaction();
		}
		break;

	case ECrowdReactionState::Linger:
	{
		Reaction.LingerElapsedS += DeltaTimeS;
		if (!Reaction.bArrived)
		{
			const bool bRescuer = Reaction.Role == ECrowdReactionRole::Rescuer;
			const double Gap = Distance - Reaction.GetLingerTargetRadiusM();
			// Chi soccorre è arrivato quando tocca; chi guarda quando è fermo sull'anello.
			const bool bInPlace = bRescuer ? Gap <= Settings.ArrivalToleranceM : FMath::Abs(Gap) < Settings.ArrivalToleranceM;
			if (bInPlace && Reaction.GetSpeedMS() < Settings.ArrivalSpeedMS)
			{
				Reaction.bArrived = true;
			}
			else if (Reaction.LingerElapsedS > Rules.MaxApproachS)
			{
				Reaction.SetReaction(ECrowdReactionState::Avoid, ECrowdReactionRole::Normal, Current);
			}
			break;
		}

		Reaction.LingerRemainingS -= DeltaTimeS;
		if (Reaction.LingerRemainingS <= 0.0)
		{
			if (Reaction.Role == ECrowdReactionRole::Rescuer)
			{
				// Finito il soccorso il malore si spegne: gli altri tornano allo stato 0 al prossimo giro.
				Current->EndEvent();
				Reaction.ClearReaction();
			}
			else
			{
				Reaction.SetReaction(ECrowdReactionState::Avoid, ECrowdReactionRole::Normal, Current);
			}
		}
		break;
	}

	case ECrowdReactionState::Flee:
		if (Distance >= Settings.SafeRadiusM)
		{
			Reaction.SetReaction(ECrowdReactionState::Avoid, ECrowdReactionRole::AfterFlight, Current);
			// Il premio si dà una volta per evento: altrimenti rientrare e riuscire converrebbe.
			if (!Reaction.SafeBonusGiven.Contains(Current))
			{
				Reaction.SafeBonusGiven.Add(Current);
				Reaction.bPendingSafeBonus = true;
			}
		}
		break;

	default:
		break;
	}
}

void UCrowdEventsSubsystem::LookForEvents(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining)
{
	const AActor* Owner = Reaction.GetOwner();
	const bool bFree = Reaction.State == ECrowdReactionState::None;

	for (const TWeakObjectPtr<ACrowdEvent>& Weak : Events)
	{
		ACrowdEvent* Event = Weak.Get();
		if (!Event || !Event->IsEventActive() || Reaction.HandledEvents.Contains(Event))
		{
			continue;
		}
		// Chi sta già reagendo a un evento lo lascia solo per un pericolo.
		if (!bFree && Event->Params.EventClass != ECrowdEventClass::Danger)
		{
			continue;
		}
		const double Distance = DistanceM(Reaction, *Event);
		if (Distance > Event->Params.PerceptionRadiusM)
		{
			continue;
		}

		if (bTraining)
		{
			// In addestramento l'awareness è ignorata; una quota resta nello stato 0.
			Reaction.HandledEvents.Add(Event);
			if (Random.GetFraction() < Rules.TrainingNeutralQuota)
			{
				continue;
			}
			AssignState(Reaction, *Event, true);
			return;
		}

		const FVector ToEvent = Event->GetActorLocation() - Owner->GetActorLocation();
		const bool bInFront = FVector::DotProduct(ToEvent, Owner->GetActorForwardVector()) >= 0.0;
		const int32 FleeingSeen = Event->Params.EventClass == ECrowdEventClass::Danger ? CountFleeingSeen(Reaction) : 0;
		const double Probability = UCrowdEventsLibrary::NoticeProbability(Reaction.Attention, Event->Params.Salience, Distance, bInFront,
			FleeingSeen, DeltaTimeS, Rules.PerceptionDecayM, Rules.ContagionWeight);
		if (Random.GetFraction() < Probability)
		{
			Reaction.HandledEvents.Add(Event);
			AssignState(Reaction, *Event, false);
			return;
		}
	}
}

void UCrowdEventsSubsystem::AssignState(UCrowdReactionComponent& Reaction, ACrowdEvent& Event, bool bTraining)
{
	const ECrowdEventClass EventClass = Event.Params.EventClass;
	if (EventClass == ECrowdEventClass::Danger)
	{
		Reaction.SetReaction(ECrowdReactionState::Flee, ECrowdReactionRole::Normal, &Event);
		return;
	}

	const bool bRescuePossible = EventClass == ECrowdEventClass::Illness && CountRescuers(&Event) < Rules.MaxRescuers;

	if (bTraining)
	{
		// Uno a caso fra gli stati ammessi dalla classe (specifica §9.3).
		const int32 Choice = Random.RandRange(0, bRescuePossible ? 2 : 1);
		if (Choice == 0)
		{
			Reaction.SetReaction(ECrowdReactionState::Avoid, ECrowdReactionRole::Normal, &Event);
		}
		else
		{
			StartLinger(Reaction, Event, Choice == 2 ? ECrowdReactionRole::Rescuer : ECrowdReactionRole::Spectator, true);
		}
		return;
	}

	// Regole di dimostrazione (specifica §5.3).
	if (bRescuePossible && Reaction.bWillingToHelp)
	{
		StartLinger(Reaction, Event, ECrowdReactionRole::Rescuer, false);
	}
	else if (Random.GetFraction() < Reaction.CuriosityProbability)
	{
		StartLinger(Reaction, Event, ECrowdReactionRole::Spectator, false);
	}
	else
	{
		Reaction.SetReaction(ECrowdReactionState::Avoid, ECrowdReactionRole::Normal, &Event);
	}
}

void UCrowdEventsSubsystem::StartLinger(UCrowdReactionComponent& Reaction, ACrowdEvent& Event, ECrowdReactionRole Role, bool bTraining)
{
	Reaction.SetReaction(ECrowdReactionState::Linger, Role, &Event);
	const FVector2D Range = bTraining ? Rules.TrainingLingerS
		: (Role == ECrowdReactionRole::Rescuer ? Rules.RescueLingerS : Rules.SpectatorLingerS);
	Reaction.LingerRemainingS = RandomInRange(Range.X, Range.Y);
}

void UCrowdEventsSubsystem::DrawStates() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (const TWeakObjectPtr<UCrowdReactionComponent>& Weak : Reactions)
	{
		const UCrowdReactionComponent* Reaction = Weak.Get();
		if (!Reaction || !Reaction->GetOwner() || Reaction->GetState() == ECrowdReactionState::None)
		{
			continue;
		}
		const FVector Location = Reaction->GetOwner()->GetActorLocation() + FVector(0.0, 0.0, 120.0);
		DrawDebugPoint(World, Location, 14.f, StateColor(Reaction->GetState(), Reaction->GetRole()), false, -1.f, 0);
	}
}
