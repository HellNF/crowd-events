#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrowdEventsLibrary.generated.h"

/**
 * Parametri di campo di un evento (specifica della milestone 1, §3.2).
 * Le distanze sono in metri, non in unità di Unreal.
 */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdEventField
{
	GENERATED_BODY()

	/** Interesse: intensità della componente attrattiva, fra 0 e 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double AttractiveIntensity = 1.0;

	/** Pericolo: intensità della componente repulsiva. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double RepulsiveIntensity = 0.77;

	/** Decadimento della componente repulsiva, in metri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double RepulsiveDecayM = 1.0;

	/** Decadimento della componente attrattiva, in metri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double AttractiveDecayM = 4.0;
};

UCLASS()
class CROWDEVENTS_API UCrowdEventsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Versione del plugin: serve a controllare da Blueprint che il nodo sia richiamabile. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static FString GetCrowdEventsVersion();

	/**
	 * Campo percepito dal pedone a distanza DistanceM dall'evento (specifica §4.1):
	 * U = Beta * Irep * exp(-d / Brep) - Alpha * Iatt * exp(-d / Batt).
	 * Alpha e Beta sono quanto il pedone è attratto e quanto è respinto.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double PerceivedField(const FCrowdEventField& Field, double Alpha, double Beta, double DistanceM);

	/**
	 * Raggio dell'anello, in metri: la distanza a cui il campo ha il minimo (specifica §4.2).
	 * Se il minimo non esiste (argomento del logaritmo non superiore a 1, una componente
	 * assente o Batt non maggiore di Brep) restituisce ContactDistanceM.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double RingRadius(const FCrowdEventField& Field, double Alpha, double Beta, double ContactDistanceM = 0.8);
};
