#include "CrowdEventsLibrary.h"
#include "CrowdEventTypes.h"

FString UCrowdEventsLibrary::GetCrowdEventsVersion()
{
	return TEXT("0.3.6");
}

double UCrowdEventsLibrary::PerceivedField(const FCrowdEventField& Field, double Alpha, double Beta, double DistanceM)
{
	double Value = 0.0;

	if (Field.RepulsiveDecayM > 0.0)
	{
		Value += Beta * Field.RepulsiveIntensity * FMath::Exp(-DistanceM / Field.RepulsiveDecayM);
	}
	if (Field.AttractiveDecayM > 0.0)
	{
		Value -= Alpha * Field.AttractiveIntensity * FMath::Exp(-DistanceM / Field.AttractiveDecayM);
	}

	return Value;
}

double UCrowdEventsLibrary::RingRadius(const FCrowdEventField& Field, double Alpha, double Beta, double ContactDistanceM)
{
	const double Attraction = Alpha * Field.AttractiveIntensity;
	const double Repulsion = Beta * Field.RepulsiveIntensity;

	if (Attraction <= 0.0 || Repulsion <= 0.0 || Field.RepulsiveDecayM <= 0.0 || Field.AttractiveDecayM <= Field.RepulsiveDecayM)
	{
		return ContactDistanceM;
	}

	const double Argument = (Repulsion * Field.AttractiveDecayM) / (Attraction * Field.RepulsiveDecayM);
	if (Argument <= 1.0)
	{
		return ContactDistanceM;
	}

	return (Field.RepulsiveDecayM * Field.AttractiveDecayM) / (Field.AttractiveDecayM - Field.RepulsiveDecayM) * FMath::Loge(Argument);
}

namespace
{
	bool HasRingShape(const FCrowdEventField& Field)
	{
		return Field.AttractiveIntensity > 0.0 && Field.RepulsiveDecayM > 0.0 && Field.AttractiveDecayM > Field.RepulsiveDecayM;
	}

	double RandomInRange(const FRandomStream& Stream, double Min, double Max)
	{
		return Min + (Max - Min) * static_cast<double>(Stream.GetFraction());
	}
}

double UCrowdEventsLibrary::RepulsiveIntensityForRing(const FCrowdEventField& Field, double RingRadiusM)
{
	if (!HasRingShape(Field))
	{
		return Field.RepulsiveIntensity;
	}
	// Da RingRadius: anello = k ln(Brep λatt / (Batt λrep)), con k = λrep λatt / (λatt − λrep).
	const double K = (Field.RepulsiveDecayM * Field.AttractiveDecayM) / (Field.AttractiveDecayM - Field.RepulsiveDecayM);
	return Field.AttractiveIntensity * Field.RepulsiveDecayM / Field.AttractiveDecayM * FMath::Exp(RingRadiusM / K);
}

FCrowdEventField UCrowdEventsLibrary::ClampFieldRing(const FCrowdEventField& Field, double MinRingM, double MaxRingM)
{
	FCrowdEventField Result = Field;
	if (!HasRingShape(Field))
	{
		return Result;
	}
	// Repulsione minima e massima che danno anelli agli estremi: l'anello cresce con la repulsione.
	const double MinRepulsion = RepulsiveIntensityForRing(Field, MinRingM);
	const double MaxRepulsion = RepulsiveIntensityForRing(Field, MaxRingM);
	Result.RepulsiveIntensity = FMath::Clamp(Field.RepulsiveIntensity, MinRepulsion, MaxRepulsion);
	return Result;
}

FCrowdEventField UCrowdEventsLibrary::SampleRingField(const FRandomStream& Stream, double MinRingM, double MaxRingM)
{
	double RepulsionMin = 0.0;
	double RepulsionMax = 0.0;
	CrowdEvents::GetTrainingRange(ECrowdEventParameter::RepulsiveIntensity, RepulsionMin, RepulsionMax);

	const double RingM = RandomInRange(Stream, MinRingM, MaxRingM);
	FCrowdEventField Field;
	for (int32 Attempt = 0; Attempt < 100; ++Attempt)
	{
		Field.AttractiveIntensity = RandomInRange(Stream, 0.5, 2.0);
		Field.RepulsiveDecayM = RandomInRange(Stream, 0.5, 2.0);
		Field.AttractiveDecayM = RandomInRange(Stream, FMath::Max(2.0, Field.RepulsiveDecayM + 1.0), 8.0);
		Field.RepulsiveIntensity = RepulsiveIntensityForRing(Field, RingM);
		if (Field.RepulsiveIntensity >= RepulsionMin && Field.RepulsiveIntensity <= RepulsionMax)
		{
			break;
		}
	}
	// Dopo 100 tentativi (non succede con questi intervalli) resta l'ultimo campo: l'anello è comunque RingM.
	return Field;
}

