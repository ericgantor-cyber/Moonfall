#include "MoonfallGroundLoot.h"
#include "MoonfallItemQuality.h"
#include "MoonfallHeroPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

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
 PickupDetailA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupDetailA"));
 PickupDetailA->SetupAttachment(SceneRoot);
 PickupDetailB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupDetailB"));
 PickupDetailB->SetupAttachment(SceneRoot);
 ItemLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ItemLabel"));
 ItemLabel->SetupAttachment(SceneRoot);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Sphere.Succeeded()) PickupMesh->SetStaticMesh(Sphere.Object);
 if (Cylinder.Succeeded()) RarityBeacon->SetStaticMesh(Cylinder.Object);
 if (Cube.Succeeded()) {
  PickupDetailA->SetStaticMesh(Cube.Object);
  PickupDetailB->SetStaticMesh(Cube.Object);
 }
 PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RarityBeacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 PickupDetailA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 PickupDetailB->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 PickupDetailA->SetCastShadow(false);
 PickupDetailB->SetCastShadow(false);
 PickupDetailA->SetHiddenInGame(true);
 PickupDetailB->SetHiddenInGame(true);
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
 // Never overwrite an existing or collected drop.
 if (ItemCode != 0 || bCollected || IsActorBeingDestroyed()) return;
 // Reject malformed spawned loot instead of leaving an uncollectable actor.
 if (InItemCode <= 0) { Destroy(); return; }
 const int32 Affix = InItemCode / 100000;
 const int32 Slot = (InItemCode / 10000) % 10;
 const int32 Rarity = (InItemCode / 1000) % 10;
 const int32 Tier = InItemCode % 1000;
 if (Affix < 0 || Affix > 3 || Slot < 0 || Slot > 2 ||
     Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25 ||
     (Rarity == 0 && Affix != 0) ||
     InItemCode != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier)
 {
  Destroy();
  return;
 }
 ItemCode = InItemCode;
 // Slot changes silhouette without changing the item's code, drop location,
 // pickup collision rules, persistent ID, rarity, or seed-dependent roll.
 // Use engine-supplied primitives so art remains visible before texture import.
 // Reuse constructor-provided meshes; no disk lookups in InitializeLoot.
 UStaticMesh* GearCube = PickupDetailA->GetStaticMesh();
 UStaticMesh* GearSphere = PickupMesh->GetStaticMesh();
 if (Slot == 0) {
  // Long blade with a crossguard and a bottom grip.
  if (GearCube) PickupMesh->SetStaticMesh(GearCube);
  PickupMesh->SetRelativeLocation(FVector(0.f,0.f,78.f));
  PickupMesh->SetRelativeScale3D(FVector(0.10f,0.09f,0.67f));
  PickupMesh->SetRelativeRotation(FRotator(0.f,0.f,-16.f));
  PickupDetailA->SetRelativeLocation(FVector(0.f,0.f,50.f));
  PickupDetailA->SetRelativeScale3D(FVector(0.45f,0.10f,0.09f));
  PickupDetailB->SetRelativeLocation(FVector(0.f,0.f,30.f));
  PickupDetailB->SetRelativeScale3D(FVector(0.10f,0.10f,0.33f));
 } else if (Slot == 1) {
  // Broad breastplate plus offset shoulder pauldrons.
  if (GearCube) PickupMesh->SetStaticMesh(GearCube);
  PickupMesh->SetRelativeLocation(FVector(0.f,0.f,49.f));
  PickupMesh->SetRelativeScale3D(FVector(0.43f,0.28f,0.52f));
  PickupDetailA->SetRelativeLocation(FVector(-29.f,0.f,70.f));
  PickupDetailA->SetRelativeScale3D(FVector(0.25f,0.33f,0.19f));
  PickupDetailB->SetRelativeLocation(FVector(29.f,0.f,70.f));
  PickupDetailB->SetRelativeScale3D(FVector(0.25f,0.33f,0.19f));
 } else {
  // Gem in the center of two angled, suspended chain links.
  if (GearSphere) PickupMesh->SetStaticMesh(GearSphere);
  PickupMesh->SetRelativeLocation(FVector(0.f,0.f,63.f));
  PickupMesh->SetRelativeScale3D(FVector(0.25f,0.13f,0.32f));
  PickupDetailA->SetRelativeLocation(FVector(-22.f,0.f,89.f));
  PickupDetailA->SetRelativeScale3D(FVector(0.12f,0.10f,0.35f));
  PickupDetailA->SetRelativeRotation(FRotator(0.f,-30.f,0.f));
  PickupDetailB->SetRelativeLocation(FVector(22.f,0.f,89.f));
  PickupDetailB->SetRelativeScale3D(FVector(0.12f,0.10f,0.35f));
  PickupDetailB->SetRelativeRotation(FRotator(0.f,30.f,0.f));
 }
 // Visual variation is bounded and derived only from validated item data.
 // Collision stays disabled and pickup distances remain unchanged.
 const float SizeAccent = 1.f + 0.004f * FMath::Clamp(Tier - 1, 0, 24);
 PickupMesh->SetRelativeScale3D(PickupMesh->GetRelativeScale3D() * SizeAccent);
 PickupDetailA->SetRelativeScale3D(PickupDetailA->GetRelativeScale3D() * SizeAccent);
 PickupDetailB->SetRelativeScale3D(PickupDetailB->GetRelativeScale3D() * SizeAccent);
 PickupDetailA->SetHiddenInGame(!IsValid(PickupDetailA->GetStaticMesh()));
 PickupDetailB->SetHiddenInGame(!IsValid(PickupDetailB->GetStaticMesh()));
 // Rarity changes beacon width/height, while text remains the accessible cue.
 const float BeaconWidth = 0.08f + 0.012f * Rarity;
 RarityBeacon->SetRelativeScale3D(FVector(BeaconWidth,BeaconWidth,
   1.25f + 0.11f * Rarity));
 const TCHAR* SlotNames[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
 const TCHAR* TraitNames[] = {TEXT(""), TEXT("Force "), TEXT("Ward "), TEXT("Focus ")};
 // Match the HUD's quality order. Emerald is Moonfall-exclusive.
 const FColor QualityColor = MoonfallItemQuality::WorldColor(Rarity);
 ItemLabel->SetText(FText::FromString(FString::Printf(TEXT("%s %s%s  T%d  [F / X]"),
   MoonfallItemQuality::Name(Rarity), TraitNames[Affix], SlotNames[Slot], Tier)));
 ItemLabel->SetTextRenderColor(QualityColor);
 // Colored beacon and object communicate rarity without transient effects or tick.
 if (UMaterialInstanceDynamic* BeaconMaterial = RarityBeacon->CreateAndSetMaterialInstanceDynamic(0))
  BeaconMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(QualityColor));
 if (UMaterialInstanceDynamic* PickupMaterial = PickupMesh->CreateAndSetMaterialInstanceDynamic(0)) {
  PickupMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(QualityColor));
  if (IsValid(PickupDetailA) && IsValid(PickupDetailA->GetStaticMesh()))
   PickupDetailA->SetMaterial(0, PickupMaterial);
  if (IsValid(PickupDetailB) && IsValid(PickupDetailB->GetStaticMesh()))
   PickupDetailB->SetMaterial(0, PickupMaterial);
 }
}

