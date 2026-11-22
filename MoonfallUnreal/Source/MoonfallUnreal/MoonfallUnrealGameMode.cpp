#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallCombatHUD.h"
#include "MoonfallWorldBootstrap.h"
#include "HAL/PlatformTime.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
 HUDClass = AMoonfallCombatHUD::StaticClass();
}

bool AMoonfallUnrealGameMode::GetDungeonEntranceTransform(
 int32 ZoneIndex, int32 DungeonIndex, FTransform& OutTransform) const
{
 if (ZoneIndex < 0 || ZoneIndex >= DungeonZoneCount
     || DungeonIndex < 0 || DungeonIndex >= DungeonsPerZone) return false;
 const int32 Slot = ZoneIndex * DungeonsPerZone + DungeonIndex;
 // Thirty parallel lanes grouped into 6 contiguous regions of 5 dungeons.
 // Each 900-unit-wide layout has a 450-unit gap to its neighbors. Every
 // entrance connects to the same Y=-2500 public road, never a loading screen.
 const float X = (static_cast<float>(Slot) - 14.5f) * DungeonLaneSpacing;
 OutTransform = FTransform(FRotator(0.f, -90.f, 0.f),
   FVector(X, -3100.f, 0.f));
 return true;
}

void AMoonfallUnrealGameMode::BeginPlay()
{
 Super::BeginPlay();
 if (!GetWorld()) return;

 // Preserve existing default behavior, but permit an isolated authored
 // continent or integration test map to omit the legacy combat arena.
 const MoonfallWorldBootstrap::FPolicy BootPolicy{
   bGenerateLegacyArena,bGenerateDungeonWorld};
 if (BootPolicy.SpawnLegacyArena) {
  bool bHasArena = false;
  for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It)
   if (IsValid(*It) && !It->IsActorBeingDestroyed()) {
    bHasArena = true;
    break;
   }
  if (MoonfallWorldBootstrap::ShouldSpawnArena(BootPolicy,bHasArena))
   GetWorld()->SpawnActor<AMoonfallArena>(
     FVector::ZeroVector, FRotator::ZeroRotator);
 }

 if (!MoonfallWorldBootstrap::ShouldGenerateDungeons(BootPolicy)) return;
 ReadyDungeonCount = 0;
 bDungeonWorldReadyNotified = false;
 bFinalDungeonAuditComplete = false;
 FailedDungeonSlots.Reset();
 NextFailedDungeonRetry = 0;
 AuthoredDungeonSlots.SetNumZeroed(TotalDungeons);
 AuthoredDungeonActors.Reset();
 AuthoredDungeonActors.SetNum(TotalDungeons);
 SpawnedDungeonActors.Reset();
 SpawnedDungeonActors.SetNum(TotalDungeons);
 SpawnedDungeonSlots.SetNumZeroed(TotalDungeons);
 RetriedDungeonSlots.SetNumZeroed(TotalDungeons);
 // Pre-index map-authored dungeons in one scan. Newly streamed actors may
 // still arrive later, so each unclaimed slot also checks immediately before
 // spawning. Both checks are limited to startup work, never a Tick poll.
 for (TActorIterator<AMoonfallDungeonLayout> It(GetWorld()); It; ++It) {
  // A layout being destroyed must not permanently reserve a dungeon slot.
  if (!IsValid(*It) || It->IsActorBeingDestroyed()) continue;
  const int32 Zone = It->ZoneIndex;
  const int32 Dungeon = It->DungeonIndex;
  if (Zone >= 0 && Zone < DungeonZoneCount
      && Dungeon >= 0 && Dungeon < DungeonsPerZone)
   {
    const int32 Slot = Zone * DungeonsPerZone + Dungeon;
    if (!AuthoredDungeonSlots[Slot]) {
     AuthoredDungeonSlots[Slot] = 1;
     AuthoredDungeonActors[Slot] = *It;
     ++ReadyDungeonCount;
    }
   }
 }
 NextDungeonSlot = 0;
 SpawnDungeonBatch();
 if (MoonfallWorldBootstrap::HasPendingDungeonWork(
       NextDungeonSlot, NextFailedDungeonRetry, FailedDungeonSlots.Num()))
  GetWorldTimerManager().SetTimer(DungeonSpawnTimer, this,
    &AMoonfallUnrealGameMode::SpawnDungeonBatch, 0.1f, true);
}

void AMoonfallUnrealGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 // During world teardown the timer manager may no longer be reachable.
 // Do not dereference an absent world while clearing the batch timer.
 if (GetWorld())
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
 Super::EndPlay(EndPlayReason);
}

