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
assert "IsActorBeingDestroyed()" in cpp
assert "ClearTimer(DungeonSpawnTimer)" in cpp
assert "Super::EndPlay(EndPlayReason)" in cpp
assert "constexpr int32 MaxLayoutsPerBatch = 2;" in cpp
assert "MoonfallWorldBootstrap::MaxLayoutsPerBatch == MaxLayoutsPerBatch" in cpp
assert "constexpr FPolicy IsolatedWorldPolicy()" in policy
assert "static_assert(TotalDungeons==30)" in policy
assert "GetWorld()->SpawnActor<AMoonfallArena>" in cpp
assert "GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>" in cpp
print("World bootstrap source guards passed; UE5 compilation unverified")
