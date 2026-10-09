#include "SCrowdEventsPanel.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CrowdEvent.h"
#include "CrowdEventPlacementMode.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "CrowdEventsEditor"

namespace
{
	FText ClassText(ECrowdEventClass EventClass)
	{
		switch (EventClass)
		{
		case ECrowdEventClass::Illness: return LOCTEXT("ClassIllness", "Illness");
		case ECrowdEventClass::Danger: return LOCTEXT("ClassDanger", "Danger");
		default: return LOCTEXT("ClassAttractive", "Attractive");
		}
	}
}

UWorld* SCrowdEventsPanel::GetTargetWorld()
{
	if (!GEditor)
	{
		return nullptr;
	}
	if (GEditor->PlayWorld)
	{
		return GEditor->PlayWorld;
	}
	return GEditor->GetEditorWorldContext().World();
}

bool SCrowdEventsPanel::IsSimulating()
{
	return GEditor && GEditor->PlayWorld != nullptr;
}

ACrowdEvent* SCrowdEventsPanel::GetSelectedEvent() const
{
	return SelectedEvent.Get();
}

void SCrowdEventsPanel::ReloadPresets()
{
	Presets.Reset();
	for (const FCrowdEventPresetData& BuiltIn : CrowdEvents::GetBuiltInPresets())
	{
		Presets.Add(MakeShared<FCrowdEventPresetData>(BuiltIn));
	}

	// In coda i preset salvati nel progetto come Data Asset.
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByClass(UCrowdEventPreset::StaticClass()->GetClassPathName(), Assets);
	for (const FAssetData& Asset : Assets)
	{
		if (const UCrowdEventPreset* Preset = Cast<UCrowdEventPreset>(Asset.GetAsset()))
		{
			TSharedRef<FCrowdEventPresetData> Data = MakeShared<FCrowdEventPresetData>(Preset->Preset);
			if (Data->DisplayName.IsEmpty())
			{
				Data->DisplayName = FText::FromName(Asset.AssetName);
			}
			Presets.Add(Data);
		}
	}

	SelectedPreset = Presets.Num() > 0 ? Presets[0] : nullptr;
}

EActiveTimerReturnType SCrowdEventsPanel::RefreshEvents(double CurrentTime, float DeltaTime)
{
	TArray<ACrowdEvent*> Found;
	if (UWorld* World = GetTargetWorld())
	{
		for (TActorIterator<ACrowdEvent> It(World); It; ++It)
		{
			Found.Add(*It);
		}
	}

	// L'elenco si ricostruisce solo se l'insieme degli eventi è cambiato.
	bool bChanged = Found.Num() != EventItems.Num();
	for (int32 Index = 0; !bChanged && Index < Found.Num(); ++Index)
	{
		bChanged = EventItems[Index]->Event.Get() != Found[Index];
	}

	if (bChanged)
	{
		EventItems.Reset();
		FEventItemPtr ToSelect;
		for (ACrowdEvent* Event : Found)
		{
			FEventItemPtr Item = MakeShared<FEventItem>();
			Item->Event = Event;
			EventItems.Add(Item);
			if (Event == SelectedEvent.Get())
			{
				ToSelect = Item;
			}
		}
		if (EventList.IsValid())
		{
			EventList->RequestListRefresh();
			if (ToSelect.IsValid())
			{
				EventList->SetSelection(ToSelect, ESelectInfo::Direct);
			}
			else
			{
				SelectedEvent.Reset();
				EventList->ClearSelection();
			}
		}
	}

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SCrowdEventsPanel::MakeSectionHeader(const FText& Title) const
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		];
}

