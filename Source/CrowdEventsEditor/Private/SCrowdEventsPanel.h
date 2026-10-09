#pragma once

#include "CoreMinimal.h"
#include "CrowdEventTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ACrowdEvent;
class ITableRow;
class STableViewBase;

/**
 * Pannello «Crowd Events»: avvia e ferma la simulazione, piazza eventi da un preset,
 * elenca quelli presenti e ne modifica i parametri mentre la simulazione gira.
 */
class SCrowdEventsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCrowdEventsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FEventItem
	{
		TWeakObjectPtr<ACrowdEvent> Event;
	};
	typedef TSharedPtr<FEventItem> FEventItemPtr;
	typedef TSharedPtr<FCrowdEventPresetData> FPresetPtr;

	/** Mondo su cui lavora il pannello: la simulazione se è in corso, altrimenti il livello aperto. */
	static UWorld* GetTargetWorld();
	static bool IsSimulating();

	void ReloadPresets();
	EActiveTimerReturnType RefreshEvents(double CurrentTime, float DeltaTime);
	ACrowdEvent* GetSelectedEvent() const;

	TSharedRef<SWidget> MakeSectionHeader(const FText& Title) const;
	TSharedRef<SWidget> MakeParameterRow(const FText& Label, const FText& Tooltip, ECrowdEventParameter Parameter, TFunction<double*(ACrowdEvent&)> Access);
	TSharedRef<ITableRow> MakeEventRow(FEventItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnStartSimulation();
	FReply OnStopSimulation();
	FReply OnPlaceClicked();
	FReply OnEndSelected();
	FReply OnRemoveSelected();

	TArray<FPresetPtr> Presets;
	FPresetPtr SelectedPreset;

	TArray<FEventItemPtr> EventItems;
	TSharedPtr<SListView<FEventItemPtr>> EventList;
	TWeakObjectPtr<ACrowdEvent> SelectedEvent;

	/** Con la modalità avanzata i cursori escono dagli intervalli di addestramento. */
	bool bAdvanced = false;
};
