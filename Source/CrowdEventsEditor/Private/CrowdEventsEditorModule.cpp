#include "CrowdEventsEditorModule.h"

#include "CrowdEventPlacementMode.h"
#include "EditorModeRegistry.h"
#include "Framework/Docking/TabManager.h"
#include "SCrowdEventsPanel.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "CrowdEventsEditor"

namespace
{
	const FName PanelTabName(TEXT("CrowdEventsPanel"));
}

void FCrowdEventsEditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FCrowdEventPlacementMode>(
		FCrowdEventPlacementMode::ModeId,
		LOCTEXT("PlacementModeName", "Crowd Event Placement"),
		FSlateIcon(),
		false);

	// Il pannello si apre dal menu Window dell'editor.
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PanelTabName, FOnSpawnTab::CreateRaw(this, &FCrowdEventsEditorModule::SpawnPanelTab))
		.SetDisplayName(LOCTEXT("PanelTabTitle", "Crowd Events"))
		.SetTooltipText(LOCTEXT("PanelTabTooltip", "Place and edit crowd events while the simulation runs."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
}

void FCrowdEventsEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PanelTabName);
	FEditorModeRegistry::Get().UnregisterMode(FCrowdEventPlacementMode::ModeId);
}

TSharedRef<SDockTab> FCrowdEventsEditorModule::SpawnPanelTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCrowdEventsPanel)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCrowdEventsEditorModule, CrowdEventsEditor)
