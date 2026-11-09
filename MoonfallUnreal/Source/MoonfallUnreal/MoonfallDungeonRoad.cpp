#include "MoonfallDungeonRoad.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallDungeonRoad::AMoonfallDungeonRoad()
{
 PrimaryActorTick.bCanEverTick = false;
 USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoadRoot"));
 SetRootComponent(Root);
 PathFloors = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WorldRoadFloors"));
 PathFloors->SetupAttachment(Root);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) PathFloors->SetStaticMesh(Cube.Object);
 PathFloors->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 PathFloors->SetCollisionResponseToAllChannels(ECR_Block);
}

FVector AMoonfallDungeonRoad::GetDungeonOrigin(int32 ZoneIndex, int32 DungeonIndex)
{
 // Six distinct regions, five side-by-side dungeons per region. Every
 // chamber's length points south, avoiding overlap between dungeons.
 if (ZoneIndex < 0 || ZoneIndex >= AMoonfallHeroPawn::DungeonZoneCount
     || DungeonIndex < 0 || DungeonIndex >= AMoonfallHeroPawn::DungeonsPerZone)
  return FVector::ZeroVector;
 return FVector(ZoneIndex * ZoneSpacing + DungeonIndex * DungeonSpacing,
                DungeonStartY, 0.f);
}

void AMoonfallDungeonRoad::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 PathFloors->ClearInstances();
 // One contiguous ground-level east-west spine bridges the arena gateway
 // and each dungeon's north entrance. No physics bodies per corridor tile.
 const float FinalX = GetDungeonOrigin(AMoonfallHeroPawn::DungeonZoneCount - 1,
                                      AMoonfallHeroPawn::DungeonsPerZone - 1).X;
 const float SegmentLength = 3000.f;
 const int32 SegmentCount = FMath::CeilToInt((FinalX + 1200.f) / SegmentLength);
 for (int32 Segment = 0; Segment < SegmentCount; ++Segment) {
  const float StartX = Segment * SegmentLength - 600.f;
  PathFloors->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(StartX + SegmentLength * 0.5f, -1950.f, -20.f),
      FVector(SegmentLength / 100.f, 15.f, 0.4f)));
 }
 // Short broad entry path through the 400-unit arena gate. The raised
 // flooring has exactly the same top Z as dungeon floors, so no step.
 PathFloors->AddInstance(FTransform(FRotator::ZeroRotator,
     FVector(0.f, -1250.f, -20.f), FVector(4.f, 12.f, 0.4f)));
}
