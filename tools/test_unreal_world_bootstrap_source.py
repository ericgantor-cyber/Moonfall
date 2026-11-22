"""Static guardrails. Does not imply successful native Unreal Engine compilation."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder=root/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
header=(folder/"MoonfallUnrealGameMode.h").read_text(encoding="utf-8")
cpp=(folder/"MoonfallUnrealGameMode.cpp").read_text(encoding="utf-8")
policy=(folder/"MoonfallWorldBootstrap.h").read_text(encoding="utf-8")
assert "bool bGenerateLegacyArena = true;" in header
assert "bool bGenerateDungeonWorld = true;" in header
assert "virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override" in header
assert '#include "MoonfallWorldBootstrap.h"' in cpp
assert "ShouldSpawnArena(BootPolicy,bHasArena)" in cpp
assert "ShouldGenerateDungeons(BootPolicy)" in cpp
startup=cpp.split("void AMoonfallUnrealGameMode::BeginPlay()",1)[1].split(
    "void AMoonfallUnrealGameMode::EndPlay(",1)[0]
arena=startup.split("for (TActorIterator<AMoonfallArena>",1)[1].split(
    "if (MoonfallWorldBootstrap::ShouldSpawnArena",1)[0]
dungeons=startup.split("for (TActorIterator<AMoonfallDungeonLayout>",1)[1].split(
    "NextDungeonSlot = 0;",1)[0]
assert "IsValid(*It) && !It->IsActorBeingDestroyed()" in arena
# A destroyed layout must be rejected before it can reserve a slot or count ready.
assert "if (!IsValid(*It) || It->IsActorBeingDestroyed()) continue;" in dungeons
assert dungeons.index("It->IsActorBeingDestroyed()") < dungeons.index(
    "AuthoredDungeonSlots[Slot] = 1;")
assert dungeons.index("AuthoredDungeonSlots[Slot] = 1;") < dungeons.index(
    "++ReadyDungeonCount;")
endplay=cpp.split("void AMoonfallUnrealGameMode::EndPlay(",1)[1].split(
    "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
assert "if (GetWorld())" in endplay
assert "GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);" in endplay
assert "Super::EndPlay(EndPlayReason);" in endplay
assert endplay.index("if (GetWorld())") < endplay.index(
    "GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);")
assert endplay.index("GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);") < endplay.index(
    "Super::EndPlay(EndPlayReason);")
assert "constexpr int32 MaxLayoutsPerBatch = 8;" in cpp
assert "inline constexpr int MaxLayoutsPerBatch=8;" in policy
assert "int32 MaxDungeonLayoutsPerBatch = 8;" in header
assert "float DungeonSpawnBudgetMs = 2.5f;" in header
assert '#include "HAL/PlatformTime.h"' in cpp
batch=cpp.split("void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[1]
assert "MoonfallWorldBootstrap::ClampBatchSize(" in batch
assert "MoonfallWorldBootstrap::ClampBatchTimeBudgetMs(" in batch
assert "FPlatformTime::Seconds()" in batch
assert "MoonfallWorldBootstrap::ShouldContinueBatch(" in batch
assert batch.index("ShouldContinueBatch(") < batch.index(
    "const bool bRetryAttempt = NextDungeonSlot >= TotalDungeons;")
assert batch.index("const bool bRetryAttempt = NextDungeonSlot >= TotalDungeons;") < batch.index(
    "FailedDungeonSlots[NextFailedDungeonRetry++] : NextDungeonSlot++;")
for value in ("BatchCount(0)==8","BatchCount(22)==8","BatchCount(23)==7",
              "BatchCount(27)==3","BatchCount(29)==1","BatchCount(30)==0"):
    assert value in policy
assert "MoonfallWorldBootstrap::MaxLayoutsPerBatch == MaxLayoutsPerBatch" in cpp
assert "constexpr FPolicy IsolatedWorldPolicy()" in policy
assert "static_assert(TotalDungeons==30)" in policy
assert "GetWorld()->SpawnActor<AMoonfallArena>" in cpp
assert "GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>" in cpp
assert "TArray<int32> FailedDungeonSlots;" in header
assert "int32 NextFailedDungeonRetry = 0;" in header
assert "bool bDungeonWorldReadyNotified = false;" in header
assert "bool bFinalDungeonAuditComplete = false;" in header
assert "bFinalDungeonAuditComplete = false;" in startup
assert "TArray<uint8> SpawnedDungeonSlots;" in header
assert "TArray<TWeakObjectPtr<AMoonfallDungeonLayout>> SpawnedDungeonActors;" in header
assert "TArray<uint8> RetriedDungeonSlots;" in header
for expected in ("SpawnedDungeonActors.SetNum(TotalDungeons);",
                 "SpawnedDungeonSlots.SetNumZeroed(TotalDungeons);",
                 "RetriedDungeonSlots.SetNumZeroed(TotalDungeons);"):
    assert expected in startup
assert "FailedDungeonSlots.Reset();" in startup
assert "TArray<TWeakObjectPtr<AMoonfallDungeonLayout>> AuthoredDungeonActors;" in header
assert "AuthoredDungeonActors.Reset();" in startup
assert "AuthoredDungeonActors.SetNum(TotalDungeons);" in startup
assert "AuthoredDungeonActors[Slot] = *It;" in startup
assert startup.index("AuthoredDungeonActors[Slot] = *It;") < startup.index(
    "SpawnDungeonBatch();")
assert "NextFailedDungeonRetry = 0;" in startup
batch = cpp.split("void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[1]
assert "const bool bStartedPrimaryPass = NextDungeonSlot < TotalDungeons;" in batch
assert "if (bStartedPrimaryPass && NextDungeonSlot >= TotalDungeons) break;" in batch
assert "FailedDungeonSlots[NextFailedDungeonRetry++]" in batch
assert "if (ReconcileAuthoredDungeonSlot(Slot)) continue;" in batch
assert "bool AMoonfallUnrealGameMode::ReconcileAuthoredDungeonSlot(int32 Slot)" in cpp
reconcile=cpp.split("bool AMoonfallUnrealGameMode::ReconcileAuthoredDungeonSlot(int32 Slot)",1)[1]
assert "AuthoredDungeonActors[Slot].Get()" in reconcile
assert "MoonfallWorldBootstrap::ShouldRetainAuthoredSlot(" in reconcile
assert "AuthoredDungeonSlots[Slot] = 0;" in reconcile
assert "AuthoredDungeonActors[Slot].Reset();" in reconcile
assert "ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);" in reconcile
assert reconcile.index("ShouldRetainAuthoredSlot(") < reconcile.index(
    "AuthoredDungeonSlots[Slot] = 0;") < reconcile.index(
    "ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);")
assert "bool ReconcileAuthoredDungeonSlot(int32 Slot);" in header
assert "MoonfallWorldBootstrap::ShouldAuditDungeonSlots(" in batch
assert "bFinalDungeonAuditComplete = true;" in batch
assert "for (int32 Slot = 0; Slot < TotalDungeons; ++Slot)" in batch
assert "AuthoredDungeonSlots[Slot] && !ReconcileAuthoredDungeonSlot(Slot)" in batch
assert "FailedDungeonSlots.Add(Slot); // One final recovery attempt only." in batch
assert batch.index("ShouldAuditDungeonSlots(") < batch.index(
    "bFinalDungeonAuditComplete = true;") < batch.index(
    "for (int32 Slot = 0; Slot < TotalDungeons; ++Slot)") < batch.index(
    "OnDungeonWorldReady(ReadyDungeonCount);")
assert "ShouldAuditDungeonSlots(false,30,1,1)" in policy
assert "!ShouldAuditDungeonSlots(true,30,1,1)" in policy
assert "if (bRetryAttempt && RetriedDungeonSlots.IsValidIndex(Slot))" in batch
assert "RetriedDungeonSlots[Slot] = 1;" in batch
assert "SpawnedDungeonSlots[Slot] = 1;" in batch
assert "SpawnedDungeonActors[Slot] = Layout;" in batch
assert batch.index("bSpawnSucceeded = IsValid(Layout)") < batch.index(
    "SpawnedDungeonActors[Slot] = Layout;") < batch.index("++ReadyDungeonCount;")
assert "bool ReconcileSpawnedDungeonSlot(int32 Slot);" in header
assert "bool AMoonfallUnrealGameMode::ReconcileSpawnedDungeonSlot(int32 Slot)" in cpp
generated=cpp.split("bool AMoonfallUnrealGameMode::ReconcileSpawnedDungeonSlot(int32 Slot)",1)[1]
assert "SpawnedDungeonActors[Slot].Get()" in generated
assert "SpawnedDungeonSlots[Slot] = 0;" in generated
assert "ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);" in generated
assert "SpawnedDungeonSlots[Slot] &&" in batch
assert "!ReconcileSpawnedDungeonSlot(Slot) &&" in batch
assert "!RetriedDungeonSlots[Slot]" in batch
assert batch.index("ReconcileSpawnedDungeonSlot(Slot)") < batch.index(
    "OnDungeonWorldReady(ReadyDungeonCount);")
assert "MatchesDungeonSlot(5,4,29)" in policy
assert "constexpr bool MatchesDungeonSlot(int Zone,int Dungeon,int Slot)" in policy
assert "AMoonfallDungeonLayout* FindExistingDungeonForSlot(int32 Slot) const;" in header
assert "AMoonfallUnrealGameMode::FindExistingDungeonForSlot(" in cpp
fallback=cpp.split("AMoonfallUnrealGameMode::FindExistingDungeonForSlot(",1)[1]
assert "if (!GetWorld() || Slot < 0 || Slot >= TotalDungeons) return nullptr;" in fallback
assert "TActorIterator<AMoonfallDungeonLayout> It(GetWorld())" in fallback
assert "!IsValid(Candidate) || Candidate->IsActorBeingDestroyed()" in fallback
assert "MoonfallWorldBootstrap::MatchesDungeonSlot(" in fallback
assert "Candidate->ZoneIndex, Candidate->DungeonIndex, Slot" in fallback
assert "if (AMoonfallDungeonLayout* Alternate = FindExistingDungeonForSlot(Slot))" in reconcile
assert reconcile.index("FindExistingDungeonForSlot(Slot)") < reconcile.index(
    "ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);")
assert "if (AMoonfallDungeonLayout* Alternate = FindExistingDungeonForSlot(Slot))" in generated
assert generated.index("FindExistingDungeonForSlot(Slot)") < generated.index(
    "ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);")
assert "if (ReconcileSpawnedDungeonSlot(Slot)) continue;" in batch
assert "if (bRetryAttempt) {" in batch
assert "if (AMoonfallDungeonLayout* Existing = FindExistingDungeonForSlot(Slot))" in batch
assert "AuthoredDungeonActors[Slot] = Existing;" in batch
assert batch.index("FindExistingDungeonForSlot(Slot)") < batch.index(
    "SpawnActorDeferred<AMoonfallDungeonLayout>(")
assert "AuthoredDungeonSlots[Slot] && !ReconcileAuthoredDungeonSlot(Slot) &&" in batch
assert "!RetriedDungeonSlots[Slot]" in batch
assert "ShouldRetainAuthoredSlot(true,false)" in policy
assert "!ShouldRetainAuthoredSlot(true,true)" in policy
assert "MoonfallWorldBootstrap::ShouldQueueRetry(bSpawnSucceeded,bRetryAttempt)" in batch
assert "FailedDungeonSlots.Add(Slot);" in batch
assert batch.index("bSpawnSucceeded = IsValid(Layout)") < batch.index(
    "if (bSpawnSucceeded) {") < batch.index(
    "SpawnedDungeonActors[Slot] = Layout;") < batch.index(
    "++ReadyDungeonCount;")
assert batch.index("if (!bDungeonWorldReadyNotified)") < batch.index("OnDungeonWorldReady(ReadyDungeonCount)")
assert batch.count("OnDungeonWorldReady(ReadyDungeonCount);") == 1
assert "ShouldQueueRetry(false,false)" in policy
assert "HasPendingDungeonWork(30,0,1)" in policy
print("World bootstrap source guards passed; UE5 compilation unverified")