void AMoonfallGroundLoot::SetPersistentDropId(const FGuid& InId)
{
 // World-drop identity is immutable and must never replace dungeon reward
 // coordinates. The existing spawn/restore call sites assign the ID first.
 if (!InId.IsValid() || bCollected || IsActorBeingDestroyed() || ItemCode != 0
     || PersistentDropId.IsValid() || RewardZone != INDEX_NONE
     || RewardDungeon != INDEX_NONE || RewardChamber != INDEX_NONE) return;
 PersistentDropId = InId;
}

void AMoonfallGroundLoot::InitializeDungeonReward(int32 Zone, int32 Dungeon, int32 Chamber)
{
 // The persisted reward identity is write-once. A second initialization
 // must not redirect an already spawned dungeon drop to a different claim.
 if (bCollected || IsActorBeingDestroyed() || RewardZone != INDEX_NONE
     || RewardDungeon != INDEX_NONE || RewardChamber != INDEX_NONE
     || PersistentDropId.IsValid()) return;
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || Chamber <= 0 || Chamber >= 13 || ItemCode <= 0) return;
 RewardZone = Zone;
 RewardDungeon = Dungeon;
 RewardChamber = Chamber;
}

bool AMoonfallGroundLoot::TryCollect()
{
 if (bCollected || ItemCode <= 0 || IsActorBeingDestroyed()) return false;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 // Direct Blueprint/actor calls must respect the same modal and defeat
 // restrictions as the manual/automatic pickup search.
 if (!IsValid(Hero) || !GetWorld() || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->bOptionsOpen || Hero->bInventoryOpen || Hero->bAtlasOpen
     || Hero->bCharacterStatsOpen || Hero->bResidentContextOpen) return false;
 // Enforce pickup reachability at the item boundary as well as in the hero's
 // candidate search. Blueprint or future callers must not collect distant loot.
 const FVector HeroPosition = Hero->GetActorLocation();
 const FVector LootPosition = GetActorLocation();
 // Extreme finite positions can still overflow the squared-distance result.
 // Never allow a nonfinite distance to bypass the pickup boundary.
 if (HeroPosition.ContainsNaN() || LootPosition.ContainsNaN()) return false;
 const float DistanceSq = FVector::DistSquared2D(HeroPosition, LootPosition);
 if (!FMath::IsFinite(DistanceSq) ||
     FMath::Abs(LootPosition.Z - HeroPosition.Z) > 150.f ||
     DistanceSq > FMath::Square(250.f))
  return false;
 FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MoonfallGroundLootCollectSight), false);
 SightParams.AddIgnoredActor(Hero);
 SightParams.AddIgnoredActor(this);
 FHitResult SightHit;
 if (GetWorld()->LineTraceSingleByChannel(SightHit,
     HeroPosition + FVector(0.f, 0.f, 40.f),
     LootPosition + FVector(0.f, 0.f, 40.f),
     ECC_Visibility, SightParams)) return false;
 const bool bDungeonTreasure = RewardZone != INDEX_NONE;
 const bool bSaved = bDungeonTreasure
   ? Hero->CollectDungeonTreasureCode(ItemCode, RewardZone, RewardDungeon, RewardChamber)
   : (PersistentDropId.IsValid()
      ? Hero->CollectPersistedGroundEquipmentCode(ItemCode, PersistentDropId)
      : Hero->CollectGroundEquipmentCode(ItemCode));
 if (!bSaved) return false;
 bCollected = true;
 Destroy();
 return true;
}
