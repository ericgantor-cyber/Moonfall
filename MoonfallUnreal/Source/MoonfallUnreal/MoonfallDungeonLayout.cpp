#include "MoonfallDungeonLayout.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "MoonfallGroundLoot.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
 // Visual-only instanced-mesh distance fade. Distant chambers do not need
 // rendering from the arena or neighboring zones. Keep collision intact:
 // actors, traps, loot and world traversal are never disabled by culling.
 Floors->SetCullDistances(24000, 34000);
 Walls->SetCullDistances(18000, 28000);
 HazardPlates->SetCullDistances(12000, 22000);
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

int32 AMoonfallDungeonLayout::GetBonusGroupSize(int32 GroupIndex) const
{
 return bBonusEncounterActive && GroupIndex >= 0 && GroupIndex < 3
   ? BonusGroupSizes[GroupIndex] : 0;
}

int32 AMoonfallDungeonLayout::GetBonusGroupChamber(int32 GroupIndex) const
{
 return bBonusEncounterActive && GroupIndex >= 0 && GroupIndex < 3
   ? BonusGroupChambers[GroupIndex] : INDEX_NONE;
}

void AMoonfallDungeonLayout::PlanBonusEncounter()
{
 if (bBonusEncounterRolled) return;
 bBonusEncounterRolled = true;
 // Isolate the encounter roll from chamber RNG so treasure/elite outcomes
 // cannot be changed by bonus group planning.
 FRandomStream BonusRandom(GetEffectiveLayoutSeed() ^ 0x237B51);
 bBonusEncounterActive = BonusRandom.FRand() < 0.25f;
 if (bBonusEncounterActive) {
  for (int32 Group = 0; Group < 3; ++Group)
   BonusGroupSizes[Group] = BonusRandom.RandRange(5, 11);
  // Assign exactly three distinct interior targets without changing the
  // original chamber types, hazards, loot rolls, or boss encounter.
  // Prefer non-combat rooms so a bonus pack cannot collide with the
  // normal six-enemy chamber formation. Never replace hazards or bosses.
  // Every 14-chamber dungeon has twelve interior rooms, only three hazards.
  int32 Assigned = 0;
  // Shuffle eligible rooms with a separate seeded stream. Preserve chamber
  // generation and reward RNG, and prefer non-combat rooms before combat.
  FRandomStream PlacementRandom(GetEffectiveLayoutSeed() ^ 0x6D92F1);
  for (const int32 PreferredType : {4, 3, 2, 1}) {
   TArray<int32> Candidates;
   for (int32 Chamber = 1; Chamber < ChamberCount - 1; ++Chamber)
    if (GetChamberType(Chamber) == PreferredType) Candidates.Add(Chamber);
   for (int32 Index = 0; Index < Candidates.Num(); ++Index) {
    const int32 SwapWith = PlacementRandom.RandRange(Index, Candidates.Num() - 1);
    Candidates.Swap(Index, SwapWith);
   }
   for (const int32 Chamber : Candidates) {
    if (Assigned >= 3) break;
    BonusGroupChambers[Assigned++] = Chamber;
   }
   if (Assigned >= 3) break;
  }
  if (Assigned < 3) {
   // Fail closed rather than selecting a hazard or final boss chamber.
   bBonusEncounterActive = false;
   for (int32 Group = 0; Group < 3; ++Group) BonusGroupSizes[Group] = 0;
  }
 }
 // Planning never spawns an enemy or changes the existing chamber budget.
 OnBonusEncounterPlanned(bBonusEncounterActive,
   BonusGroupSizes[0], BonusGroupSizes[1], BonusGroupSizes[2]);
}

