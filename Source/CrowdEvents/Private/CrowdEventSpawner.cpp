#include "CrowdEventSpawner.h"

#include "CrowdEvent.h"
#include "CrowdEventsLibrary.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ACrowdEventSpawner::ACrowdEventSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	RootComponent = Area;
	Area->InitBoxExtent(FVector(1000.0, 50.0, 50.0));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetCanEverAffectNavigation(false);

	Reach = CreateDefaultSubobject<UBoxComponent>(TEXT("Reach"));
	Reach->SetupAttachment(RootComponent);
	Reach->InitBoxExtent(FVector(1900.0, 500.0, 200.0));
	Reach->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Reach->SetCanEverAffectNavigation(false);
}

void ACrowdEventSpawner::BeginPlay()
{
	Super::BeginPlay();
	Stream.Initialize(Seed != 0 ? Seed : FMath::Rand());
	WaitS = RandomIn(FirstDelayS);
}

void ACrowdEventSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bSpawnEvents)
	{
		return;
	}
	if (Current && !IsValid(Current))
	{
		// Tolto da fuori, per esempio a mano durante la simulazione.
		Current = nullptr;
		WaitS = RandomIn(PauseS);
	}
	if (Current)
	{
		if (Current->IsEventActive())
		{
			return;
		}
		// I pedoni che reagivano tornano allo stato 0 al prossimo giro del gestore.
		Current->Destroy();
		Current = nullptr;
		WaitS = RandomIn(PauseS);
	}
	WaitS -= DeltaSeconds;
	if (WaitS <= 0.0)
	{
		SpawnEvent();
	}
}

void ACrowdEventSpawner::SpawnEvent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Extent = Area->GetUnscaledBoxExtent();
	const FVector LocalPoint(RandomIn(FVector2D(-Extent.X, Extent.X)), RandomIn(FVector2D(-Extent.Y, Extent.Y)), -Extent.Z);
	const FTransform SpawnTransform(Area->GetComponentTransform().TransformPosition(LocalPoint));

	ACrowdEvent* Event = World->SpawnActorDeferred<ACrowdEvent>(ACrowdEvent::StaticClass(), SpawnTransform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Event)
	{
		return;
	}
	Event->EventName = FText::FromString(TEXT("Training event"));
	Event->Params.EventClass = EventClass;
	Event->Params.Field = UCrowdEventsLibrary::SampleRingField(Stream, MinRingM, MaxRingM);
	Event->Params.PerceptionRadiusM = RandomIn(PerceptionRadiusM);
	Event->Params.DurationS = RandomIn(DurationS);
	Event->Params.Salience = 1.0;
	Event->bAllowLingerInTraining = bAllowLinger;
	Event->bLimitReach = true;
	Event->ReachBox = Reach->Bounds.GetBox();
	Event->FinishSpawning(SpawnTransform);

	Current = Event;
	++SpawnedCount;
}

double ACrowdEventSpawner::RandomIn(const FVector2D& Range) const
{
	return Range.X + (Range.Y - Range.X) * static_cast<double>(Stream.GetFraction());
}
