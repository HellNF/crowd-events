#include "CrowdEvent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

bool ACrowdEvent::bDrawDebug = true;

namespace
{
	const double MetersToUnits = 100.0;

	FColor ClassColor(ECrowdEventClass EventClass)
	{
		switch (EventClass)
		{
		case ECrowdEventClass::Illness: return FColor(255, 170, 0);
		case ECrowdEventClass::Danger: return FColor(230, 40, 40);
		default: return FColor(40, 170, 255);
		}
	}
}

ACrowdEvent::ACrowdEvent()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Raggio 0,4 m, da terra a 1,8 m: i raggi dei pedoni partono a 90 cm (specifica §3.3).
	Marker = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->InitCapsuleSize(40.f, 90.f);
	Marker->SetRelativeLocation(FVector(0.0, 0.0, 90.0));
	Marker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Marker->SetCollisionResponseToAllChannels(ECR_Ignore);
	Marker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Marker->SetCanEverAffectNavigation(false);

	// Figura provvisoria: un disco a terra, finché non ci sono i manichini.
	Figure = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(RootComponent);
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Figure->SetCanEverAffectNavigation(false);
	Figure->SetRelativeScale3D(FVector(0.8, 0.8, 0.05));
	Figure->SetRelativeLocation(FVector(0.0, 0.0, 2.5));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Figure->SetStaticMesh(CylinderMesh.Object);
	}

	EventName = FText::FromString(TEXT("Event"));
}

void ACrowdEvent::ApplyPreset(const FCrowdEventPresetData& Preset)
{
	EventName = Preset.DisplayName;
	Params = Preset.Params;
}

double ACrowdEvent::GetRingRadiusM() const
{
	return UCrowdEventsLibrary::RingRadius(Params.Field, 1.0, 1.0);
}

double ACrowdEvent::GetRemainingTimeS() const
{
	if (Params.DurationS <= 0.0)
	{
		return -1.0;
	}
	return FMath::Max(0.0, Params.DurationS - ElapsedS);
}

void ACrowdEvent::EndEvent()
{
	if (bEnded)
	{
		return;
	}
	bEnded = true;
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Figure->SetVisibility(false);
}

void ACrowdEvent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Il tempo dell'evento scorre solo in simulazione, non nell'editor fermo.
	if (World->IsGameWorld() && !bEnded)
	{
		ElapsedS += DeltaSeconds;
		if (Params.DurationS > 0.0 && ElapsedS >= Params.DurationS)
		{
			EndEvent();
		}
	}

	if (bDrawDebug && !bEnded)
	{
		const FVector Center = GetActorLocation() + FVector(0.0, 0.0, 5.0);
		const FColor Color = ClassColor(Params.EventClass);
		// Anello pieno, raggio di percezione più sottile.
		DrawDebugCircle(World, Center, static_cast<float>(GetRingRadiusM() * MetersToUnits), 64, Color, false, -1.f, 0, 6.f,
			FVector::XAxisVector, FVector::YAxisVector, false);
		DrawDebugCircle(World, Center, static_cast<float>(Params.PerceptionRadiusM * MetersToUnits), 96, Color, false, -1.f, 0, 1.5f,
			FVector::XAxisVector, FVector::YAxisVector, false);
	}
}
