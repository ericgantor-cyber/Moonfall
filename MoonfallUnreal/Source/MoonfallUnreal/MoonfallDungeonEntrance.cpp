#include "MoonfallDungeonEntrance.h"
#include "MoonfallHeroPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallDungeonEntrance::AMoonfallDungeonEntrance()
{
 PrimaryActorTick.bCanEverTick = false;
 DiscoveryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("DiscoveryTrigger"));
 SetRootComponent(DiscoveryTrigger);
 DiscoveryTrigger->InitBoxExtent(FVector(180.f, 180.f, 160.f));
 DiscoveryTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 DiscoveryTrigger->SetCollisionObjectType(ECC_WorldDynamic);
 DiscoveryTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
 DiscoveryTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 DiscoveryTrigger->SetGenerateOverlapEvents(true);

 EntranceMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EntranceMarker"));
 EntranceMarker->SetupAttachment(DiscoveryTrigger);
 EntranceMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 EntranceMarker->SetRelativeScale3D(FVector(1.8f, 0.35f, 2.7f));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) EntranceMarker->SetStaticMesh(Cube.Object);
}

int32 AMoonfallDungeonEntrance::GetDungeonEntranceKey() const
{
 if (ZoneIndex < 0 || ZoneIndex >= 6 || DungeonIndex < 0 || DungeonIndex >= 5)
  return INDEX_NONE;
 return ZoneIndex * 5 + DungeonIndex;
}

void AMoonfallDungeonEntrance::SetEntranceEnabled(bool bEnabled)
{
 bEntranceEnabled = bEnabled;
 // Visited entrances remain disabled to prevent event/reward duplication.
 if (IsValid(DiscoveryTrigger))
  DiscoveryTrigger->SetGenerateOverlapEvents(
    bEntranceEnabled && !bVisitedThisSession && GetDungeonEntranceKey() != INDEX_NONE);
}

void AMoonfallDungeonEntrance::BeginPlay()
{
 Super::BeginPlay();
 // Do not register invalid or disabled entrances. Runtime re-enabling is
 // supported through SetEntranceEnabled without re-binding the delegate.
 DiscoveryTrigger->OnComponentBeginOverlap.AddDynamic(
  this, &AMoonfallDungeonEntrance::OnEntranceOverlap);
 SetEntranceEnabled(bEntranceEnabled);
}

void AMoonfallDungeonEntrance::OnEntranceOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 if (bVisitedThisSession || !bEntranceEnabled || GetDungeonEntranceKey() == INDEX_NONE) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) return;
 bVisitedThisSession = true;
 DiscoveryTrigger->SetGenerateOverlapEvents(false);
 OnEntranceVisited(Hero, ZoneIndex, DungeonIndex);
}