TSharedRef<SWidget> SCrowdEventsPanel::MakeParameterRow(const FText& Label, const FText& Tooltip, ECrowdEventParameter Parameter, TFunction<double*(ACrowdEvent&)> Access)
{
	double RangeMin = 0.0;
	double RangeMax = 1.0;
	CrowdEvents::GetTrainingRange(Parameter, RangeMin, RangeMax);

	// Fuori dalla modalità avanzata il valore non può uscire dall'intervallo di addestramento.
	auto MinValue = [this, RangeMin]() { return TOptional<double>(bAdvanced ? 0.0 : RangeMin); };
	auto MaxValue = [this, RangeMax]() { return TOptional<double>(bAdvanced ? RangeMax * 10.0 : RangeMax); };

	return SNew(SHorizontalBox)
		.ToolTipText(Tooltip)
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		.VAlign(VAlign_Center)
		.Padding(FMargin(8.f, 2.f))
		[
			SNew(STextBlock).Text(Label)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		.Padding(FMargin(8.f, 2.f))
		[
			SNew(SSpinBox<double>)
			.MinValue_Lambda(MinValue)
			.MaxValue_Lambda(MaxValue)
			.MinSliderValue_Lambda(MinValue)
			.MaxSliderValue_Lambda(MaxValue)
			.MinFractionalDigits(2)
			.MaxFractionalDigits(2)
			.IsEnabled_Lambda([this]() { return GetSelectedEvent() != nullptr; })
			.Value_Lambda([this, Access]()
			{
				ACrowdEvent* Event = GetSelectedEvent();
				return Event ? *Access(*Event) : 0.0;
			})
			.OnValueChanged_Lambda([this, Access](double NewValue)
			{
				if (ACrowdEvent* Event = GetSelectedEvent())
				{
					*Access(*Event) = NewValue;
					// A simulazione ferma la modifica va salvata con il livello.
					if (Event->GetWorld() && !Event->GetWorld()->IsGameWorld())
					{
						Event->MarkPackageDirty();
					}
				}
			})
		];
}

TSharedRef<ITableRow> SCrowdEventsPanel::MakeEventRow(FEventItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FEventItemPtr>, OwnerTable)
		[
			SNew(STextBlock)
			.Margin(FMargin(8.f, 3.f))
			.Text_Lambda([Item]()
			{
				const ACrowdEvent* Event = Item->Event.Get();
				if (!Event)
				{
					return FText::GetEmpty();
				}
				FText Status;
				if (!Event->IsEventActive())
				{
					Status = LOCTEXT("EventEnded", "ended");
				}
				else if (Event->GetRemainingTimeS() < 0.0)
				{
					Status = LOCTEXT("EventNoEnd", "no end");
				}
				else
				{
					Status = FText::Format(LOCTEXT("EventRemaining", "{0} s left"), FText::AsNumber(FMath::RoundToInt(Event->GetRemainingTimeS())));
				}
				return FText::Format(LOCTEXT("EventRow", "{0}  -  {1}  -  {2}"), Event->EventName, ClassText(Event->Params.EventClass), Status);
			})
		];
}

FReply SCrowdEventsPanel::OnStartSimulation()
{
	if (GEditor && !IsSimulating())
	{
		// In questo progetto funziona solo Simulate: il GameMode non ha un PlayerController.
		FRequestPlaySessionParams SessionParams;
		SessionParams.WorldType = EPlaySessionWorldType::SimulateInEditor;
		FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<IAssetViewport> ActiveViewport = LevelEditor.GetFirstActiveViewport();
		if (ActiveViewport.IsValid())
		{
			SessionParams.DestinationSlateViewport = TWeakPtr<IAssetViewport>(ActiveViewport);
		}
		GEditor->RequestPlaySession(SessionParams);
	}
	return FReply::Handled();
}

FReply SCrowdEventsPanel::OnStopSimulation()
{
	if (GEditor && IsSimulating())
	{
		FCrowdEventPlacementMode::Cancel();
		GEditor->RequestEndPlayMap();
	}
	return FReply::Handled();
}

FReply SCrowdEventsPanel::OnPlaceClicked()
{
	if (FCrowdEventPlacementMode::IsPlacing())
	{
		FCrowdEventPlacementMode::Cancel();
	}
	else if (SelectedPreset.IsValid())
	{
		TWeakPtr<SCrowdEventsPanel> WeakPanel = SharedThis(this);
		FCrowdEventPlacementMode::Begin(*SelectedPreset, [WeakPanel](ACrowdEvent* Event)
		{
			// L'evento appena piazzato diventa quello selezionato nel pannello.
			if (TSharedPtr<SCrowdEventsPanel> Panel = WeakPanel.Pin())
			{
				Panel->SelectedEvent = Event;
			}
		});
	}
	return FReply::Handled();
}

FReply SCrowdEventsPanel::OnEndSelected()
{
	if (ACrowdEvent* Event = GetSelectedEvent())
	{
		Event->EndEvent();
	}
	return FReply::Handled();
}

FReply SCrowdEventsPanel::OnRemoveSelected()
{
	if (ACrowdEvent* Event = GetSelectedEvent())
	{
		UWorld* World = Event->GetWorld();
		if (World && !World->IsGameWorld())
		{
			const FScopedTransaction Transaction(LOCTEXT("RemoveEvent", "Remove Crowd Event"));
			World->EditorDestroyActor(Event, true);
		}
		else
		{
			Event->Destroy();
		}
		SelectedEvent.Reset();
	}
	return FReply::Handled();
}

