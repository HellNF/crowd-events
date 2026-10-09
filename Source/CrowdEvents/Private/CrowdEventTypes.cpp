#include "CrowdEventTypes.h"

#define LOCTEXT_NAMESPACE "CrowdEvents"

namespace
{
	FCrowdEventPresetData MakePreset(const FText& Name, ECrowdEventClass EventClass, double AttractiveIntensity, double RepulsiveIntensity,
		double RepulsiveDecayM, double PerceptionRadiusM, double Salience, double DurationS)
	{
		FCrowdEventPresetData Preset;
		Preset.DisplayName = Name;
		Preset.Params.EventClass = EventClass;
		Preset.Params.Field.AttractiveIntensity = AttractiveIntensity;
		Preset.Params.Field.RepulsiveIntensity = RepulsiveIntensity;
		Preset.Params.Field.RepulsiveDecayM = RepulsiveDecayM;
		Preset.Params.Field.AttractiveDecayM = 4.0;
		Preset.Params.PerceptionRadiusM = PerceptionRadiusM;
		Preset.Params.Salience = Salience;
		Preset.Params.DurationS = DurationS;
		return Preset;
	}
}

const TArray<FCrowdEventPresetData>& CrowdEvents::GetBuiltInPresets()
{
	// Valori della specifica §3.2; le intensità repulsive 0,77 e 5,0 danno anelli di 1,5 e 4 m.
	static const TArray<FCrowdEventPresetData> Presets = {
		MakePreset(LOCTEXT("PresetStreetPerformer", "Street performer"), ECrowdEventClass::Attractive, 1.0, 0.77, 1.0, 10.0, 1.0, 60.0),
		MakePreset(LOCTEXT("PresetBrawl", "Brawl"), ECrowdEventClass::Attractive, 1.0, 5.0, 1.0, 10.0, 1.0, 60.0),
		MakePreset(LOCTEXT("PresetFallenPerson", "Fallen person"), ECrowdEventClass::Illness, 1.0, 0.77, 1.0, 8.0, 0.5, 45.0),
		MakePreset(LOCTEXT("PresetFire", "Fire"), ECrowdEventClass::Danger, 0.0, 1.0, 3.0, 15.0, 2.0, 30.0),
		MakePreset(LOCTEXT("PresetArmedPerson", "Armed person"), ECrowdEventClass::Danger, 0.0, 1.0, 3.0, 15.0, 2.0, 30.0)
	};
	return Presets;
}

void CrowdEvents::GetTrainingRange(ECrowdEventParameter Parameter, double& OutMin, double& OutMax)
{
	switch (Parameter)
	{
	case ECrowdEventParameter::AttractiveIntensity:
		// Il pericolo non ha componente attrattiva: il minimo è 0.
		OutMin = 0.0; OutMax = 2.0; break;
	case ECrowdEventParameter::RepulsiveIntensity:
		OutMin = 0.385; OutMax = 10.0; break;
	case ECrowdEventParameter::RepulsiveDecayM:
		OutMin = 0.5; OutMax = 6.0; break;
	case ECrowdEventParameter::AttractiveDecayM:
		OutMin = 2.0; OutMax = 8.0; break;
	case ECrowdEventParameter::PerceptionRadiusM:
		OutMin = 4.0; OutMax = 30.0; break;
	case ECrowdEventParameter::Salience:
		OutMin = 0.25; OutMax = 4.0; break;
	case ECrowdEventParameter::DurationS:
		// Zero vuol dire «senza fine», quindi resta ammesso.
		OutMin = 0.0; OutMax = 120.0; break;
	default:
		OutMin = 0.0; OutMax = 1.0; break;
	}
}

#undef LOCTEXT_NAMESPACE
