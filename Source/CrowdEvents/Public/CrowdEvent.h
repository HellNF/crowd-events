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

	/** Disegna anello e raggio di percezione di tutti gli eventi; lo comanda il pannello. */
	static bool bDrawDebug;

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
	double ElapsedS = 0.0;
	bool bEnded = false;
};