bool AMoonfallDungeonLayout::TrySpawnBonusGroup(
 AMoonfallHeroPawn* Hero, int32 ChamberIndex)
{
 if (!bBonusEncounterActive || !GetWorld() || !IsValid(Hero)
     || !Hero->IsPlayerControlled() || !FMath::IsFinite(Hero->Health)
     || Hero->Health <= 0.f || Hero->bOptionsOpen || Hero->bInventoryOpen
     || Hero->bCharacterStatsOpen || Hero->bResidentContextOpen)
  return false;
 int32 Group = INDEX_NONE;
 for (int32 Candidate = 0; Candidate < 3; ++Candidate) {
  if (BonusGroupChambers[Candidate] == ChamberIndex) {
   Group = Candidate;
   break;
  }
 }
 if (Group == INDEX_NONE) return false;
 const uint8 GroupBit = static_cast<uint8>(1u << Group);
 if ((SpawnedBonusGroupsMask & GroupBit) != 0u) return true;
 const int32 GroupSize = BonusGroupSizes[Group];
 const int32 Level = GetOrLockEncounterLevel(Hero);
 if (GroupSize < 5 || GroupSize > 11 || Level <= 0
     || ChamberIndex <= 0 || ChamberIndex >= ChamberCount - 1
     || GetChamberType(ChamberIndex) == 5) return false;

 UWorld* World = GetWorld();
 // Never add a bonus crowd during this dungeon's boss fight.
 // Other dungeons in the shared six-zone world remain independent.
 for (TActorIterator<AMoonfallEnemy> It(World); It; ++It) {
  if (IsValid(*It) && !It->IsActorBeingDestroyed()
      && It->GetOwner() == this && It->bBoss
      && FMath::IsFinite(It->Health) && It->Health > 0.f)
   return false;
 }
 TArray<AMoonfallEnemy*> GroupActors;
 GroupActors.Reserve(GroupSize);
 FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(MoonfallDungeonBonusSpawn), false);
 // Keep dungeon walls and floors in collision checks, not ignored.
 bool bAllSpawned = true;
 // Two side lanes of up to six, all within the existing 1200 x 900
 // room. Never place a mob on the central doorway/overlap path (Y=0):
 // the entering hero otherwise blocks an entire pack on every retry.
 for (int32 Member = 0; Member < GroupSize; ++Member) {
  const int32 Column = Member / 2;
  const float X = (static_cast<float>(Column) - 2.5f) * 120.f;
  const float Y = (Member % 2 == 0) ? -240.f : 240.f;
  bool bSpawnedMember = false;
  // Bounded offsets stay inside the room's side lanes, away from Y=0.
  // Successful attempts stop immediately; never exceed GroupSize actors.
  for (int32 Attempt = 0; Attempt < 3; ++Attempt) {
   const float RetryX = Attempt == 0 ? 0.f : (Attempt == 1 ? 45.f : -45.f);
   const FVector Position = GetActorTransform().TransformPosition(
     FVector(ChamberIndex * ChamberLength + X + RetryX, Y, 110.f));
   if (World->OverlapBlockingTestByChannel(Position, FQuat::Identity,
         ECC_Pawn, FCollisionShape::MakeSphere(55.f), CollisionParams))
    continue;
   const FTransform SpawnTransform(GetActorRotation(), Position);
   AMoonfallEnemy* Enemy = World->SpawnActorDeferred<AMoonfallEnemy>(
     AMoonfallEnemy::StaticClass(), SpawnTransform, this, nullptr,
     ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
   if (!IsValid(Enemy)) continue;
   Enemy->bBoss = false;
   Enemy->bElite = Group > 0 && Member == GroupSize - 1;
   Enemy->EncounterLevelOverride = Level;
   Enemy->DungeonChamberIndex = ChamberIndex;
   Enemy->Health = Enemy->bElite ? 145.f : 75.f;
   Enemy->AttackDamage = Enemy->bElite ? 11.f : 7.f;
   Enemy->Tags.AddUnique(FName(TEXT("MoonfallDungeonEncounter")));
   Enemy->Tags.AddUnique(FName(TEXT("MoonfallDungeonBonus")));
   if (Member == 0) Enemy->Tags.AddUnique(FName(TEXT("DungeonScout")));
   Enemy->FinishSpawning(SpawnTransform);
   if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed()) {
    if (IsValid(Enemy)) Enemy->Destroy();
    continue;
   }
   GroupActors.Add(Enemy);
   bSpawnedMember = true;
   break;
  }
  if (!bSpawnedMember) {
   bAllSpawned = false;
   break;
  }
 }
 if (!bAllSpawned || GroupActors.Num() != GroupSize) {
  // A rejected member must never leave a partial 5..11 pack or consume
  // the one-shot token; re-entering this room can retry once space clears.
  for (AMoonfallEnemy* Enemy : GroupActors)
   if (IsValid(Enemy)) Enemy->Destroy();
  return false;
 }
 SpawnedBonusGroupsMask |= GroupBit;
 OnBonusGroupSpawned(Group, ChamberIndex, GroupSize);
 return true;
}

