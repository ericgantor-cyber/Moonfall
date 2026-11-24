"""UE5 V13 talent-save integration contracts without requiring Unreal Editor binaries.

These source checks supplement, but never replace, native UE5 Windows tests.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
header = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
save = (SRC / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
rules = (SRC / "MoonfallTalentRules.h").read_text(encoding="utf-8")

assert "int32 Version = 13;" in save
assert "TArray<int32> TalentRanks;" in save
assert 'MoonfallTalents::Ranks TalentRanks{};' in header
assert '#include "MoonfallTalentRules.h"' in header
assert "constexpr int NodeCount=141;" in rules

save_start = hero.index("bool AMoonfallHeroPawn::SaveWorldProgress()")
load_start = hero.index("bool AMoonfallHeroPawn::LoadWorldProgress()")
save_body, load_body = hero[save_start:load_start], hero[load_start:]
assert "MoonfallTalents::Evaluate(Level, TalentRanks, ValidTalentSnapshot)" in save_body
assert save_body.index("Evaluate(Level, TalentRanks, ValidTalentSnapshot)") < save_body.index("CreateSaveGameObject")
assert "Save->TalentRanks.Reserve(MoonfallTalents::NodeCount)" in save_body
assert "for (const int32 Rank : TalentRanks) Save->TalentRanks.Add(Rank)" in save_body
assert "Save->Version > 13" in load_body
assert "MoonfallTalents::Ranks LoadedTalentRanks{};" in load_body
assert "if (Save->Version >= 13)" in load_body
assert "Save->TalentRanks.Num() != MoonfallTalents::NodeCount" in load_body
assert "MoonfallTalents::Evaluate(Save->Level, LoadedTalentRanks, ValidatedTalentSnapshot)" in load_body
assert load_body.index("Evaluate(Save->Level, LoadedTalentRanks") < load_body.index("TeleportTo(")
assert load_body.index("TalentRanks = LoadedTalentRanks;") > load_body.index("TeleportTo(")
assert "bPersistentSaveWritesBlocked = true;" in load_body
# V1-V12 saves load zero spent ranks; V13 validates before mutation.
assert load_body.index("MoonfallTalents::Ranks LoadedTalentRanks{};") < load_body.index("if (Save->Version >= 13)")
for method in ("SpendTalentPoint", "RespecTalentPoints"):
    begin = hero.index("bool AMoonfallHeroPawn::" + method + "(")
    end = hero.index("\n}", begin) + 2
    body = hero[begin:end]
    assert "MoonfallTalents::Ranks Candidate = TalentRanks;" in body
    assert "const MoonfallTalents::Ranks Previous = TalentRanks;" in body
    assert "TalentRanks = Candidate;" in body
    assert "if (!SaveWorldProgress()) { TalentRanks = Previous; return false; }" in body
assert "bool SpendTalentPoint(int32 NodeIndex);" in header
assert "bool RespecTalentPoints();" in header
assert "int32 GetAvailableTalentPoints() const;" in header
assert "int32 GetTalentRank(int32 NodeIndex) const;" in header

# No new input mappings or automatic talent spending in this save-only slice.
print("UE5 talent save V13: old-save defaults, strict load validation and transaction rollback verified.")
