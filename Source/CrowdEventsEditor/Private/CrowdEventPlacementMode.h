#pragma once

#include "CoreMinimal.h"
#include "CrowdEventTypes.h"
#include "EdMode.h"

class ACrowdEvent;
struct FViewportCursorLocation;

/**
 * Modalità di piazzamento: scelto un preset nel pannello, il clic successivo nel viewport
 * mette l'evento sul pavimento in quel punto. Esc annulla.
 */
class FCrowdEventPlacementMode : public FEdMode
{
public:
	static const FEditorModeID ModeId;

	/** Entra in modalità piazzamento con il preset dato. */
	static void Begin(const FCrowdEventPresetData& Preset, TFunction<void(ACrowdEvent*)> OnPlaced);
	static void Cancel();
	static bool IsPlacing();

	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool UsesToolkits() const override { return false; }
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override { return true; }

private:
	/** Cerca il pavimento lungo il raggio del cursore; falso se il punto non è valido. */
	static bool FindFloor(UWorld* World, const FViewportCursorLocation& Cursor, FVector& OutLocation);

	FCrowdEventPresetData PendingPreset;
	TFunction<void(ACrowdEvent*)> PlacedCallback;
	FVector HoverLocation = FVector::ZeroVector;
	bool bHoverValid = false;
};
