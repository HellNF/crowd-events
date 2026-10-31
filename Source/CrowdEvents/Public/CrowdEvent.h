#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrowdEventTypes.h"
#include "CrowdEvent.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

/**
 * Un evento nel livello (specifica §3.3): una figura visibile, un marcatore che i raggi
 * dei pedoni colpiscono e i parametri del campo. La bolla attorno all'evento non è un
 * volume: è solo una distanza nei calcoli.
 */
UCLASS()
class CROWDEVENTS_API ACrowdEvent : public AActor
{
	GENERATED_BODY()

public:
	ACrowdEvent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FText EventName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ShowOnlyInnerProperties))
	FCrowdEventParams Params;

	/** Copia nome e parametri di un preset nell'evento. */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	void ApplyPreset(const FCrowdEventPresetData& Preset);

	/** In addestramento, falso limita chi reagisce alla schivata (stadio 1). La dimostrazione lo ignora. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bAllowLingerInTraining = true;

	/** In addestramento, quota di chi sosta fra chi reagisce e può sostare (0,5 = metà, come fino alla 0.4.1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double LingerShareInTraining = 0.5;

	/** In addestramento, vero lascia sostare anche i fantasmi; falso li limita alla schivata, come nella 0.4.1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bGhostsMayLingerInTraining = false;

	/** Premio una tantum a chi arriva fermo sull'anello (o sul soccorso). 0 = come fino alla 0.4.2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0"))
	double LingerArrivalBonusInTraining = 0.0;

	/** Penalità una tantum a chi perde lo stato 2 senza essere arrivato: esce dal raggio o supera il tempo di avvicinamento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0"))
	double LingerMissPenaltyInTraining = 0.0;

	/** Moltiplicatore in più del termine di regolarità di DCR nello stato 2 (1 = come fino alla 0.4.3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double LingerSmoothnessMultiplierInTraining = 1.0;

	/** Condizione di arrivo sull'anello per questo evento; 0 = quella del componente (0,5 m e 0,2 m/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0"))
	double ArrivalToleranceOverrideM = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0"))
	double ArrivalSpeedOverrideMS = 0.0;

	/**
	 * Vero: nello stato 2 la velocità massima segue il bersaglio, 200 × v*(t) cm/s, come DCR la
	 * lega alla velocità desiderata negli stati 0 e 1; avvicinandosi all'anello il pedone rallenta
	 * fino a fermarsi. Falso: 2 × la velocità desiderata iniziale, fissa (specifica §5.5, fino alla 0.4.4).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bLingerSpeedFollowsTarget = false;

	/**
	 * Vero: chi percepisce un pericolo entra nello stato 1 con il ruolo «dopo la fuga», cioè la
	 * schivata con la bolla larga quanto la distanza di sicurezza, e non passa mai allo stato 3.
	 * Serve a provare se la schivata imparata basta a far fuggire (0.4.7). Falso: stato 3.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bDangerAsWideAvoidance = false;

	/**
	 * Malore: può soccorrere solo chi ha l'evento davanti e lo passerebbe a non più di questa
	 * distanza laterale, in metri; 0 = chiunque (come fino alla 0.4.6).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (ClampMin = "0.0"))
	double RescueMaxLateralM = 0.0;

	/** Se vero, percepiscono l'evento solo i pedoni dentro ReachBox, guardando solo X e Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bLimitReach = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events", meta = (EditCondition = "bLimitReach"))
	FBox ReachBox = FBox(ForceInit);

	bool IsInReach(const FVector& Location) const { return !bLimitReach || ReachBox.IsInsideXY(Location); }

	/** Raggio dell'anello in metri per un pedone con Alpha = Beta = 1. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	double GetRingRadiusM() const;

	/** Secondi che mancano alla fine; -1 se l'evento non ha una durata. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	double GetRemainingTimeS() const;

	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	bool IsEventActive() const { return !bEnded; }

	/** Chiude l'evento: i raggi non lo colpiscono più e la figura sparisce. */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	void EndEvent();

	/** Numero progressivo dato dal gestore all'avvio della simulazione; -1 prima. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 GetEventId() const { return EventId; }

	/** Disegna anello e raggio di percezione di tutti gli eventi; lo comanda il pannello. */
	static bool bDrawDebug;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	// I cerchi si vedono anche a simulazione ferma.
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

protected:
	/** Cilindro invisibile che blocca solo il canale dei raggi, non il movimento. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd Events")
	TObjectPtr<UCapsuleComponent> Marker;

	UPROPERTY(VisibleAnywhere, Category = "Crowd Events")
	TObjectPtr<UStaticMeshComponent> Figure;

private:
	friend class UCrowdEventsSubsystem;

	int32 EventId = -1;
	double ElapsedS = 0.0;
	bool bEnded = false;
};
