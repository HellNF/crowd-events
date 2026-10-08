#include "CrowdEventsModule.h"

DEFINE_LOG_CATEGORY(LogCrowdEvents);

void FCrowdEventsModule::StartupModule()
{
	// Riga cercata nel log dell'editor per confermare che il plugin si è caricato.
	UE_LOG(LogCrowdEvents, Log, TEXT("CrowdEvents caricato"));
}

void FCrowdEventsModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FCrowdEventsModule, CrowdEvents)
