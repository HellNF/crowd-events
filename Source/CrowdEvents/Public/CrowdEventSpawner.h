#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrowdEventTypes.h"
#include "CrowdEventSpawner.generated.h"

class ACrowdEvent;
class UBoxComponent;

/**
 * Genera gli eventi dell'addestramento (specifica §9.4): uno alla volta, in un punto a caso
 * di Area, con l'anello estratto fra MinRingM e MaxRingM. Finito un evento, dopo una pausa
 * casuale ne compare un altro. Lo percepiscono solo i pedoni dentro Reach, così un evento
 * non arriva ai corridoi vicini attraverso i muri. Il campo si estrae con Sample Ring Field:
 * vale per le classi con anello (attrattivo, malore), non per il pericolo.
 */
UCLASS()
class CROWDEVENTS_API ACrowdEventSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACrowdEventSpawner();

	/** Dove può comparire l'evento; l'evento sta alla quota della base del box. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crowd Events")
	TObjectPtr<UBoxComponent> Area;

	/** Lo spazio dei pedoni che percepiscono l'evento, per esempio tutto il corridoio. Conta il suo ingombro allineato agli assi. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crowd Events")
	TObjectPtr<UBoxComponent> Reach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bSpawnEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	ECrowdEventClass EventClass = ECrowdEventClass::Attractive;

	/** Falso nello stadio 1: chi reagisce schiva soltanto (stati 0 e 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bAllowLinger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double MinRingM = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double MaxRingM = 5.0;

	/** Raggio di percezione, estratto uniforme fra X e Y metri. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D PerceptionRadiusM = FVector2D(4.0, 30.0);

	/** Durata di ogni evento, estratta uniforme fra X e Y secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D DurationS = FVector2D(60.0, 60.0);

	/** Pausa fra la fine di un evento e il successivo, in secondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D PauseS = FVector2D(5.0, 15.0);

	/** Attesa prima del primo evento, in secondi: sfasa i corridoi fra loro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FVector2D FirstDelayS = FVector2D(0.0, 10.0);

	/** Seme delle estrazioni; 0 ne sceglie uno diverso a ogni avvio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	int32 Seed = 0;

	/** L'evento in corso; nullo durante le pause. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	ACrowdEvent* GetCurrentEvent() const { return Current; }

	/** Eventi generati dall'avvio. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 GetSpawnedCount() const { return SpawnedCount; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SpawnEvent();
	double RandomIn(const FVector2D& Range) const;

	UPROPERTY(Transient)
	TObjectPtr<ACrowdEvent> Current;

	FRandomStream Stream;
	double WaitS = 0.0;
	int32 SpawnedCount = 0;
};
