#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

CROWDEVENTS_API DECLARE_LOG_CATEGORY_EXTERN(LogCrowdEvents, Log, All);

class FCrowdEventsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
