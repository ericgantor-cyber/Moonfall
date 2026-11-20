#include "MoonfallNorthContinentActor.h"
#include "MoonfallNorthContinentLayout.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallNorthContinentActor::AMoonfallNorthContinentActor() {
 PrimaryActorTick.bCanEverTick=false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("NorthernContinentRoot")));
 ProvincePads=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvincePads"));
 ProvincePads->SetupAttachment(GetRootComponent());
 ProvinceRoads=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvinceRoads"));
 ProvinceRoads->SetupAttachment(GetRootComponent());
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  ProvincePads->SetStaticMesh(Cube.Object);
  ProvinceRoads->SetStaticMesh(Cube.Object);
 }
 ProvincePads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ProvinceRoads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ProvincePads->SetCollisionResponseToAllChannels(ECR_Block);
 ProvinceRoads->SetCollisionResponseToAllChannels(ECR_Block);
}
void AMoonfallNorthContinentActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 if (!IsValid(ProvincePads) || !IsValid(ProvinceRoads)) return;
 ProvincePads->ClearInstances();
 ProvinceRoads->ClearInstances();
 if (!ProvincePads->GetStaticMesh() || !ProvinceRoads->GetStaticMesh()
     || !MoonfallNorth::ValidContinentCenters()) return;
 // One static plaza per zone, all with the same walkable Z=0 top face.
 for (int32 Zone=0; Zone<MoonfallNorth::ZoneCount; ++Zone) {
  const auto& P=MoonfallNorth::ContinentCenters[Zone];
  const float Half=static_cast<float>(MoonfallNorth::ProvincePadHalfSizeCm);
  ProvincePads->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),-20.f),
    FVector(Half/50.f,Half/50.f,0.4f)));
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
}
FVector AMoonfallNorthContinentActor::GetProvinceCenterWorldLocation(int32 ProvinceIndex) const {
 if (!MoonfallNorth::ValidZone(ProvinceIndex)) return GetActorLocation();
 const auto& P=MoonfallNorth::ContinentCenters[ProvinceIndex];
 return GetActorTransform().TransformPosition(
  FVector(static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),100.f));
}
