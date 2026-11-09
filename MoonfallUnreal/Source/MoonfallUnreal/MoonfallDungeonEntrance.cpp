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

void AMoonfallDungeonEntrance::BeginPlay()
{
 Super::BeginPlay();
 // Invalid map-authored IDs are inert rather than mutating save data.
 if (ZoneIndex < 0 || ZoneIndex >= AMoonfallHeroPawn::DungeonZoneCount
     || DungeonIndex < 0 || DungeonIndex >= AMoonfallHeroPawn::DungeonsPerZone)
 {
  DiscoveryTrigger->SetGenerateOverlapEvents(false);
  return;
 }
 DiscoveryTrigger->OnComponentBeginOverlap.AddDynamic(
     this, &AMoonfallDungeonEntrance::OnEntranceOverlap);
}

void AMoonfallDungeonEntrance::OnEntranceOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()) return;
 Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex);
}
