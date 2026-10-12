#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdReactionTypes.h"
#include "CrowdEventsSubsystem.generated.h"

class ACrowdEvent;
class UCrowdReactionComponent;

/**
 * Gestore di eventi e stati (specifica §5, §9.3): conosce gli eventi attivi e i pedoni,
 * decide chi si accorge di che cosa, assegna gli stati e ne governa le transizioni.
 * Niente di ciò che fa viene appreso dalla policy. Esiste da solo in ogni mondo di gioco.
 */
UCLASS()
class CROWDEVENTS_API UCrowdEventsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FCrowdRuleSettings Rules;

	/**
	 * Regole di dimostrazione o di addestramento. La variabile di console
	 * CrowdEvents.Training (0 o 1) ha la precedenza, se impostata.
	 */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	void SetRuleSet(ECrowdRuleSet NewRuleSet) { RuleSet = NewRuleSet; }

	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	ECrowdRuleSet GetRuleSet() const;

	/** Fissa il seme delle estrazioni del gestore, per ripetere una corsa. */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/** Numero di pedoni nello stato indicato. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 CountInState(ECrowdReactionState State) const;

	/** Numero di soccorritori dell'evento. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 CountRescuers(const ACrowdEvent* Event) const;

	/** Gestore del mondo a cui appartiene l'oggetto. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events", meta = (WorldContext = "WorldContextObject"))
	static UCrowdEventsSubsystem* GetCrowdEventsSubsystem(const UObject* WorldContextObject);

	void RegisterEvent(ACrowdEvent* Event);
	void UnregisterEvent(ACrowdEvent* Event);
	void RegisterReaction(UCrowdReactionComponent* Reaction);
	void UnregisterReaction(UCrowdReactionComponent* Reaction);

	double RandomInRange(double Min, double Max) { return Min + (Max - Min) * Random.GetFraction(); }

	/** Disegna sopra ogni pedone un punto con il colore dello stato; lo comanda il pannello. */
	static bool bDrawStates;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void UpdateReaction(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining);
	/** Transizioni dello stato in corso (specifica §5.4). */
	void UpdateTransitions(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining);
	/** Cerca un evento a cui il pedone deve ancora reagire e gli assegna lo stato. */
	void LookForEvents(UCrowdReactionComponent& Reaction, double DeltaTimeS, bool bTraining);
	void AssignState(UCrowdReactionComponent& Reaction, ACrowdEvent& Event, bool bTraining);
	void StartLinger(UCrowdReactionComponent& Reaction, ACrowdEvent& Event, ECrowdReactionRole Role, bool bTraining);
	/** Pedoni in fuga davanti al pedone, entro il raggio del contagio. */
	int32 CountFleeingSeen(const UCrowdReactionComponent& Reaction) const;
	void DrawStates() const;

	ECrowdRuleSet RuleSet = ECrowdRuleSet::Demonstration;
	FRandomStream Random = FRandomStream(12345);
	int32 NextEventId = 0;

	TArray<TWeakObjectPtr<ACrowdEvent>> Events;
	TArray<TWeakObjectPtr<UCrowdReactionComponent>> Reactions;
};
