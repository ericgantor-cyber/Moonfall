#include "MoonfallDungeonLayout.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
 HazardPlates = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DungeonHazardPlates"));
 HazardPlates->SetupAttachment(Root);
 HazardPlates->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  Floors->SetStaticMesh(Cube.Object);
  Walls->SetStaticMesh(Cube.Object);
  HazardPlates->SetStaticMesh(Cube.Object);
 }
 // Instanced geometry supports physical collision without one actor per tile.
 Floors->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Floors->SetCollisionResponseToAllChannels(ECR_Block);
 Walls->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Walls->SetCollisionResponseToAllChannels(ECR_Block);

 HazardTriggerA = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerA"));
 HazardTriggerB = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerB"));
 HazardTriggerC = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardTriggerC"));
 UBoxComponent* HazardTriggers[] = {HazardTriggerA, HazardTriggerB, HazardTriggerC};
 for (UBoxComponent* Trigger : HazardTriggers) {
  Trigger->SetupAttachment(Root);
  Trigger->InitBoxExtent(FVector(90.f, 90.f, 110.f));
  Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
  Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  Trigger->SetGenerateOverlapEvents(true);
 }

 EntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntranceTrigger"));
 EntranceTrigger->SetupAttachment(Root);
 EntranceTrigger->InitBoxExtent(FVector(260.f, 240.f, 160.f));
 EntranceTrigger->SetRelativeLocation(FVector::ZeroVector);
 EntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 EntranceTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
 EntranceTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 EntranceTrigger->SetGenerateOverlapEvents(true);

 FinalChamberTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("FinalChamberTrigger"));
 FinalChamberTrigger->SetupAttachment(Root);
 FinalChamberTrigger->InitBoxExtent(FVector(260.f, 240.f, 160.f));
 FinalChamberTrigger->SetRelativeLocation(
   FVector((ChamberCount - 1) * ChamberLength, 0.f, 150.f));
 FinalChamberTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 FinalChamberTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
 FinalChamberTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 FinalChamberTrigger->SetGenerateOverlapEvents(true);
 // Twelve interior room triggers, never entrance or final boss. Component
 // indices correspond to chamber 1..12 and do not Tick.
 for (int32 Chamber = 1; Chamber < ChamberCount - 1; ++Chamber) {
  UBoxComponent* Trigger = CreateDefaultSubobject<UBoxComponent>(
    FName(*FString::Printf(TEXT("ChamberTrigger_%02d"), Chamber)));
  Trigger->SetupAttachment(Root);
  Trigger->InitBoxExtent(FVector(220.f, 165.f, 140.f));
  Trigger->SetRelativeLocation(FVector(Chamber * ChamberLength, 0.f, 145.f));
  Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
  Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  Trigger->SetGenerateOverlapEvents(true);
  ChamberTriggers.Add(Trigger);
 }
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
 // Unbiased partial Fisher-Yates of the twelve interior rooms.
 FRandomStream Stream(GetEffectiveLayoutSeed());
 int32 Candidates[12];
 for (int32 Index = 0; Index < 12; ++Index) Candidates[Index] = Index + 1;
 for (int32 Index = 0; Index < 3; ++Index) {
  const int32 Choice = Stream.RandRange(Index, 11);
  Swap(Candidates[Index], Candidates[Choice]);
  OutChambers[Index] = Candidates[Index];
 }
}

int32 AMoonfallDungeonLayout::GetChamberType(int32 ChamberIndex) const
{
 if (ChamberIndex < 0 || ChamberIndex >= ChamberCount) return INDEX_NONE;
 if (ChamberIndex == 0) return 0;
 if (ChamberIndex == ChamberCount - 1) return 6;
 int32 Hazards[3];
 BuildHazardChambers(Hazards);
 for (const int32 Hazard : Hazards)
  if (Hazard == ChamberIndex) return 5;
 if (!bRandomizeChambers) return 1 + ((ChamberIndex - 1) % 4);
 // Independent per-room roll: a new archetype does not re-seed hazards.
 FRandomStream RoomRandom(GetEffectiveLayoutSeed() ^ (ChamberIndex * 7919 + 0x41CD));
 return RoomRandom.RandRange(1, 4);
}

