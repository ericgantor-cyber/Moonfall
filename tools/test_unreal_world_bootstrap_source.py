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
assert "ClearTimer(DungeonSpawnTimer)" in cpp
assert "Super::EndPlay(EndPlayReason)" in cpp
assert "constexpr int32 MaxLayoutsPerBatch = 2;" in cpp
assert "MoonfallWorldBootstrap::MaxLayoutsPerBatch == MaxLayoutsPerBatch" in cpp
assert "constexpr FPolicy IsolatedWorldPolicy()" in policy
assert "static_assert(TotalDungeons==30)" in policy
assert "GetWorld()->SpawnActor<AMoonfallArena>" in cpp
assert "GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>" in cpp
print("World bootstrap source guards passed; UE5 compilation unverified")
