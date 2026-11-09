#include "MoonfallDungeonLayout.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallDungeonLayout::AMoonfallDungeonLayout()
{
 PrimaryActorTick.bCanEverTick = false;
 USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("DungeonRoot"));
 SetRootComponent(Root);
 Floors = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DungeonFloors"));
 Floors->SetupAttachment(Root);
 Walls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DungeonWalls"));
 Walls->SetupAttachment(Root);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  Floors->SetStaticMesh(Cube.Object);
  Walls->SetStaticMesh(Cube.Object);
 }
 // Reuse the engine cube mesh with per-instance transforms, not a mesh actor
 // for each chamber. Floors and walls participate in regular collision.
 Floors->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Floors->SetCollisionResponseToAllChannels(ECR_Block);
 Walls->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Walls->SetCollisionResponseToAllChannels(ECR_Block);
 HazardPlates = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HazardPlates"));
 HazardPlates->SetupAttachment(Root);
 if (Cube.Succeeded()) HazardPlates->SetStaticMesh(Cube.Object);
 HazardPlates->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 // Side traps are authored as fixed overlap volumes, never solid obstructions.
 HazardTriggerA = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerA"));
 HazardTriggerB = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerB"));
 HazardTriggerC = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerC"));
 UBoxComponent* Triggers[] = {HazardTriggerA, HazardTriggerB, HazardTriggerC};
 for (int32 Index = 0; Index < 3; ++Index) {
  UBoxComponent* Trigger = Triggers[Index];
  Trigger->SetupAttachment(Root);
  Trigger->InitBoxExtent(FVector(140.f, 130.f, 110.f));
  // Actual placement is set in OnConstruction from a deterministic seed.
  Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
  Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  Trigger->SetGenerateOverlapEvents(true);
 }

 EntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntranceTrigger"));
 EntranceTrigger->SetupAttachment(Root);
 EntranceTrigger->InitBoxExtent(FVector(260.f, 240.f, 160.f));
 EntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 EntranceTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
 EntranceTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 EntranceTrigger->SetGenerateOverlapEvents(true);

 FinalChamberTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("FinalChamberTrigger"));
 FinalChamberTrigger->SetupAttachment(Root);
 FinalChamberTrigger->InitBoxExtent(FVector(260.f, 240.f, 160.f));
 FinalChamberTrigger->SetRelativeLocation(FVector((ChamberCount - 1) * ChamberLength, 0.f, 150.f));
 FinalChamberTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 FinalChamberTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
 FinalChamberTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 FinalChamberTrigger->SetGenerateOverlapEvents(true);
}

int32 AMoonfallDungeonLayout::GetEffectiveLayoutSeed() const
{
 return LayoutSeed != 0 ? LayoutSeed
      : 1337 + FMath::Clamp(ZoneIndex, 0, 5) * 101
             + FMath::Clamp(DungeonIndex, 0, 4) * 37;
}

void AMoonfallDungeonLayout::BuildHazardChambers(int32 OutChambers[3]) const
{
 if (!bRandomizeChambers) {
  OutChambers[0] = 3; OutChambers[1] = 7; OutChambers[2] = 11;
  return;
 }
 FRandomStream Stream(GetEffectiveLayoutSeed());
 // Unique chamber indices, excluding the entrance and final boss room.
 int32 Candidates[12];
 for (int32 Index = 0; Index < 12; ++Index) Candidates[Index] = Index + 1;
 for (int32 Index = 0; Index < 3; ++Index) {
  const int32 Choice = Stream.RandRange(Index, 11);
  Swap(Candidates[Index], Candidates[Choice]);
  OutChambers[Index] = Candidates[Index];
 }
}

int32 AMoonfallDungeonLayout::GetRandomizedHazardChamber(int32 HazardIndex) const
{
 if (HazardIndex < 0 || HazardIndex >= 3) return -1;
 int32 Chambers[3];
 BuildHazardChambers(Chambers);
 return Chambers[HazardIndex];
}

