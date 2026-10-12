#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrowdReactionTypes.h"
#include "CrowdReactionComponent.generated.h"

class ACrowdEvent;

/**
 * Reazione di un pedone agli eventi (specifica §10): stato e ruolo assegnati dal gestore,
 * sensibilità individuale, osservazioni e termini nuovi della reward. Va aggiunto a BP_Pawn.
 * Lo stato lo cambia solo UCrowdEventsSubsystem; i Blueprint di DCR leggono.
 */
UCLASS(ClassGroup = (CrowdEvents), meta = (BlueprintSpawnableComponent))
class CROWDEVENTS_API UCrowdReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrowdReactionComponent();

	/** Quanto il pedone è attratto dagli eventi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double Alpha = 1.0;

	/** Quanto il pedone è respinto dagli eventi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double Beta = 1.0;

	/** Attenzione, fra 0 e 1: bassa per chi è distratto. Conta solo in dimostrazione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double Attention = 1.0;

	/** Probabilità di fermarsi a guardare, una volta accorto. Conta solo in dimostrazione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	double CuriosityProbability = 0.3;

	/** Propenso a soccorrere. Conta solo in dimostrazione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bWillingToHelp = true;

	/** Falso per i pedoni fantasma del training: non percepiscono eventi e non contano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	bool bPerceivesEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Events")
	FCrowdReactionSettings Settings;

	/**
	 * Da chiamare a ogni reset del pedone, con la velocità desiderata estratta da DCR in m/s:
	 * riporta lo stato a 0 e dimentica gli eventi già incontrati.
	 */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	void ResetReaction(double BaseDesiredSpeedMS);

	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	ECrowdReactionState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	ECrowdReactionRole GetRole() const { return Role; }

	/** Evento a cui il pedone sta reagendo; nullo nello stato 0. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	ACrowdEvent* GetEvent() const;

	/**
	 * Vero se l'attore è l'evento a cui il pedone sta reagendo. Per CastRays: solo in questo
	 * caso il raggio segna «colpisce un evento» e l'attore resta fuori da ostacoli e HitMap.
	 * Per chi è nello stato 0, e per ogni altro evento, l'evento è un ostacolo come gli altri.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	bool IsReactingTo(const AActor* Actor) const;

	/** Stato come numero da 0 a 3, per il registro di SaveData. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 GetStateIndex() const { return static_cast<int32>(State); }

	/** Identificativo dell'evento a cui il pedone reagisce, -1 se nessuno: per il registro di SaveData. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	int32 GetEventId() const;

	/** Distanza in metri dal centro dell'evento, -1 se nessuno: per il registro di SaveData. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	double GetDistanceToEventM() const;

	/**
	 * Le 12 osservazioni sugli eventi (specifica §6.2), nell'ordine: stato (4), soccorritore (1),
	 * evento noto (1), classe (3), posizione dell'evento nel sistema del pedone divisa per il
	 * raggio di percezione (2: avanti, destra), scarto dal raggio di riferimento (1).
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	TArray<float> GetEventObservations() const;

	/** Moltiplicatori dei cinque termini della reward di DCR nello stato attuale. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	FCrowdRewardMultipliers GetRewardMultipliers() const;

	/**
	 * Somma già pesata dei termini nuovi della reward (specifica §8.2-8.4), da aggiungere a
	 * quella di DCR: termine di evento, guardare l'evento e velocità nello stato 2, premio
	 * all'uscita dalla fuga. Da chiamare una volta per decisione: chiamate ripetute nello
	 * stesso istante restituiscono lo stesso valore.
	 */
	UFUNCTION(BlueprintCallable, Category = "Crowd Events")
	double GetEventReward();

	/**
	 * Velocità bersaglio in m/s per la reward e per l'osservazione DesiredVelocity (specifica §5.5).
	 * DcrDesiredSpeedMS è il valore che DCR ha già calcolato: resta quello negli stati 0 e 1.
	 */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	double GetSpeedTargetMS(double DcrDesiredSpeedMS) const;

	/** Velocità massima in cm/s da dare a MaxWalkSpeed (specifica §5.5). */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	double GetMaxWalkSpeedCm(double DcrDesiredSpeedMS) const;

	/** Vero negli stati 0 e 1: solo questi pedoni entrano nella media delle velocità dei vicini. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	bool CountsInNeighbourAverage() const;

	/** Distanza dall'evento a cui il pedone deve sostare, in metri: l'anello o il contatto. */
	double GetLingerTargetRadiusM() const;

	/** Componente di reazione dell'attore, se ne ha uno. */
	UFUNCTION(BlueprintPure, Category = "Crowd Events")
	static UCrowdReactionComponent* GetReactionComponent(const AActor* Actor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UCrowdEventsSubsystem;

	/** Lo chiama solo il gestore. */
	void SetReaction(ECrowdReactionState NewState, ECrowdReactionRole NewRole, ACrowdEvent* NewEvent);
	void ClearReaction();

	/** Beta effettivo: chi soccorre non è respinto. */
	double GetEffectiveBeta() const;
	double GetSpeedMS() const;

	UPROPERTY(VisibleAnywhere, Category = "Crowd Events")
	ECrowdReactionState State = ECrowdReactionState::None;

	UPROPERTY(VisibleAnywhere, Category = "Crowd Events")
	ECrowdReactionRole Role = ECrowdReactionRole::Normal;

	TWeakObjectPtr<ACrowdEvent> Event;

	/** Eventi per cui la decisione è già stata presa: non si riassegna uno stato due volte. */
	TSet<TWeakObjectPtr<ACrowdEvent>> HandledEvents;

	/** Eventi per cui il premio di uscita dalla fuga è già stato dato. */
	TSet<TWeakObjectPtr<ACrowdEvent>> SafeBonusGiven;

	double BaseSpeedMS = 1.5;
	double PanicSpeedMS = 2.75;

	// Sosta: tempo che resta dopo l'arrivo e tempo passato da quando è iniziata.
	double LingerRemainingS = 0.0;
	double LingerElapsedS = 0.0;
	bool bArrived = false;

	bool bPendingSafeBonus = false;

	// Reward: distanza e istante dell'ultima chiamata, per il termine di fuga.
	double PreviousDistanceM = -1.0;
	double LastRewardTimeS = -1.0;
	double LastReward = 0.0;
};