int32 AMoonfallDungeonLayout::GetOrLockEncounterLevel(const AMoonfallHeroPawn* Hero)
{
 if (LockedEncounterLevel <= 0 && IsValid(Hero) && Hero->IsPlayerControlled()
     && FMath::IsFinite(Hero->Health) && Hero->Health > 0.f) {
  const int32 MinLevel = FMath::Clamp(MinimumDungeonLevel, 1, 100);
  const int32 MaxLevel = FMath::Clamp(MaximumDungeonLevel, MinLevel, 100);
  LockedEncounterLevel = FMath::Clamp(Hero->Level, MinLevel, MaxLevel);
  PlanBonusEncounter();
 }
 return LockedEncounterLevel;
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
 if (!Hero->HasDiscoveredDungeon(ZoneIndex, DungeonIndex)
     && !Hero->DiscoverDungeonEntrance(ZoneIndex, DungeonIndex)) return;
 GetOrLockEncounterLevel(Hero);
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
 // A failed spawn must not consume the final chamber event. Player can
 // retreat and retry when collision or the global population cap clears.
 if (!Hero->HasDefeatedDungeonBoss(ZoneIndex, DungeonIndex)
     && !StartFinalBossChamber(Hero)) return;
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
  // Bonus encounters retry on re-entry if any formation slot was blocked.
  // Normal chamber encounters keep their original one-shot completion mask.
  TrySpawnBonusGroup(Hero, ChamberIndex);
  if ((EnteredChambersMask & Bit) != 0u) return;
  const int32 ChamberType = GetChamberType(ChamberIndex);
  // Failed spawn leaves the chamber retryable on a later overlap.
  if (ChamberType == 1 && !StartCombatChamber(Hero, ChamberIndex)) return;
  if (ChamberType == 2 && !SpawnTreasureChamber(Hero, ChamberIndex)) return;
  EnteredChambersMask |= Bit;
  OnChamberEntered(Hero, ChamberIndex, ChamberType);
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
 if (!FMath::IsFinite(Now) || Now < 0.f) return;
 // A corrupt cooldown or world-time reset must not disable the trap.
 const bool bClockRewound = FMath::IsFinite(LastHazardObservedTime)
   && LastHazardObservedTime >= 0.f && Now < LastHazardObservedTime;
 LastHazardObservedTime = Now;
 const float Remaining = NextHazardDamageAt - Now;
 if (!bClockRewound && FMath::IsFinite(Remaining)
     && Remaining > 0.f && Remaining <= 20.f) return;
 const float Next = Now + FMath::Clamp(HazardCooldownSeconds, 0.5f, 20.f);
 if (!FMath::IsFinite(Next) || Next <= Now) return;
 NextHazardDamageAt = Next;
 Hero->ApplyMoonfallDamage(FMath::Clamp(HazardDamage, 0.f, 100.f));
}