void SCrowdEventsPanel::Construct(const FArguments& InArgs)
{
	ReloadPresets();

	const auto HasSelection = [this]() { return GetSelectedEvent() != nullptr; };

	ChildSlot
	[
		SNew(SScrollBox)

		// --- Simulazione ---
		+ SScrollBox::Slot()
		[
			MakeSectionHeader(LOCTEXT("SimulationHeader", "Simulation"))
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text_Lambda([]()
			{
				return IsSimulating()
					? LOCTEXT("SimulationRunning", "Running. Changes made now are lost when the simulation stops.")
					: LOCTEXT("SimulationStopped", "Stopped. Use Start below: the editor's Play button does not work in this project.");
			})
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SButton)
				.Text(LOCTEXT("StartButton", "Start"))
				.IsEnabled_Lambda([]() { return !IsSimulating(); })
				.OnClicked(this, &SCrowdEventsPanel::OnStartSimulation)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("StopButton", "Stop"))
				.IsEnabled_Lambda([]() { return IsSimulating(); })
				.OnClicked(this, &SCrowdEventsPanel::OnStopSimulation)
			]
		]

		// --- Nuovo evento ---
		+ SScrollBox::Slot()
		[
			MakeSectionHeader(LOCTEXT("NewEventHeader", "New event"))
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SComboBox<FPresetPtr>)
				.OptionsSource(&Presets)
				.InitiallySelectedItem(SelectedPreset)
				.OnGenerateWidget_Lambda([](FPresetPtr Preset) -> TSharedRef<SWidget>
				{
					return SNew(STextBlock).Text(Preset.IsValid() ? Preset->DisplayName : FText::GetEmpty());
				})
				.OnSelectionChanged_Lambda([this](FPresetPtr Preset, ESelectInfo::Type)
				{
					if (Preset.IsValid())
					{
						SelectedPreset = Preset;
					}
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SelectedPreset.IsValid() ? SelectedPreset->DisplayName : LOCTEXT("NoPreset", "No preset");
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled_Lambda([this]() { return SelectedPreset.IsValid(); })
				.Text_Lambda([]()
				{
					return FCrowdEventPlacementMode::IsPlacing() ? LOCTEXT("CancelPlace", "Cancel") : LOCTEXT("PlaceButton", "Place in level");
				})
				.OnClicked(this, &SCrowdEventsPanel::OnPlaceClicked)
			]
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 0.f, 8.f, 4.f))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text_Lambda([]()
			{
				return FCrowdEventPlacementMode::IsPlacing()
					? LOCTEXT("PlaceHintActive", "Click on the floor in the viewport to place the event. Esc cancels.")
					: LOCTEXT("PlaceHintIdle", "Pick a preset, press Place in level, then click where the event should appear.");
			})
		]

		// --- Eventi presenti ---
		+ SScrollBox::Slot()
		[
			MakeSectionHeader(LOCTEXT("EventsHeader", "Events in the level"))
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(0.f, 4.f))
		[
			SNew(SBox)
			.MinDesiredHeight(60.f)
			.MaxDesiredHeight(160.f)
			[
				SAssignNew(EventList, SListView<FEventItemPtr>)
				.ListItemsSource(&EventItems)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SCrowdEventsPanel::MakeEventRow)
				.OnSelectionChanged_Lambda([this](FEventItemPtr Item, ESelectInfo::Type SelectInfo)
				{
					if (SelectInfo != ESelectInfo::Direct)
					{
						if (Item.IsValid())
						{
							SelectedEvent = Item->Event;
						}
						else
						{
							SelectedEvent.Reset();
						}
					}
				})
			]
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SButton)
				.Text(LOCTEXT("EndButton", "End event"))
				.ToolTipText(LOCTEXT("EndTooltip", "Close the selected event now: pedestrians stop reacting to it."))
				.IsEnabled_Lambda([this]() { const ACrowdEvent* Event = GetSelectedEvent(); return Event && Event->IsEventActive(); })
				.OnClicked(this, &SCrowdEventsPanel::OnEndSelected)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveButton", "Remove"))
				.IsEnabled_Lambda(HasSelection)
				.OnClicked(this, &SCrowdEventsPanel::OnRemoveSelected)
			]
		]

		// --- Evento selezionato ---
		+ SScrollBox::Slot()
		[
			MakeSectionHeader(LOCTEXT("SelectedHeader", "Selected event"))
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text_Lambda([this]()
			{
				const ACrowdEvent* Event = GetSelectedEvent();
				if (!Event)
				{
					return LOCTEXT("NoSelection", "Select an event in the list to edit it.");
				}
				FNumberFormattingOptions Format;
				Format.SetMinimumFractionalDigits(2);
				Format.SetMaximumFractionalDigits(2);
				return FText::Format(LOCTEXT("SelectedSummary", "{0}  -  ring radius {1} m"), Event->EventName, FText::AsNumber(Event->GetRingRadiusM(), &Format));
			})
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SSegmentedControl<ECrowdEventClass>)
			.IsEnabled_Lambda(HasSelection)
			.Value_Lambda([this]()
			{
				const ACrowdEvent* Event = GetSelectedEvent();
				return Event ? Event->Params.EventClass : ECrowdEventClass::Attractive;
			})
			.OnValueChanged_Lambda([this](ECrowdEventClass NewClass)
			{
				if (ACrowdEvent* Event = GetSelectedEvent())
				{
					Event->Params.EventClass = NewClass;
				}
			})
			+ SSegmentedControl<ECrowdEventClass>::Slot(ECrowdEventClass::Attractive)
			.Text(LOCTEXT("ClassAttractive", "Attractive"))
			+ SSegmentedControl<ECrowdEventClass>::Slot(ECrowdEventClass::Illness)
			.Text(LOCTEXT("ClassIllness", "Illness"))
			+ SSegmentedControl<ECrowdEventClass>::Slot(ECrowdEventClass::Danger)
			.Text(LOCTEXT("ClassDanger", "Danger"))
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("AttractiveIntensity", "Interest"),
				LOCTEXT("AttractiveIntensityTip", "Strength of the attractive component."),
				ECrowdEventParameter::AttractiveIntensity,
				[](ACrowdEvent& Event) { return &Event.Params.Field.AttractiveIntensity; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("RepulsiveIntensity", "Danger"),
				LOCTEXT("RepulsiveIntensityTip", "Strength of the repulsive component. Higher values widen the ring."),
				ECrowdEventParameter::RepulsiveIntensity,
				[](ACrowdEvent& Event) { return &Event.Params.Field.RepulsiveIntensity; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("RepulsiveDecay", "Repulsion decay (m)"),
				LOCTEXT("RepulsiveDecayTip", "Distance over which the repulsion fades."),
				ECrowdEventParameter::RepulsiveDecayM,
				[](ACrowdEvent& Event) { return &Event.Params.Field.RepulsiveDecayM; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("AttractiveDecay", "Attraction decay (m)"),
				LOCTEXT("AttractiveDecayTip", "Distance over which the attraction fades."),
				ECrowdEventParameter::AttractiveDecayM,
				[](ACrowdEvent& Event) { return &Event.Params.Field.AttractiveDecayM; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("PerceptionRadius", "Perception radius (m)"),
				LOCTEXT("PerceptionRadiusTip", "Pedestrians can notice the event only within this distance."),
				ECrowdEventParameter::PerceptionRadiusM,
				[](ACrowdEvent& Event) { return &Event.Params.PerceptionRadiusM; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("Salience", "Salience"),
				LOCTEXT("SalienceTip", "How easily the event is noticed."),
				ECrowdEventParameter::Salience,
				[](ACrowdEvent& Event) { return &Event.Params.Salience; })
		]
		+ SScrollBox::Slot()
		[
			MakeParameterRow(LOCTEXT("Duration", "Duration (s)"),
				LOCTEXT("DurationTip", "How long the event lasts. Zero means it never ends by itself."),
				ECrowdEventParameter::DurationS,
				[](ACrowdEvent& Event) { return &Event.Params.DurationS; })
		]

		// --- Vista ---
		+ SScrollBox::Slot()
		[
			MakeSectionHeader(LOCTEXT("ViewHeader", "View"))
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([]() { return ACrowdEvent::bDrawDebug ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([](ECheckBoxState State) { ACrowdEvent::bDrawDebug = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(LOCTEXT("DrawDebug", "Show ring and perception radius"))
			]
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SCheckBox)
			.ToolTipText(LOCTEXT("AdvancedTip", "Lets values leave the ranges seen in training. Pedestrian behaviour is not guaranteed outside them."))
			.IsChecked_Lambda([this]() { return bAdvanced ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bAdvanced = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(LOCTEXT("Advanced", "Advanced: allow values outside the training ranges"))
			]
		]
	];

	RegisterActiveTimer(0.25f, FWidgetActiveTimerDelegate::CreateSP(this, &SCrowdEventsPanel::RefreshEvents));
}

#undef LOCTEXT_NAMESPACE