void AMoonfallDungeonLayout::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 Floors->ClearInstances();
 Walls->ClearInstances();
 HazardPlates->ClearInstances();
 int32 HazardChambers[3];
 BuildHazardChambers(HazardChambers);
 FRandomStream LayoutRandom(GetEffectiveLayoutSeed() ^ 0x5A17);
 UBoxComponent* HazardTriggers[] = {HazardTriggerA, HazardTriggerB, HazardTriggerC};
 for (int32 Index = 0; Index < 3; ++Index) {
  const int32 Chamber = HazardChambers[Index];
  // Center 315, radius 90: closest point 225, outside the +/-180 center path.
  const float Side = LayoutRandom.RandRange(0, 1) == 0 ? -315.f : 315.f;
  HazardTriggers[Index]->SetRelativeLocation(
    FVector(Chamber * ChamberLength, Side, 105.f));
  HazardPlates->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(Chamber * ChamberLength, Side, 8.f),
    FVector(1.8f, 1.8f, 0.1f)));
 }
 // All fourteen chambers share a continuous floor and central door opening.
 for (int32 Chamber = 0; Chamber < ChamberCount; ++Chamber) {
  const float X = Chamber * ChamberLength;
  Floors->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(X, 0.f, -20.f),
    FVector(ChamberLength / 100.f, ChamberWidth / 100.f, 0.4f)));
  for (const float Side : {-1.f, 1.f}) {
   Walls->AddInstance(FTransform(FRotator::ZeroRotator,
     FVector(X, Side * ChamberWidth * 0.5f, 150.f),
     FVector(ChamberLength / 100.f, 0.4f, 3.f)));
   for (const float Offset : {-420.f, 420.f})
    Walls->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(X + Offset, Side * 335.f, 150.f), FVector(0.7f, 0.7f, 3.f)));
  }
 }
 // Dividers leave a 360-unit center path without teleporting between rooms.
 for (int32 Chamber = 1; Chamber < ChamberCount; ++Chamber) {
  const float X = (Chamber - 0.5f) * ChamberLength;
  for (const float Side : {-1.f, 1.f})
   Walls->AddInstance(FTransform(FRotator::ZeroRotator,
     FVector(X, Side * 315.f, 150.f), FVector(0.4f, 2.7f, 3.f)));
 }
 Walls->AddInstance(FTransform(FRotator::ZeroRotator,
   FVector((ChamberCount - 0.5f) * ChamberLength, 0.f, 150.f),
   FVector(0.4f, ChamberWidth / 100.f, 3.f)));
}

void AMoonfallDungeonLayout::BeginPlay()
{
 Super::BeginPlay();
 if (ZoneIndex < 0 || ZoneIndex >= 6 || DungeonIndex < 0 || DungeonIndex >= 5) {
  EntranceTrigger->SetGenerateOverlapEvents(false);
  FinalChamberTrigger->SetGenerateOverlapEvents(false);
  for (UBoxComponent* Trigger : {HazardTriggerA, HazardTriggerB, HazardTriggerC})
   if (IsValid(Trigger)) Trigger->SetGenerateOverlapEvents(false);
  for (UBoxComponent* Trigger : ChamberTriggers)
   if (IsValid(Trigger)) Trigger->SetGenerateOverlapEvents(false);
  return;
 }
 EntranceTrigger->OnComponentBeginOverlap.AddDynamic(
   this, &AMoonfallDungeonLayout::HandleEntrance);
 FinalChamberTrigger->OnComponentBeginOverlap.AddDynamic(
   this, &AMoonfallDungeonLayout::HandleFinalChamber);
 for (UBoxComponent* Trigger : {HazardTriggerA, HazardTriggerB, HazardTriggerC})
  if (IsValid(Trigger))
   Trigger->OnComponentBeginOverlap.AddDynamic(
     this, &AMoonfallDungeonLayout::HandleHazardOverlap);
 for (UBoxComponent* Trigger : ChamberTriggers)
  if (IsValid(Trigger))
   Trigger->OnComponentBeginOverlap.AddDynamic(
     this, &AMoonfallDungeonLayout::HandleChamberOverlap);
}

void AMoonfallDungeonLayout::HandleEntrance(
 UPrimitiveComponent* Overlapped, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) return;
 // First visit saves through the V8 ledger; later visits are already known.
 if (!Hero->HasDiscoveredDungeon(ZoneIndex, DungeonIndex))
  Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex);
}

void AMoonfallDungeonLayout::HandleFinalChamber(
 UPrimitiveComponent* Overlapped, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (bFinalChamberNotified || !IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) return;
 // Invalid/uninitialized save state does not permit a final-room event.
 if (!Hero->HasDiscoveredDungeon(ZoneIndex, DungeonIndex)
     && !Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex)) return;
 bFinalChamberNotified = true;
 FinalChamberTrigger->SetGenerateOverlapEvents(false);
 OnFinalChamberEntered(Hero);
}

void AMoonfallDungeonLayout::HandleChamberOverlap(
 UPrimitiveComponent* Overlapped, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) return;
 // Require a durable entrance discovery before announcing room events.
 if (!Hero->HasDiscoveredDungeon(ZoneIndex, DungeonIndex)
     && !Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex)) return;
 for (int32 Index = 0; Index < ChamberTriggers.Num(); ++Index) {
  if (ChamberTriggers[Index] != Overlapped) continue;
  const int32 ChamberIndex = Index + 1;
  const uint16 Bit = static_cast<uint16>(1u << ChamberIndex);
  if ((EnteredChambersMask & Bit) != 0u) return;
  EnteredChambersMask |= Bit;
  OnChamberEntered(Hero, ChamberIndex, GetChamberType(ChamberIndex));
  return;
 }
}

void AMoonfallDungeonLayout::HandleHazardOverlap(
 UPrimitiveComponent* Overlapped, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 // Only the three bounded side traps may damage a living player.
 if (Overlapped != HazardTriggerA && Overlapped != HazardTriggerB
     && Overlapped != HazardTriggerC) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || !GetWorld() || !FMath::IsFinite(HazardDamage) || HazardDamage <= 0.f
     || !FMath::IsFinite(HazardCooldownSeconds)) return;
 // Never damage a player before their dungeon discovery is durably saved.
 if (!Hero->HasDiscoveredDungeon(ZoneIndex, DungeonIndex)
     && !Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex)) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now < NextHazardDamageAt) return;
 NextHazardDamageAt = Now + FMath::Clamp(HazardCooldownSeconds, 0.5f, 20.f);
 Hero->ApplyMoonfallDamage(FMath::Clamp(HazardDamage, 0.f, 100.f));
}
