#include "MoonfallNorthContinentActor.h"
#include "MoonfallNorthContinentLayout.h"
#include "MoonfallNorthLandmarks.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallNorthContinentActor::AMoonfallNorthContinentActor() {
 PrimaryActorTick.bCanEverTick=false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("NorthernContinentRoot")));
 ProvincePads=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvincePads"));
 ProvincePads->SetupAttachment(GetRootComponent());
 ProvinceRoads=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvinceRoads"));
 ProvinceRoads->SetupAttachment(GetRootComponent());
 ProvinceLandmarks=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvinceLandmarks"));
 ProvinceLandmarks->SetupAttachment(GetRootComponent());
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  ProvincePads->SetStaticMesh(Cube.Object);
  ProvinceRoads->SetStaticMesh(Cube.Object);
  ProvinceLandmarks->SetStaticMesh(Cube.Object);
 }
 ProvincePads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ProvinceRoads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ProvincePads->SetCollisionResponseToAllChannels(ECR_Block);
 ProvinceRoads->SetCollisionResponseToAllChannels(ECR_Block);
 // Decorative biome silhouettes must never obstruct navigation or combat.
 ProvinceLandmarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ProvinceLandmarks->SetGenerateOverlapEvents(false);
 // Fixed-size non-ticking sign components. Never create components per frame.
 for (int32 Zone=0; Zone<MoonfallNorth::ZoneCount; ++Zone) {
  UTextRenderComponent* Sign=CreateDefaultSubobject<UTextRenderComponent>(
    FName(*FString::Printf(TEXT("NorthernZoneSign_%d"), Zone)));
  Sign->SetupAttachment(GetRootComponent());
  Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Sign->SetGenerateOverlapEvents(false);
  Sign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  Sign->SetWorldSize(86.f);
  ProvinceSigns.Add(Sign);
 }
}
void AMoonfallNorthContinentActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 if (!IsValid(ProvincePads) || !IsValid(ProvinceRoads) || !IsValid(ProvinceLandmarks)) return;
 ProvincePads->ClearInstances();
 ProvinceRoads->ClearInstances();
 ProvinceLandmarks->ClearInstances();
 for (UTextRenderComponent* Sign : ProvinceSigns)
  if (IsValid(Sign)) Sign->SetText(FText::GetEmpty());
 if (!ProvincePads->GetStaticMesh() || !ProvinceRoads->GetStaticMesh()
     || !ProvinceLandmarks->GetStaticMesh()
     || !MoonfallNorth::ValidContinentCenters()
     || !MoonfallNorth::ValidLandmarks()
     || ProvinceSigns.Num() != MoonfallNorth::ZoneCount) return;
 // One static plaza per zone, all with the same walkable Z=0 top face.
 for (int32 Zone=0; Zone<MoonfallNorth::ZoneCount; ++Zone) {
  const auto& P=MoonfallNorth::ContinentCenters[Zone];
  const float Half=static_cast<float>(MoonfallNorth::ProvincePadHalfSizeCm);
  ProvincePads->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),-20.f),
    FVector(Half/50.f,Half/50.f,0.4f)));
  if (UTextRenderComponent* Sign=ProvinceSigns[Zone]) {
   const auto& Region=MoonfallNorth::Zones[Zone];
   // No quest titles, locked divine sites or unrevealed dungeon names.
   Sign->SetText(FText::FromString(FString::Printf(
     TEXT("%s | LVL %d\n%s"),
     UTF8_TO_TCHAR(Region.Name), Region.SuggestedLevel,
     UTF8_TO_TCHAR(Region.Hub))));
   Sign->SetRelativeLocation(FVector(
     static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),220.f));
  }
 }
 // One road per undirected graph edge: no duplicated bridges or actor ticks.
 for (int32 A=0; A<MoonfallNorth::ZoneCount; ++A) {
  for (int32 B=A+1; B<MoonfallNorth::ZoneCount; ++B) {
   if (!MoonfallNorth::Linked(A,B)) continue;
   const auto& PA=MoonfallNorth::ContinentCenters[A];
   const auto& PB=MoonfallNorth::ContinentCenters[B];
   const float DX=static_cast<float>(PB.Xcm-PA.Xcm);
   const float DY=static_cast<float>(PB.Ycm-PA.Ycm);
   const float Length=FMath::Sqrt(DX*DX+DY*DY);
   if (!FMath::IsFinite(Length) || Length<1.f) continue;
   const FVector Midpoint((PA.Xcm+PB.Xcm)*0.5f,
     (PA.Ycm+PB.Ycm)*0.5f,-20.f);
   const FRotator Rotation(0.f,FMath::RadiansToDegrees(FMath::Atan2(DY,DX)),0.f);
   ProvinceRoads->AddInstance(FTransform(Rotation,Midpoint,
     FVector(Length/100.f,MoonfallNorth::RoadHalfWidthCm/50.f,0.4f)));
  }
 }
 // Fixed decorative silhouettes per biome: 18 non-colliding instances total.
 // Replace these cubes with authored environmental meshes/materials in the editor.
 for (int32 Index=0; Index<MoonfallNorth::LandmarkCount; ++Index) {
  const auto& L=MoonfallNorth::Landmarks[Index];
  const auto& P=MoonfallNorth::ContinentCenters[L.Province];
  const float HalfHeight=static_cast<float>(L.HeightCm)*0.5f;
  ProvinceLandmarks->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(static_cast<float>(P.Xcm+L.OffsetXcm),
      static_cast<float>(P.Ycm+L.OffsetYcm),HalfHeight),
    FVector(static_cast<float>(L.HalfWidthCm)/50.f,
      static_cast<float>(L.HalfWidthCm)/50.f,HalfHeight/50.f)));
 }
}
FVector AMoonfallNorthContinentActor::GetProvinceCenterWorldLocation(int32 ProvinceIndex) const {
 if (!MoonfallNorth::ValidZone(ProvinceIndex)) return GetActorLocation();
 const auto& P=MoonfallNorth::ContinentCenters[ProvinceIndex];
 return GetActorTransform().TransformPosition(
  FVector(static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),100.f));
}
