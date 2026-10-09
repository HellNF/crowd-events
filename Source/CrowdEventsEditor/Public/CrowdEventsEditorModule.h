#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

/** Modulo dell'editor: registra il pannello «Crowd Events» e la modalità di piazzamento. */
class FCrowdEventsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnPanelTab(const FSpawnTabArgs& Args);
};
