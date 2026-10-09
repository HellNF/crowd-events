#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CrowdEventsLibrary.h"
#include "CrowdEventTypes.generated.h"

/** Classi di evento (specifica della milestone 1, §3.1). */
UENUM(BlueprintType)
enum class ECrowdEventClass : uint8
{
	/** Artista di strada, colluttazione: sosta sull'anello. */
	Attractive,
	/** Anziano caduto: due soccorrono a contatto, altri guardano. */
	Illness,
	/** Persona armata, incendio: fuga. */
	Danger
};

/**
 * Parametri di un evento (specifica §3.2).
 * Le distanze sono in metri, le durate in secondi.
 */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdEventParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	ECrowdEventClass EventClass = ECrowdEventClass::Attractive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FCrowdEventField Field;

	/** Raggio entro cui un pedone può accorgersi dell'evento, in metri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double PerceptionRadiusM = 10.0;

	/** Salienza: quanto l'evento si fa notare (specifica §5.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double Salience = 1.0;

	/** Durata dell'evento, in secondi. Zero: l'evento non finisce da solo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double DurationS = 60.0;
};

/** Un evento già configurato, con il nome mostrato nel pannello. */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdEventPresetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FCrowdEventParams Params;
};

/** Preset salvato nel progetto: si crea e si duplica dal Content Browser. */
UCLASS(BlueprintType)
class CROWDEVENTS_API UCrowdEventPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crowd Events", meta = (ShowOnlyInnerProperties))
	FCrowdEventPresetData Preset;
};

/** Parametri numerici di un evento che hanno un intervallo di addestramento. */
enum class ECrowdEventParameter : uint8
{
	AttractiveIntensity,
	RepulsiveIntensity,
	RepulsiveDecayM,
	AttractiveDecayM,
	PerceptionRadiusM,
	Salience,
	DurationS
};

namespace CrowdEvents
{
	/** Preset di partenza, definiti nel codice e quindi non modificabili dal pannello. */
	CROWDEVENTS_API const TArray<FCrowdEventPresetData>& GetBuiltInPresets();

	/**
	 * Intervallo in cui il parametro viene estratto in addestramento: da metà del valore
	 * più piccolo della specifica al doppio del più grande. Fuori da qui il comportamento
	 * della policy non è garantito.
	 */
	CROWDEVENTS_API void GetTrainingRange(ECrowdEventParameter Parameter, double& OutMin, double& OutMax);
}