void AMoonfallUnrealGameMode::SpawnDungeonBatch()
{
 if (!GetWorld()) return;
 // Up to eight layouts per timer pass, but stop spawning after the
 // configured millisecond budget. Always allow at least one slot so even
 // a slow machine finishes all 30 without a starvation loop.
 constexpr int32 MaxLayoutsPerBatch = 8;
 static_assert(MoonfallWorldBootstrap::MaxLayoutsPerBatch == MaxLayoutsPerBatch,
   "World startup batch size drifted from the eight-layout limit");
 const int32 BatchLimit = MoonfallWorldBootstrap::ClampBatchSize(
   MaxDungeonLayoutsPerBatch);
 const double BudgetMs = MoonfallWorldBootstrap::ClampBatchTimeBudgetMs(
   DungeonSpawnBudgetMs);
 const double BatchStartedAt = FPlatformTime::Seconds();
 // Failed spawns receive one retry on a later timer pass, not immediately
 // after the final primary slot. This gives transient world changes time
 // to settle without increasing the per-pass frame budget or retrying forever.
 const bool bStartedPrimaryPass = NextDungeonSlot < TotalDungeons;
 int32 Processed = 0;
 while (MoonfallWorldBootstrap::HasPendingDungeonWork(
     NextDungeonSlot, NextFailedDungeonRetry, FailedDungeonSlots.Num())) {
  if (bStartedPrimaryPass && NextDungeonSlot >= TotalDungeons) break;
  const double ElapsedMs =
    (FPlatformTime::Seconds() - BatchStartedAt) * 1000.0;
  if (!MoonfallWorldBootstrap::ShouldContinueBatch(
      Processed, BatchLimit, ElapsedMs, BudgetMs)) break;
  const bool bRetryAttempt = NextDungeonSlot >= TotalDungeons;
  const int32 Slot = bRetryAttempt
    ? FailedDungeonSlots[NextFailedDungeonRetry++] : NextDungeonSlot++;
  ++Processed;
  if (bRetryAttempt && RetriedDungeonSlots.IsValidIndex(Slot))
   RetriedDungeonSlots[Slot] = 1;
  if (ReconcileAuthoredDungeonSlot(Slot)) continue;
  if (ReconcileSpawnedDungeonSlot(Slot)) continue;
  // A streaming level can introduce an authored layout after BeginPlay's
  // map scan, even before the first attempt for this slot. Check both
  // primary and retry attempts to avoid duplicate dungeon geometry.
  if (AMoonfallDungeonLayout* Existing = FindExistingDungeonForSlot(Slot)) {
   AuthoredDungeonSlots[Slot] = 1;
   AuthoredDungeonActors[Slot] = Existing;
   ++ReadyDungeonCount;
   continue;
  }
  const int32 Zone = Slot / DungeonsPerZone;
  const int32 DungeonIndex = Slot % DungeonsPerZone;
  FTransform DungeonTransform;
  bool bSpawnSucceeded = false;
  AMoonfallDungeonLayout* Layout = nullptr;
  if (GetDungeonEntranceTransform(Zone, DungeonIndex, DungeonTransform)) {
   Layout = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
     AMoonfallDungeonLayout::StaticClass(), DungeonTransform, nullptr, nullptr,
     ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
   if (IsValid(Layout)) {
    Layout->ZoneIndex = Zone;
    Layout->DungeonIndex = DungeonIndex;
    Layout->bRandomizeChambers = true;
    Layout->FinishSpawning(DungeonTransform);
    bSpawnSucceeded = IsValid(Layout) && !Layout->IsActorBeingDestroyed();
   }
  }
  if (bSpawnSucceeded) {
   SpawnedDungeonSlots[Slot] = 1;
   SpawnedDungeonActors[Slot] = Layout;
   ++ReadyDungeonCount;
  }
  else if (MoonfallWorldBootstrap::ShouldQueueRetry(bSpawnSucceeded,bRetryAttempt))
   FailedDungeonSlots.Add(Slot);
 }
 // Only once, after initial slots and earlier retries: authored or newly
 // generated layouts can disappear after processing. Check at most 30
 // slots, without polling any actors on subsequent gameplay frames.
 if (MoonfallWorldBootstrap::ShouldAuditDungeonSlots(
     bFinalDungeonAuditComplete, NextDungeonSlot,
     NextFailedDungeonRetry, FailedDungeonSlots.Num())) {
  bFinalDungeonAuditComplete = true;
  for (int32 Slot = 0; Slot < TotalDungeons; ++Slot) {
   if (AuthoredDungeonSlots.IsValidIndex(Slot) &&
       AuthoredDungeonSlots[Slot] && !ReconcileAuthoredDungeonSlot(Slot) &&
       !RetriedDungeonSlots[Slot])
    FailedDungeonSlots.Add(Slot); // One final recovery attempt only.
   if (SpawnedDungeonSlots.IsValidIndex(Slot) && SpawnedDungeonSlots[Slot] &&
       !ReconcileSpawnedDungeonSlot(Slot) &&
       !RetriedDungeonSlots[Slot])
    FailedDungeonSlots.Add(Slot); // Do not retry a previously retried slot.
  }
 }
 if (!MoonfallWorldBootstrap::HasPendingDungeonWork(
     NextDungeonSlot, NextFailedDungeonRetry, FailedDungeonSlots.Num())) {
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
  if (!bDungeonWorldReadyNotified) {
   bDungeonWorldReadyNotified = true;
   if (ReadyDungeonCount != TotalDungeons)
    UE_LOG(LogTemp, Warning, TEXT("Moonfall dungeon world: only %d/%d layouts ready"),
      ReadyDungeonCount, TotalDungeons);
   OnDungeonWorldReady(ReadyDungeonCount);
  }
 }
}

