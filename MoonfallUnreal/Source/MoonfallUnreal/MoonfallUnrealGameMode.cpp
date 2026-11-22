#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallCombatHUD.h"
#include "MoonfallWorldBootstrap.h"
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
 AuthoredDungeonSlots.SetNumZeroed(TotalDungeons);
 // Inspect authored layouts only once, rather than scanning world actors for
 // each of the thirty spawn requests. Never duplicate an authored dungeon.
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
     ++ReadyDungeonCount;
    }
   }
 }
 NextDungeonSlot = 0;
 SpawnDungeonBatch();
 if (NextDungeonSlot < TotalDungeons)
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
 // Keep the startup frame responsive. Thirty layouts take no Tick of their own;
 // the short timer stops entirely when all slots have been considered.
 // Keep the legacy integration invariant and require the new portable
 // policy to agree with the established two-layout startup budget.
 constexpr int32 MaxLayoutsPerBatch = 2;
 static_assert(MoonfallWorldBootstrap::MaxLayoutsPerBatch == MaxLayoutsPerBatch,
   "World startup budget drifted from the legacy two-layout limit");
 int32 Processed = 0;
 while (NextDungeonSlot < TotalDungeons && Processed < MaxLayoutsPerBatch) {
  const int32 Slot = NextDungeonSlot++;
  ++Processed;
  if (AuthoredDungeonSlots.IsValidIndex(Slot) && AuthoredDungeonSlots[Slot])
   continue;
  const int32 Zone = Slot / DungeonsPerZone;
  const int32 DungeonIndex = Slot % DungeonsPerZone;
  FTransform DungeonTransform;
  if (!GetDungeonEntranceTransform(Zone, DungeonIndex, DungeonTransform))
   continue;
  AMoonfallDungeonLayout* Layout = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
    AMoonfallDungeonLayout::StaticClass(), DungeonTransform, nullptr, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(Layout)) continue;
  Layout->ZoneIndex = Zone;
  Layout->DungeonIndex = DungeonIndex;
  Layout->bRandomizeChambers = true;
  Layout->FinishSpawning(DungeonTransform);
  if (IsValid(Layout) && !Layout->IsActorBeingDestroyed()) ++ReadyDungeonCount;
 }
 if (NextDungeonSlot >= TotalDungeons) {
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
  if (ReadyDungeonCount != TotalDungeons)
   UE_LOG(LogTemp, Warning, TEXT("Moonfall dungeon world: only %d/%d layouts ready"),
     ReadyDungeonCount, TotalDungeons);
  OnDungeonWorldReady(ReadyDungeonCount);
 }
}
