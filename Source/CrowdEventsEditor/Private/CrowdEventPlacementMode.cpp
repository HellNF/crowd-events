#include "CrowdEventPlacementMode.h"

#include "CrowdEvent.h"
#include "CrowdEventsLibrary.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "ScopedTransaction.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "TimerManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "CrowdEventsEditor"

const FEditorModeID FCrowdEventPlacementMode::ModeId(TEXT("CrowdEventPlacement"));

namespace
{
	const double MetersToUnits = 100.0;

	void Notify(const FText& Message)
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 3.f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	// La modalità non si spegne mentre sta ancora gestendo il proprio clic.
	void DeactivateNextTick()
	{
		if (GEditor)
		{
			GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([]()
			{
				GLevelEditorModeTools().DeactivateMode(FCrowdEventPlacementMode::ModeId);
			}));
		}
	}
}

void FCrowdEventPlacementMode::Begin(const FCrowdEventPresetData& Preset, TFunction<void(ACrowdEvent*)> OnPlaced)
{
	GLevelEditorModeTools().ActivateMode(ModeId);
	if (FCrowdEventPlacementMode* Mode = GLevelEditorModeTools().GetActiveModeTyped<FCrowdEventPlacementMode>(ModeId))
	{
		Mode->PendingPreset = Preset;
		Mode->PlacedCallback = MoveTemp(OnPlaced);
		Mode->bHoverValid = false;
	}
}

void FCrowdEventPlacementMode::Cancel()
{
	GLevelEditorModeTools().DeactivateMode(ModeId);
}

bool FCrowdEventPlacementMode::IsPlacing()
{
	return GLevelEditorModeTools().IsModeActive(ModeId);
}

bool FCrowdEventPlacementMode::FindFloor(UWorld* World, const FVector& Origin, const FVector& Direction, FVector& OutLocation)
{
	if (!World)
	{
		return false;
	}

	// Gli altri eventi e i pedoni non contano: si cerca la geometria del livello.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CrowdEventPlacement), true);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA<ACrowdEvent>() || It->IsA<APawn>())
		{
			Query.AddIgnoredActor(*It);
		}
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 1000000.0, ECC_Visibility, Query))
	{
		return false;
	}
	// Un muro o una parete inclinata non è un punto valido.
	if (Hit.ImpactNormal.Z < 0.7)
	{
		return false;
	}

	OutLocation = Hit.ImpactPoint;
	return true;
}

bool FCrowdEventPlacementMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (Click.GetKey() != EKeys::LeftMouseButton)
	{
		return false;
	}

	UWorld* World = InViewportClient ? InViewportClient->GetWorld() : nullptr;
	FVector Location;
	if (!FindFloor(World, Click.GetOrigin(), Click.GetDirection(), Location))
	{
		Notify(LOCTEXT("InvalidPoint", "Not a valid spot: click on the floor."));
		return true;
	}

	ACrowdEvent* Event = nullptr;
	{
		// A simulazione ferma l'evento entra nel livello e si può annullare con Ctrl+Z.
		TUniquePtr<FScopedTransaction> Transaction;
		if (!World->IsGameWorld())
		{
			Transaction = MakeUnique<FScopedTransaction>(LOCTEXT("PlaceEvent", "Place Crowd Event"));
			World->GetCurrentLevel()->Modify();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Event = World->SpawnActor<ACrowdEvent>(ACrowdEvent::StaticClass(), FTransform(Location), SpawnParameters);
		if (Event)
		{
			Event->ApplyPreset(PendingPreset);
			Event->SetActorLabel(PendingPreset.DisplayName.ToString());
		}
	}

	if (Event && PlacedCallback)
	{
		PlacedCallback(Event);
	}
	DeactivateNextTick();
	return true;
}

bool FCrowdEventPlacementMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Key == EKeys::Escape && Event == IE_Pressed)
	{
		DeactivateNextTick();
		return true;
	}
	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

bool FCrowdEventPlacementMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y)
{
	if (ViewportClient && Viewport)
	{
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags));
		FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
		const FViewportCursorLocation Cursor(View, ViewportClient, X, Y);
		bHoverValid = FindFloor(ViewportClient->GetWorld(), Cursor.GetOrigin(), Cursor.GetDirection(), HoverLocation);
		ViewportClient->Invalidate();
	}
	return false;
}

void FCrowdEventPlacementMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	if (!bHoverValid)
	{
		return;
	}

	// Anteprima: il marcatore, l'anello e il raggio di percezione dell'evento che comparirà.
	const FVector Base = HoverLocation + FVector(0.0, 0.0, 5.0);
	const FLinearColor Color = FLinearColor::Green;
	const double RingRadiusM = UCrowdEventsLibrary::RingRadius(PendingPreset.Params.Field, 1.0, 1.0);
	DrawCircle(PDI, Base, FVector::XAxisVector, FVector::YAxisVector, Color, 40.0, 24, SDPG_Foreground, 3.f);
	DrawCircle(PDI, Base, FVector::XAxisVector, FVector::YAxisVector, Color, RingRadiusM * MetersToUnits, 64, SDPG_Foreground, 3.f);
	DrawCircle(PDI, Base, FVector::XAxisVector, FVector::YAxisVector, Color, PendingPreset.Params.PerceptionRadiusM * MetersToUnits, 96, SDPG_Foreground, 1.f);
}

#undef LOCTEXT_NAMESPACE
