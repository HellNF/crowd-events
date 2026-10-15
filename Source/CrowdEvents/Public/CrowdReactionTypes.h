#pragma once

#include "CoreMinimal.h"
#include "CrowdReactionTypes.generated.h"

/** Stati di reazione, assegnati dal sistema e non dalla policy (specifica §5.1). */
UENUM(BlueprintType)
enum class ECrowdReactionState : uint8
{
	/** 0: nessuna reazione. */
	None,
	/** 1: schivare; il goal resta. */
	Avoid,
	/** 2: sostare; il goal è sospeso. */
	Linger,
	/** 3: fuggire; il goal è abbandonato. */
	Flee
};

/** Ruolo dentro lo stato: i primi due valgono per lo stato 1, gli altri due per lo stato 2. */
UENUM(BlueprintType)
enum class ECrowdReactionRole : uint8
{
	Normal,
	/** Stato 1 dopo una fuga: la distanza di sicurezza è un muro. */
	AfterFlight,
	/** Stato 2: guarda dall'anello. */
	Spectator,
	/** Stato 2: soccorre a contatto. */
	Rescuer
};

/** Regole con cui il sistema assegna gli stati. */
UENUM(BlueprintType)
enum class ECrowdRuleSet : uint8
{
	/** Awareness e regole per classe di evento (specifica §5.2, §5.3). */
	Demonstration,
	/** Stati casuali fra quelli ammessi, senza awareness (specifica §9.3). */
	Training
};

/** Moltiplicatori dei cinque termini della reward di DCR (specifica §8.4). */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdRewardMultipliers
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Crowd Events")
	double Goal = 1.0;

	/** Zero nello stato 2: lì il termine di velocità è dentro la reward aggiuntiva. */
	UPROPERTY(BlueprintReadOnly, Category = "Crowd Events")
	double Speed = 1.0;

	UPROPERTY(BlueprintReadOnly, Category = "Crowd Events")
	double Social = 1.0;

	UPROPERTY(BlueprintReadOnly, Category = "Crowd Events")
	double Spawn = 1.0;

	UPROPERTY(BlueprintReadOnly, Category = "Crowd Events")
	double Smoothness = 1.0;
};

/** Pesi e distanze della reazione di un pedone (specifica §5.4, §8.4). Valori iniziali, da tarare. */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdReactionSettings
{
	GENERATED_BODY()

	/** Peso del termine di evento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double EventWeight = 0.1;

	/** Peso del guardare l'evento nello stato 2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double LookWeight = 0.01;

	/** Peso del termine di velocità di DCR. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double DcrSpeedWeight = 0.001;

	/** Moltiplicatore del termine di velocità nello stato 2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double LingerSpeedMultiplier = 50.0;

	/** Panico: quanto calano penalità sociale e regolarità nello stato 3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double PanicK = 0.5;

	/** Peso del muro alla distanza di sicurezza, dopo la fuga. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double SafeWallWeight = 1.0;

	/** Premio alla prima uscita dalla fuga, una sola volta per evento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double SafeBonus = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double SafeRadiusM = 15.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double PanicRadiusM = 10.0;

	/** Raggio della capsula più raggio del marcatore più un margine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double ContactDistanceM = 0.8;

	/** Spessore del cerchio di chi guarda, aggiunto all'anello per avere la bolla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double RingWidthM = 1.5;

	/** Distanza su cui la velocità bersaglio dello stato 2 scende a zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double LingerLengthM = 1.0;

	/** Il pedone è arrivato quando è entro questa distanza dal bersaglio... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double ArrivalToleranceM = 0.5;

	/** ...e più lento di così. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double ArrivalSpeedMS = 0.2;

	/**
	 * Se vero, nello stato 0 tutte le osservazioni sugli eventi valgono zero, bit dello stato
	 * compreso. Falso: nello stato 0 il bit dello stato 0 vale 1, cioè [1, 0, …, 0], gli
	 * ingressi con cui è stata addestrata la baseline (verificato sul remoto).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bNeutralStateAsZeros = false;
};

/** Parametri delle regole di assegnazione (specifica §5.2-5.4, §9.3). Valori iniziali, da tarare. */
USTRUCT(BlueprintType)
struct CROWDEVENTS_API FCrowdRuleSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	int32 MaxRescuers = 2;

	/** Decadimento della percezione, in metri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double PerceptionDecayM = 10.0;

	/** Peso del contagio da chi fugge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double ContagionWeight = 0.5;

	/** Entro questa distanza, e davanti, un pedone in fuga conta per il contagio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double ContagionRadiusM = 20.0;

	/** Sosta di uno spettatore in dimostrazione, in secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D SpectatorLingerS = FVector2D(10.0, 30.0);

	/** Durata del soccorso in dimostrazione, in secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D RescueLingerS = FVector2D(30.0, 60.0);

	/** Soste e soccorsi in addestramento, in secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D TrainingLingerS = FVector2D(5.0, 20.0);

	/** Quota di pedoni che in addestramento resta nello stato 0 anche dentro il raggio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double TrainingNeutralQuota = 0.2;

	/** Chi non raggiunge il bersaglio entro questo tempo smette di sostare, in secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double MaxApproachS = 30.0;
};