void AMoonfallDungeonLayout::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 Floors->ClearInstances();
 Walls->ClearInstances();
 HazardPlates->ClearInstances();
 // Seeded dungeon variants preserve every connecting doorway and the boss room.
 int32 HazardChambers[3];
 BuildHazardChambers(HazardChambers);
 UBoxComponent* Triggers[] = {HazardTriggerA, HazardTriggerB, HazardTriggerC};
 FRandomStream LayoutRandom(GetEffectiveLayoutSeed() ^ 0x5A17);
 for (int32 Index = 0; Index < 3; ++Index) {
  const int32 Chamber = HazardChambers[Index];
  const float Side = LayoutRandom.RandRange(0, 1) == 0 ? -265.f : 265.f;
  Triggers[Index]->SetRelativeLocation(FVector(Chamber * ChamberLength, Side, 95.f));
  HazardPlates->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(Chamber * ChamberLength, Side, 8.f),
      FVector(2.8f, 2.6f, 0.1f)));
 }
 // Fourteen connected rooms; openings in every cross-wall allow normal
 // player movement from chamber 1 to 14 without a loading transition.
 for (int32 Chamber = 0; Chamber < ChamberCount; ++Chamber) {
  const float X = Chamber * ChamberLength;
  Floors->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(X, 0.f, -20.f), FVector(ChamberLength / 100.f,
                                   ChamberWidth / 100.f, 0.4f)));
  for (const float Side : {-1.f, 1.f}) {
   Walls->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(X, Side * ChamberWidth * 0.5f, 150.f),
      FVector(ChamberLength / 100.f, 0.4f, 3.f)));
   // Pillars break up the corridor silhouette without blocking the doorway.
   // Variations are on the wall-side only; the central route stays clear.
   const float OffsetJitter = bRandomizeChambers ? LayoutRandom.FRandRange(-55.f, 55.f) : 0.f;
   for (const float Offset : {-420.f, 420.f})
    Walls->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(X + Offset + OffsetJitter, Side * 335.f, 150.f),
      FVector(0.7f, 0.7f, 3.f)));
  }
 }
 // Dividers at room boundaries leave a 360-unit central passage.
 // Final room exit remains closed; entrance is open to the authored world.
 for (int32 Chamber = 1; Chamber < ChamberCount; ++Chamber) {
  const float X = (Chamber - 0.5f) * ChamberLength;
  for (const float Side : {-1.f, 1.f})
   Walls->AddInstance(FTransform(FRotator::ZeroRotator,
     FVector(X, Side * 315.f, 150.f), FVector(0.4f, 2.7f, 3.f)));
 }
 // Cap the final arena; no accidental fall-through into unbuilt terrain.
 Walls->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector((ChamberCount - 0.5f) * ChamberLength, 0.f, 150.f),
    FVector(0.4f, ChamberWidth / 100.f, 3.f)));
}

void AMoonfallDungeonLayout::BeginPlay()
{
 Super::BeginPlay();
 if (ZoneIndex < 0 || ZoneIndex >= AMoonfallHeroPawn::DungeonZoneCount
     || DungeonIndex < 0 || DungeonIndex >= AMoonfallHeroPawn::DungeonsPerZone) {
  EntranceTrigger->SetGenerateOverlapEvents(false);
  FinalChamberTrigger->SetGenerateOverlapEvents(false);
  HazardTriggerA->SetGenerateOverlapEvents(false);
  HazardTriggerB->SetGenerateOverlapEvents(false);
  HazardTriggerC->SetGenerateOverlapEvents(false);
  return;
 }
 EntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallDungeonLayout::HandleEntrance);
 FinalChamberTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallDungeonLayout::HandleFinalChamber);
 HazardTriggerA->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallDungeonLayout::HandleHazardOverlap);
 HazardTriggerB->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallDungeonLayout::HandleHazardOverlap);
 HazardTriggerC->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallDungeonLayout::HandleHazardOverlap);
}

void AMoonfallDungeonLayout::HandleEntrance(
 UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()) return;
 Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex);
}

void AMoonfallDungeonLayout::HandleFinalChamber(
 UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (bFinalChamberNotified || !IsValid(Hero) || !Hero->IsPlayerControlled()
     || Hero->Health <= 0.f) return;
 bFinalChamberNotified = true;
 OnFinalChamberEntered(Hero);
}

void AMoonfallDungeonLayout::HandleHazardOverlap(
 UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 // The hazard is an avoidable side path, not a wave, timer, or unavoidable
 // corridor tax. Only the living player hero can be hurt.
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled() || Hero->Health <= 0.f
     || !GetWorld() || !FMath::IsFinite(HazardDamage) || HazardDamage <= 0.f)
  return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now < NextHazardDamageAt) return;
 NextHazardDamageAt = Now + FMath::Clamp(HazardCooldownSeconds, 0.5f, 20.f);
 Hero->ApplyMoonfallDamage(HazardDamage);
}
