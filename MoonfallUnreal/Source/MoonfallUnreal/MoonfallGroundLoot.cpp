#include "MoonfallGroundLoot.h"
#include "MoonfallHeroPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

AMoonfallGroundLoot::AMoonfallGroundLoot()
{
 PrimaryActorTick.bCanEverTick = false;
 SetActorEnableCollision(false);
 SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LootRoot"));
 SetRootComponent(SceneRoot);
 PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
 PickupMesh->SetupAttachment(SceneRoot);
 RarityBeacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RarityBeacon"));
 RarityBeacon->SetupAttachment(SceneRoot);
 ItemLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ItemLabel"));
 ItemLabel->SetupAttachment(SceneRoot);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 if (Sphere.Succeeded()) PickupMesh->SetStaticMesh(Sphere.Object);
 if (Cylinder.Succeeded()) RarityBeacon->SetStaticMesh(Cylinder.Object);
 PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RarityBeacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 PickupMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.2f));
 RarityBeacon->SetRelativeLocation(FVector(0.f, 0.f, 170.f));
 RarityBeacon->SetRelativeScale3D(FVector(0.08f, 0.08f, 1.4f));
 ItemLabel->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
 ItemLabel->SetWorldSize(26.f);
 ItemLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 ItemLabel->SetText(FText::FromString(TEXT("LOOT")));
}

void AMoonfallGroundLoot::InitializeLoot(int32 InItemCode)
{
 if (ItemCode != 0 || InItemCode <= 0) return;
 const int32 Affix = InItemCode / 100000;
 const int32 Slot = (InItemCode / 10000) % 10;
 const int32 Rarity = (InItemCode / 1000) % 10;
 const int32 Tier = InItemCode % 1000;
 if (Affix < 0 || Affix > 3 || Slot < 0 || Slot > 2 ||
     Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25 ||
     (Rarity == 0 && Affix != 0) ||
     InItemCode != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier)
  return;
 ItemCode = InItemCode;
 const TCHAR* SlotNames[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
 const TCHAR* QualityNames[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
 const TCHAR* TraitNames[] = {TEXT(""), TEXT("Force "), TEXT("Ward "), TEXT("Focus ")};
 const FColor Colors[] = {FColor(230,230,230), FColor(65,175,255),
   FColor(245,201,63), FColor(183,89,244), FColor(58,214,107),
   FColor(255,132,32)};
 const FColor QualityColor = Colors[Rarity];
 ItemLabel->SetText(FText::FromString(FString::Printf(TEXT("%s %s%s  T%d  [F / X]"),
   QualityNames[Rarity], TraitNames[Affix], SlotNames[Slot], Tier)));
 ItemLabel->SetTextRenderColor(QualityColor);
 // Colored beacon and object communicate rarity without transient effects or tick.
 if (UMaterialInstanceDynamic* BeaconMaterial = RarityBeacon->CreateAndSetMaterialInstanceDynamic(0))
  BeaconMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(QualityColor));
 if (UMaterialInstanceDynamic* PickupMaterial = PickupMesh->CreateAndSetMaterialInstanceDynamic(0))
  PickupMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(QualityColor));
}

bool AMoonfallGroundLoot::TryCollect()
{
 if (bCollected || ItemCode <= 0) return false;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || !Hero->CollectGroundEquipmentCode(ItemCode)) return false;
 bCollected = true;
 Destroy();
 return true;
}