// Shared by the initial slot pass and the one-time completion audit.
// A missing/destroying authored actor must not occupy a ready slot.
bool AMoonfallUnrealGameMode::ReconcileAuthoredDungeonSlot(int32 Slot)
{
 if (!AuthoredDungeonSlots.IsValidIndex(Slot) || !AuthoredDungeonSlots[Slot])
  return false;
 AMoonfallDungeonLayout* AuthoredActor =
   AuthoredDungeonActors.IsValidIndex(Slot)
     ? AuthoredDungeonActors[Slot].Get() : nullptr;
 const bool bAuthoredValid = IsValid(AuthoredActor);
 if (MoonfallWorldBootstrap::ShouldRetainAuthoredSlot(
     bAuthoredValid, bAuthoredValid && AuthoredActor->IsActorBeingDestroyed()))
  return true;
 // Prefer another valid actor with the same dungeon identity rather than
 // spawning a duplicate where a map already has a suitable layout.
 if (AMoonfallDungeonLayout* Alternate = FindExistingDungeonForSlot(Slot)) {
  AuthoredDungeonActors[Slot] = Alternate;
  return true;
 }
 AuthoredDungeonSlots[Slot] = 0;
 if (AuthoredDungeonActors.IsValidIndex(Slot))
  AuthoredDungeonActors[Slot].Reset();
 ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);
 return false;
}

// Generated dungeons also count toward readiness. Detect actors destroyed
// before the one-time final audit and release their counted slot.
bool AMoonfallUnrealGameMode::ReconcileSpawnedDungeonSlot(int32 Slot)
{
 if (!SpawnedDungeonSlots.IsValidIndex(Slot) || !SpawnedDungeonSlots[Slot])
  return false;
 AMoonfallDungeonLayout* SpawnedActor =
   SpawnedDungeonActors.IsValidIndex(Slot)
     ? SpawnedDungeonActors[Slot].Get() : nullptr;
 const bool bSpawnedValid = IsValid(SpawnedActor);
 if (MoonfallWorldBootstrap::ShouldRetainAuthoredSlot(
     bSpawnedValid, bSpawnedValid && SpawnedActor->IsActorBeingDestroyed()))
  return true;
 if (AMoonfallDungeonLayout* Alternate = FindExistingDungeonForSlot(Slot)) {
  SpawnedDungeonActors[Slot] = Alternate;
  return true;
 }
 SpawnedDungeonSlots[Slot] = 0;
 if (SpawnedDungeonActors.IsValidIndex(Slot))
  SpawnedDungeonActors[Slot].Reset();
 ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);
 return false;
}

// Bounded lookup for a missing tracked layout or an unclaimed startup slot.
// This can run during initial generation and retries, never on gameplay Tick.
AMoonfallDungeonLayout* AMoonfallUnrealGameMode::FindExistingDungeonForSlot(
 int32 Slot) const
{
 if (!GetWorld() || Slot < 0 || Slot >= TotalDungeons) return nullptr;
 for (TActorIterator<AMoonfallDungeonLayout> It(GetWorld()); It; ++It) {
  AMoonfallDungeonLayout* Candidate = *It;
  if (!IsValid(Candidate) || Candidate->IsActorBeingDestroyed()) continue;
  if (MoonfallWorldBootstrap::MatchesDungeonSlot(
      Candidate->ZoneIndex, Candidate->DungeonIndex, Slot))
   return Candidate;
 }
 return nullptr;
}
