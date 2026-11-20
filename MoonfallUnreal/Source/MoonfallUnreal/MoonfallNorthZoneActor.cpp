#include "MoonfallNorthZoneActor.h"
#include "MoonfallNorthZoneGreybox.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallNorthZoneActor::AMoonfallNorthZoneActor() {
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("NorthernZoneRoot")));
 ZoneFloors = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NorthernZoneFloors"));
 ZoneFloors->SetupAttachment(GetRootComponent());
 ZoneFloors->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ZoneFloors->SetCollisionResponseToAllChannels(ECR_Block);
 ZoneSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NorthernZoneSign"));
 ZoneSign->SetupAttachment(GetRootComponent());
 ZoneSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ZoneSign->SetGenerateOverlapEvents(false);
 ZoneSign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 ZoneSign->SetWorldSize(75.f);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) ZoneFloors->SetStaticMesh(Cube.Object);
}

void AMoonfallNorthZoneActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 if (!IsValid(ZoneFloors) || !IsValid(ZoneSign)) return;
 ZoneFloors->ClearInstances();
 ZoneSign->SetText(FText::GetEmpty());
 if (!MoonfallNorth::ValidZone(ProvinceIndex) || !ZoneFloors->GetStaticMesh()) return;
 const auto Plan=MoonfallNorth::PlanZoneGreybox(ProvinceIndex);
 if (!MoonfallNorth::ValidZoneGreybox(Plan)) return;
 for (int32 Index=0; Index<Plan.Count; ++Index) {
  const auto& Part=Plan.Pieces[Index];
  // Default engine cube is 100 cm wide. Top of each floor is at Z=0.
  ZoneFloors->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(static_cast<float>(Part.Xcm),static_cast<float>(Part.Ycm),-20.f),
    FVector(static_cast<float>(Part.HalfXcm)/50.f,
      static_cast<float>(Part.HalfYcm)/50.f, 0.4f)));
 }
 const auto& Province=MoonfallNorth::Zones[ProvinceIndex];
 ZoneSign->SetText(FText::FromString(FString::Printf(
  TEXT("%s\\nHUB: %s\\nDUNGEON: %s"),
  UTF8_TO_TCHAR(Province.Name), UTF8_TO_TCHAR(Province.Hub),
  UTF8_TO_TCHAR(Province.Dungeon))));
 ZoneSign->SetRelativeLocation(FVector(0.f,0.f,200.f));
}

FVector AMoonfallNorthZoneActor::GetDungeonApproachWorldLocation() const {
 return GetActorTransform().TransformPosition(FVector(0.f,4200.f,100.f));
}
FVector AMoonfallNorthZoneActor::GetSouthernEntranceWorldLocation() const {
 return GetActorTransform().TransformPosition(FVector(0.f,-3800.f,100.f));
}