bool AMoonfallDungeonLayout::StartCombatChamber(AMoonfallHeroPawn* Hero, int32 ChamberIndex)
{
 if (!GetWorld() || !IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || ChamberIndex <= 0 || ChamberIndex >= ChamberCount - 1
     || GetChamberType(ChamberIndex) != 1) return false;

 // Budget six enemies for each interior chamber, so earlier encounters do not
 // exhaust the dungeon budget before later rooms. Elites still spawn alone.
 const int32 MaxDungeonEncounterSpawns = (ChamberCount - 2) * 6;
 const int32 Remaining = FMath::Max(0, MaxDungeonEncounterSpawns - SpawnedEncounterCount);
 if (Remaining == 0) return true;
 // Encounter size is authored, not reduced by momentary frame rate.
 // Distant dungeon enemies already use lower-frequency AI simulation.
 FRandomStream EncounterRandom(
   GetEffectiveLayoutSeed() ^ (ChamberIndex * 19001 + 0x53A9));
 const bool bEliteEncounter = EncounterRandom.RandRange(0, 4) == 0;
 const int32 Requested = bEliteEncounter ? 1
   : EncounterRandom.RandRange(5, FMath::Clamp(EnemiesPerCombatChamber, 5, 6));
 // Never silently downgrade a regular 5-6-mob chamber to a smaller group.
 if (Remaining < Requested) return false;
 const int32 Count = Requested;
 const int32 EncounterLevel = GetOrLockEncounterLevel(Hero);
 if (EncounterLevel <= 0) return false;
 TArray<AMoonfallEnemy*> SpawnedActors;
 SpawnedActors.Reserve(Count);
 int32 Spawned = 0;
 for (int32 Index = 0; Index < Count; ++Index) {
  // Two lanes of three enemies, spaced to fit within the chamber floor.
  const float Side = (Index % 2 == 0) ? -200.f : 200.f;
  const float OffsetX = (Index / 2 - 1) * 275.f
    + EncounterRandom.FRandRange(-40.f, 40.f);
  // Retry only slots blocked by collision, using bounded alternate points.
  // Keep the same seed, per-room target, and maximum total enemy count.
  for (int32 Attempt = 0; Attempt < 3; ++Attempt) {
   const float RetryX = Attempt == 0 ? 0.f : (Attempt == 1 ? 70.f : -70.f);
   const float RetrySide = Attempt == 0 ? 0.f : (Attempt == 1 ? -70.f : 70.f);
   const FVector Position = GetActorTransform().TransformPosition(
     FVector(ChamberIndex * ChamberLength + OffsetX + RetryX, Side + RetrySide, 100.f));
  const FTransform SpawnTransform(GetActorRotation(), Position);
  AMoonfallEnemy* Enemy = GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
    AMoonfallEnemy::StaticClass(), SpawnTransform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
  if (!IsValid(Enemy)) continue;
  // Set difficulty BEFORE BeginPlay performs the standard player-level scaling.
  Enemy->bElite = bEliteEncounter;
  Enemy->bBoss = false;
  Enemy->EncounterLevelOverride = EncounterLevel;
  Enemy->Health = bEliteEncounter ? 145.f : 75.f;
  Enemy->AttackDamage = bEliteEncounter ? 11.f : 7.f;
  Enemy->ChaseSpeed = bEliteEncounter ? 165.f : 185.f;
  // Record chamber before BeginPlay to isolate attack slots across rooms.
  Enemy->DungeonChamberIndex = ChamberIndex;
  Enemy->Tags.AddUnique(FName(TEXT("MoonfallDungeonEncounter")));
  // The first surviving regular spawn in a multi-mob chamber is the scout.
  // A rejected first spawn must not leave the chamber without a scout.
  // Elite encounters retain their original behavior.
  if (!bEliteEncounter && Spawned == 0 && Count > 1)
   Enemy->Tags.AddUnique(FName(TEXT("DungeonScout")));
  Enemy->FinishSpawning(SpawnTransform);
  // Do not count or retain actors whose deferred spawn was rejected.
  // A valid but rejected actor must be cleaned up before another attempt.
  if (!IsValid(Enemy)) continue;
  if (Enemy->IsActorBeingDestroyed()) {
   Enemy->Destroy();
   continue;
  }
  SpawnedActors.Add(Enemy);
  ++Spawned;
  break; // Successful slot: never produce more than Count enemies.
  }
 }
 // Commit the chamber only when every requested actor survived spawning.
 // A blocked slot must not permanently turn a 5-6-mob room into a partial room.
 if (Spawned != Count) {
  for (AMoonfallEnemy* Enemy : SpawnedActors)
   if (IsValid(Enemy)) Enemy->Destroy();
  return false;
 }
 SpawnedEncounterCount += Spawned;
 OnChamberEncounterSpawned(Hero, ChamberIndex, Spawned, bEliteEncounter);
 return true;
}

