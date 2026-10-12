#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrowdReactionTypes.h"
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

	/** Raggio della bolla, in metri: l'anello del pedone tipico più lo spessore del cerchio (specifica §4.3). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double BubbleRadius(const FCrowdEventField& Field, double ContactDistanceM = 0.8, double RingWidthM = 1.5);

	/**
	 * Distanza dall'evento a cui il pedone deve riferirsi, in metri (specifica §4.3):
	 * la bolla nello stato 1, la distanza di sicurezza dopo una fuga, l'anello o il contatto
	 * nello stato 2. Restituisce -1 negli stati 0 e 3, che non ne hanno una.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double ReferenceRadius(ECrowdReactionState State, ECrowdReactionRole Role, const FCrowdEventField& Field, double Alpha, double Beta,
		double SafeRadiusM, double ContactDistanceM = 0.8, double RingWidthM = 1.5);

	/** Stato 1: penalità della bolla (specifica §8.3). Circa -1 sul bordo, fino a -3 dentro. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double AvoidReward(double Beta, double DistanceM, double BubbleRadiusM, double RepulsiveDecayM);

	/** Stato 1 dopo la fuga: muro alla distanza di sicurezza, senza il suo peso (specifica §8.3). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double SafeWallReward(double DistanceM, double SafeRadiusM);

	/**
	 * Stato 2: il campo rovesciato e normalizzato (specifica §8.3). Vale 0 a TargetRadiusM
	 * (anello o contatto) e scende fino a -1 allontanandosi.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double LingerReward(const FCrowdEventField& Field, double Alpha, double Beta, double DistanceM, double TargetRadiusM,
		double PerceptionRadiusM, double ContactDistanceM = 0.8);

	/** Stato 2: velocità bersaglio in m/s, che scende a zero arrivando a TargetRadiusM (specifica §5.5). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double LingerSpeedTarget(double BaseSpeedMS, double DistanceM, double TargetRadiusM, double LengthM = 1.0);

	/** Stato 2: termine di velocità, diviso per la velocità desiderata iniziale (specifica §8.3). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double LingerSpeedReward(double SpeedMS, double TargetSpeedMS, double BaseSpeedMS);

	/** Stato 3: quanto il pedone si è allontanato nel passo, rispetto alla velocità di panico (specifica §8.3). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double FleeReward(double DistanceM, double PreviousDistanceM, double PanicSpeedMS, double DeltaTimeS);

	/**
	 * Probabilità che il pedone si accorga dell'evento in un passo di DeltaTimeS (specifica §5.2).
	 * FleeingSeen è il numero di pedoni in fuga che vede: vale solo per il pericolo.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static double NoticeProbability(double Attention, double Salience, double DistanceM, bool bInFront, int32 FleeingSeen, double DeltaTimeS,
		double PerceptionDecayM = 10.0, double ContagionWeight = 0.5);

	/** Moltiplicatori dei cinque termini della reward di DCR per stato e ruolo (specifica §8.4). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static FCrowdRewardMultipliers RewardMultipliers(ECrowdReactionState State, ECrowdReactionRole Role, double PanicK = 0.5);
};
