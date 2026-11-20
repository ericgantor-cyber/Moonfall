#include "MoonfallNorthSanctumActor.h"
#include "MoonfallNorthSanctumGeometry.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallNorthSanctumActor::AMoonfallNorthSanctumActor() {
 PrimaryActorTick.bCanEverTick = false;
 USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("SanctumRoot"));
 SetRootComponent(Root);
 SanctumBlocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SanctumBlocks"));
 SanctumBlocks->SetupAttachment(Root);
 SanctumBlocks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 SanctumBlocks->SetCollisionResponseToAllChannels(ECR_Block);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) SanctumBlocks->SetStaticMesh(Cube.Object);
}

void AMoonfallNorthSanctumActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 if (!IsValid(SanctumBlocks)) return;
 SanctumBlocks->ClearInstances();
 if (DivineSiteIndex < 0 || DivineSiteIndex >= 3 || !SanctumBlocks->GetStaticMesh()) return;
 const auto Plan = MoonfallNorth::PlanSanctum(
   static_cast<MoonfallNorth::EDivineSite>(DivineSiteIndex));
 if (!MoonfallNorth::SafeSanctumPlan(Plan)) return;
 // Engine cube is 100cm. Floors are 40cm thick with top at Z=0.
 // Walls rise 240cm, starting at the same floor elevation.
 for (int32 Index=0; Index<5; ++Index) {
  const auto& Part = Plan.Parts[Index];
  const bool bFloor = Index < 2;
  const float HalfHeight = bFloor ? 20.f : 120.f;
  const FVector Center(static_cast<float>(Part.CenterXcm),
    static_cast<float>(Part.CenterYcm), bFloor ? -HalfHeight : HalfHeight);
  const FVector Scale(static_cast<float>(Part.HalfXcm)/50.f,
    static_cast<float>(Part.HalfYcm)/50.f, HalfHeight/50.f);
  SanctumBlocks->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
 }
}

FVector AMoonfallNorthSanctumActor::GetBossMarkerWorldLocation() const {
 if (DivineSiteIndex<0 || DivineSiteIndex>=3) return GetActorLocation();
 const auto Plan=MoonfallNorth::PlanSanctum(
   static_cast<MoonfallNorth::EDivineSite>(DivineSiteIndex));
 return GetActorTransform().TransformPosition(FVector(
   static_cast<float>(Plan.Parts[5].CenterXcm),
   static_cast<float>(Plan.Parts[5].CenterYcm), 100.f));
}

FVector AMoonfallNorthSanctumActor::GetEntranceWorldLocation() const {
 return GetActorTransform().TransformPosition(FVector::ZeroVector);
}