bool AMoonfallDungeonLayout::SpawnTreasureChamber(
 AMoonfallHeroPawn* Hero, int32 ChamberIndex)
{
 if (!GetWorld() || !IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || ZoneIndex < 0 || ZoneIndex >= 6 || DungeonIndex < 0 || DungeonIndex >= 5
     || ChamberIndex <= 0 || ChamberIndex >= ChamberCount - 1
     || GetChamberType(ChamberIndex) != 2) return false;
 // Only the saved reward record decides whether an item can be produced.
 const int32 Code = Hero->GetOrCreateDungeonTreasureCode(
   ZoneIndex, DungeonIndex, ChamberIndex, GetEffectiveLayoutSeed());
 if (Code <= 0) {
  // Claimed treasure is a successful, empty room; do not duplicate a pickup.
  return Hero->GetDungeonTreasureState(ZoneIndex, DungeonIndex, ChamberIndex) == -1;
 }
 if (SpawnedTreasure[ChamberIndex].IsValid()) return true;
 const FVector Position = GetActorTransform().TransformPosition(
   FVector(ChamberIndex * ChamberLength, 185.f, 55.f));
 FActorSpawnParameters Params;
 Params.Owner = this;
 Params.SpawnCollisionHandlingOverride =
   ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 AMoonfallGroundLoot* Loot = GetWorld()->SpawnActor<AMoonfallGroundLoot>(
   AMoonfallGroundLoot::StaticClass(), Position, GetActorRotation(), Params);
 if (!IsValid(Loot)) return false;
 Loot->InitializeLoot(Code);
 if (Loot->GetItemCode() != Code) {
  Loot->Destroy();
  return false;
 }
 Loot->InitializeDungeonReward(ZoneIndex, DungeonIndex, ChamberIndex);
 // Initialization can reject invalid/reused reward identities; do not
 // register that actor as dungeon treasure or expose it as ordinary loot.
 if (!Loot->HasDungeonRewardIdentity(ZoneIndex, DungeonIndex, ChamberIndex)) {
  Loot->Destroy();
  return false;
 }
 SpawnedTreasure[ChamberIndex] = Loot;
 OnDungeonTreasureSpawned(Hero, ChamberIndex, Code);
 return true;
}

