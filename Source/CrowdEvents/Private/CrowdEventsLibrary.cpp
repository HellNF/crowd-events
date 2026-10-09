#include "CrowdEventsLibrary.h"

FString UCrowdEventsLibrary::GetCrowdEventsVersion()
{
	return TEXT("0.2.0");
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
