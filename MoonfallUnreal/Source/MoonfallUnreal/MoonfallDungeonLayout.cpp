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

 // Hard lifetime budget. Visiting other chambers cannot raise it above six;
 // excess chambers still count as visited rather than retrying forever.
 const int32 Remaining = FMath::Max(0, 6 - SpawnedEncounterCount);
 if (Remaining == 0) return true;
 // Check global live population only on chamber entry, never every Tick.
 // This protects slower Windows PCs when multiple dungeons remain visited.
 constexpr int32 MaxLiveWorldEnemies = 60;
 int32 AliveWorldEnemies = 0;
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* Other = *It;
  if (!IsValid(Other) || Other->IsActorBeingDestroyed()) continue;
  if (FMath::IsFinite(Other->Health) && Other->Health <= 0.f) continue;
  if (++AliveWorldEnemies >= MaxLiveWorldEnemies) return false;
 }
 FRandomStream EncounterRandom(
   GetEffectiveLayoutSeed() ^ (ChamberIndex * 19001 + 0x53A9));
 const bool bEliteEncounter = EncounterRandom.RandRange(0, 4) == 0;
 const int32 Requested = bEliteEncounter ? 1
   : FMath::Clamp(EnemiesPerCombatChamber, 1, 2);
 const int32 Count = FMath::Min(Remaining,
   FMath::Min(Requested, MaxLiveWorldEnemies - AliveWorldEnemies));
 int32 Spawned = 0;
 for (int32 Index = 0; Index < Count; ++Index) {
  const float Side = Index == 0 ? -200.f : 200.f;
  const float OffsetX = EncounterRandom.FRandRange(-150.f, 150.f);
  const FVector Position = GetActorTransform().TransformPosition(
    FVector(ChamberIndex * ChamberLength + OffsetX, Side, 100.f));
  const FTransform SpawnTransform(GetActorRotation(), Position);
  AMoonfallEnemy* Enemy = GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
    AMoonfallEnemy::StaticClass(), SpawnTransform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
  if (!IsValid(Enemy)) continue;
  // Set difficulty BEFORE BeginPlay performs the standard player-level scaling.
  Enemy->bElite = bEliteEncounter;
  Enemy->bBoss = false;
  Enemy->Health = bEliteEncounter ? 145.f : 75.f;
  Enemy->AttackDamage = bEliteEncounter ? 11.f : 7.f;
  Enemy->ChaseSpeed = bEliteEncounter ? 165.f : 185.f;
  Enemy->Tags.AddUnique(FName(TEXT("MoonfallDungeonEncounter")));
  Enemy->FinishSpawning(SpawnTransform);
  // Do not count or retain actors whose deferred spawn was rejected.
  // A valid but rejected actor must be cleaned up before another attempt.
  if (!IsValid(Enemy)) continue;
  if (Enemy->IsActorBeingDestroyed()) {
   Enemy->Destroy();
   continue;
  }
  ++Spawned;
 }
 if (Spawned == 0) return false;
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
 // Count all living world enemies once, on first final-room entry only.
 // Bosses cannot bypass the same global cap imposed on combat chambers.
 constexpr int32 MaxLiveWorldEnemies = 60;
 int32 AliveWorldEnemies = 0;
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* Other = *It;
  if (!IsValid(Other) || Other->IsActorBeingDestroyed()) continue;
  if (FMath::IsFinite(Other->Health) && Other->Health <= 0.f) continue;
  if (++AliveWorldEnemies >= MaxLiveWorldEnemies) return false;
 }
 // Position the boss away from the centerline where the player enters.
 const FVector Position = GetActorTransform().TransformPosition(
   FVector((ChamberCount - 1) * ChamberLength + 270.f, 190.f, 100.f));
 const FTransform SpawnTransform(GetActorRotation(), Position);
 AMoonfallEnemy* Boss = GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
   AMoonfallEnemy::StaticClass(), SpawnTransform, this, nullptr,
   ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
 if (!IsValid(Boss)) return false;
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
 if (!IsValid(Boss) || Boss->IsActorBeingDestroyed()) return false;
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