bool AMoonfallDungeonLayout::StartFinalBossChamber(AMoonfallHeroPawn* Hero)
{
 if (bFinalChamberNotified || !GetWorld() || !IsValid(Hero)
     || !Hero->IsPlayerControlled() || !FMath::IsFinite(Hero->Health)
     || Hero->Health <= 0.f
     || ZoneIndex < 0 || ZoneIndex >= 6
     || DungeonIndex < 0 || DungeonIndex >= 5) return false;
 // Existing saves must not respawn permanently defeated bosses.
 if (Hero->HasDefeatedDungeonBoss(ZoneIndex, DungeonIndex)) return true;
 // Bosses are not withheld by the regular-room performance spawn budget.
 // Their existing collision and save guards remain in force.
 // Final-room collision can reject a single authored spawn point.
 // Try at most two other nearby positions without moving the entry gate.
 // Keep the original Z and Nyra divine identifiers/art completely untouched.
 // The generic existing boss logic supplies phases, combat, XP and gear drops.
 // Six distinct regional styles use the existing melee, combo, parry,
 // posture and boss-phase implementation. Each of the five dungeons in a
 // region also rises in rank. No new effects or untested attack timers.
 struct FDungeonBossStyle {
  const TCHAR* Family;
  const TCHAR* Title;
  float Health;
  float Damage;
  float ChaseSpeed;
  float AttackRange;
  float AttackCooldown;
  float Posture;
 };
 static const FDungeonBossStyle Styles[6] = {
  {TEXT("Ashen"), TEXT("Ashen Warden"), 480.f, 17.f, 145.f, 170.f, 1.65f, 155.f},
  {TEXT("Verdant"), TEXT("Thorn Sentinel"), 420.f, 16.f, 190.f, 155.f, 1.15f, 115.f},
  {TEXT("Glacial"), TEXT("Frostbound Colossus"), 575.f, 22.f, 120.f, 190.f, 1.9f, 185.f},
  {TEXT("Storm"), TEXT("Tempest Stalker"), 375.f, 15.f, 225.f, 145.f, 1.0f, 105.f},
  {TEXT("Foundry"), TEXT("Forged Juggernaut"), 625.f, 21.f, 135.f, 175.f, 1.75f, 200.f},
  {TEXT("Eclipse"), TEXT("Eclipse Harbinger"), 510.f, 20.f, 195.f, 180.f, 1.3f, 165.f}
 };
 const FDungeonBossStyle& Style = Styles[ZoneIndex];
 const int32 Rank = DungeonIndex + 1;
 const float RankScale = 1.f + 0.12f * DungeonIndex;
 for (int32 Attempt = 0; Attempt < 3; ++Attempt) {
  // Probe independent axes, not two points on the same diagonal.
  // A blocked doorway edge should not invalidate every fallback.
  const float BossOffsetX = Attempt == 1 ? -160.f : 0.f;
  const float BossOffsetY = Attempt == 2 ? -160.f : 0.f;
  const FVector Position = GetActorTransform().TransformPosition(
    FVector((ChamberCount - 1) * ChamberLength + 270.f + BossOffsetX,
      190.f + BossOffsetY, 100.f));
  const FTransform SpawnTransform(GetActorRotation(), Position);
  AMoonfallEnemy* Boss = GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
    AMoonfallEnemy::StaticClass(), SpawnTransform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
  if (!IsValid(Boss)) continue;
 Boss->bElite = true;
 Boss->bBoss = true;
 Boss->RaceId = (ZoneIndex * 5 + DungeonIndex) % 12;
 Boss->DungeonBossFamily = FName(Style.Family);
 Boss->DungeonBossRank = Rank;
 Boss->DungeonBossTitle = FText::FromString(
   FString::Printf(TEXT("%s | Rank %d"), Style.Title, Rank));
 Boss->Health = Style.Health * RankScale;
 Boss->AttackDamage = Style.Damage * (1.f + 0.07f * DungeonIndex);
 Boss->ChaseSpeed = Style.ChaseSpeed;
 Boss->AttackRange = Style.AttackRange;
 Boss->AttackCooldown = Style.AttackCooldown;
 Boss->MaxPosture = Style.Posture * RankScale;
 Boss->Tags.AddUnique(FName(TEXT("MoonfallDungeonBoss")));
 Boss->FinishSpawning(SpawnTransform);
 if (!IsValid(Boss) || Boss->IsActorBeingDestroyed()) continue;
 Boss->OnDungeonBossIdentityReady(
   Boss->DungeonBossFamily, Boss->DungeonBossTitle, Boss->DungeonBossRank);
 // Blueprint identity events are synchronous and may destroy the actor.
 // Do not consume the room trigger or announce a nonexistent boss.
 if (!IsValid(Boss) || Boss->IsActorBeingDestroyed()) return false;
 OnDungeonBossSpawned(Hero, ZoneIndex, DungeonIndex);
 // Blueprint encounter events can also reject/destroy the spawn.
 // Keep the final-chamber overlap retryable in that case.
 return IsValid(Boss) && !Boss->IsActorBeingDestroyed();
 }
 return false; // All collision-safe spawn positions were unavailable.
}