double UCrowdEventsLibrary::BubbleRadius(const FCrowdEventField& Field, double ContactDistanceM, double RingWidthM)
{
	return RingRadius(Field, 1.0, 1.0, ContactDistanceM) + RingWidthM;
}

double UCrowdEventsLibrary::ReferenceRadius(ECrowdReactionState State, ECrowdReactionRole Role, const FCrowdEventField& Field, double Alpha,
	double Beta, double SafeRadiusM, double ContactDistanceM, double RingWidthM)
{
	if (State == ECrowdReactionState::Avoid)
	{
		return Role == ECrowdReactionRole::AfterFlight ? SafeRadiusM : BubbleRadius(Field, ContactDistanceM, RingWidthM);
	}
	if (State == ECrowdReactionState::Linger)
	{
		return Role == ECrowdReactionRole::Rescuer ? ContactDistanceM : RingRadius(Field, Alpha, Beta, ContactDistanceM);
	}
	return -1.0;
}

double UCrowdEventsLibrary::AvoidReward(double Beta, double DistanceM, double BubbleRadiusM, double RepulsiveDecayM)
{
	if (RepulsiveDecayM <= 0.0)
	{
		return 0.0;
	}
	return -Beta * FMath::Min(3.0, FMath::Exp(-(DistanceM - BubbleRadiusM) / RepulsiveDecayM));
}

double UCrowdEventsLibrary::SafeWallReward(double DistanceM, double SafeRadiusM)
{
	if (SafeRadiusM <= 0.0)
	{
		return 0.0;
	}
	return -FMath::Max(0.0, (SafeRadiusM - DistanceM) / SafeRadiusM);
}

double UCrowdEventsLibrary::LingerReward(const FCrowdEventField& Field, double Alpha, double Beta, double DistanceM, double TargetRadiusM,
	double PerceptionRadiusM, double ContactDistanceM)
{
	const double Clamped = FMath::Max(DistanceM, ContactDistanceM);
	const double AtTarget = PerceivedField(Field, Alpha, Beta, TargetRadiusM);
	const double Denominator = PerceivedField(Field, Alpha, Beta, PerceptionRadiusM) - AtTarget;
	if (Denominator <= 1e-9)
	{
		return 0.0;
	}
	return FMath::Clamp(-(PerceivedField(Field, Alpha, Beta, Clamped) - AtTarget) / Denominator, -1.0, 0.0);
}

double UCrowdEventsLibrary::LingerSpeedTarget(double BaseSpeedMS, double DistanceM, double TargetRadiusM, double LengthM)
{
	if (LengthM <= 0.0)
	{
		return 0.0;
	}
	return BaseSpeedMS * FMath::Tanh(FMath::Max(0.0, DistanceM - TargetRadiusM) / LengthM);
}

double UCrowdEventsLibrary::LingerSpeedReward(double SpeedMS, double TargetSpeedMS, double BaseSpeedMS)
{
	if (BaseSpeedMS <= 0.0)
	{
		return 0.0;
	}
	return -FMath::Clamp(FMath::Square((SpeedMS - TargetSpeedMS) / BaseSpeedMS), 0.0, 1.0);
}

double UCrowdEventsLibrary::FleeReward(double DistanceM, double PreviousDistanceM, double PanicSpeedMS, double DeltaTimeS)
{
	if (PanicSpeedMS <= 0.0 || DeltaTimeS <= 0.0)
	{
		return 0.0;
	}
	return FMath::Clamp((DistanceM - PreviousDistanceM) / (PanicSpeedMS * DeltaTimeS), -1.0, 1.0);
}

double UCrowdEventsLibrary::NoticeProbability(double Attention, double Salience, double DistanceM, bool bInFront, int32 FleeingSeen,
	double DeltaTimeS, double PerceptionDecayM, double ContagionWeight)
{
	if (PerceptionDecayM <= 0.0)
	{
		return 0.0;
	}
	// Dietro al pedone l'evento si nota meno: 0,3 contro 1 nel ventaglio dei raggi.
	const double View = bInFront ? 1.0 : 0.3;
	const double Rate = Attention * Salience * FMath::Exp(-DistanceM / PerceptionDecayM) * View + ContagionWeight * FleeingSeen;
	return 1.0 - FMath::Exp(-Rate * DeltaTimeS);
}

FCrowdRewardMultipliers UCrowdEventsLibrary::RewardMultipliers(ECrowdReactionState State, ECrowdReactionRole Role, double PanicK)
{
	FCrowdRewardMultipliers Multipliers;
	if (State == ECrowdReactionState::Linger)
	{
		Multipliers.Goal = 0.0;
		Multipliers.Speed = 0.0;
		Multipliers.Spawn = 0.0;
	}
	else if (State == ECrowdReactionState::Flee)
	{
		Multipliers.Goal = 0.0;
		Multipliers.Social = 1.0 - PanicK;
		Multipliers.Spawn = 0.0;
		Multipliers.Smoothness = 1.0 - PanicK;
	}
	return Multipliers;
}
