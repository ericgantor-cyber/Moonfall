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

assert "int32 Version = 17;" in save
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
assert "Save->Version > MaxEquipmentSaveVersion" in load_body
assert "constexpr int32 MaxEquipmentSaveVersion" in load_body
assert load_body.index("Save->Version > MaxEquipmentSaveVersion") < load_body.index("TeleportTo(")
assert "MoonfallTalents::Ranks LoadedTalentRanks{};" in load_body
assert "if (Save->Version >= 13)" in load_body
assert "Save->TalentRanks.Num() != MoonfallTalents::NodeCount" in load_body
assert "MoonfallTalents::Evaluate(Save->Level, LoadedTalentRanks, LoadedTalentSnapshot)" in load_body
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
    assert "return CommitTalentAllocation(Candidate);" in body
commit_start = hero.index("bool AMoonfallHeroPawn::CommitTalentAllocation(")
commit_end = hero.index("\n}", commit_start) + 2
commit = hero[commit_start:commit_end]
assert "MoonfallTalents::ApplyResourceDelta(UpdatedResources, Before, After)" in commit
assert "const MoonfallTalents::Ranks PreviousRanks = TalentRanks;" in commit
assert "TalentRanks = Candidate;" in commit
assert "if (!SaveWorldProgress()) {" in commit
assert "TalentRanks = PreviousRanks;" in commit
for resource in ("MaxHealth", "Health", "MaxMana", "Mana", "MaxStamina", "Stamina"):
    assert resource + " = static_cast<float>(UpdatedResources." + resource + ");" in commit
    assert resource + " = static_cast<float>(PreviousResources." + resource + ");" in commit
assert commit.index("OnHealthChanged(Health, MaxHealth);") > commit.index("if (!SaveWorldProgress())")
assert "ApplyResourceDelta" in rules
# Permanent HP capacity is serialized without talent capacity; reloading adds
# only the validated bonus once after subtracting previous mana/stamina bonuses.
assert "const double BaseHealthCap = static_cast<double>(MaxHealth)" in save_body
assert "Save->MaxHealth = static_cast<float>(BaseHealthCap);" in save_body
assert "const double LoadedHealthCap = SavedBaseHealth" in load_body
assert "const double LoadedManaCap = static_cast<double>(MaxMana)" in load_body
assert "const double LoadedStaminaCap = static_cast<double>(MaxStamina)" in load_body
for resource in ("Health", "Mana", "Stamina"):
    assert "Max" + resource + " = static_cast<float>(Loaded" + resource + "Cap);" in load_body
assert load_body.index("LoadedHealthCap") < load_body.index("TeleportTo(")
assert load_body.index("LoadedManaCap") < load_body.index("TeleportTo(")
assert load_body.index("LoadedStaminaCap") < load_body.index("TeleportTo(")
assert "bool SpendTalentPoint(int32 NodeIndex);" in header
assert "bool RespecTalentPoints();" in header
assert "int32 GetAvailableTalentPoints() const;" in header
assert "int32 GetTalentRank(int32 NodeIndex) const;" in header

# No new input mappings or automatic talent spending in this save-only slice.
# Live combat must use the real hit paths, never the restoration branch.
spell_start = hero.index("bool AMoonfallHeroPawn::TryCast(")
spell_end = hero.index("void AMoonfallHeroPawn::CastLeft()", spell_start)
spell = hero[spell_start:spell_end]
melee_start = hero.index("void AMoonfallHeroPawn::MeleeAttack()")
melee_end = hero.index("Enemy->TakeMeleeDamage(", melee_start) + len("Enemy->TakeMeleeDamage(")
melee = hero[melee_start:melee_end]
assert "MoonfallTalents::DamageMultiplier(Level, TalentRanks, MoonfallTalents::Stat::MagicDamage)" in spell
assert "MoonfallTalents::DamageMultiplier(Level, TalentRanks, MoonfallTalents::Stat::MeleeDamage)" in melee
assert spell.index("if (School != 3) {") < spell.index("TalentMagicMultiplier") < spell.index("Enemy->TakeSpellDamage(SpellDamage)")
assert "* MightMultiplier * TalentMagicMultiplier;" in spell
assert melee.index("if (!bLivingTarget || !IsValid(Enemy) || Enemy->IsActorBeingDestroyed()") < melee.index("TalentMeleeMultiplier")
assert "* (1.f + GetVersatilityBonus()) * TalentMeleeMultiplier;" in melee
assert melee.index("TalentMeleeMultiplier") < melee.index("Enemy->TakeMeleeDamage(")
assert "DamageMultiplier(int Level,const Ranks& Allocated,Stat Kind)" in rules
assert "if (Evaluate(Level,Allocated,Summary)!=Result::Ok)return 1.0;" in rules

print("UE5 V13 talent saves: safe migration, three live resource caps and atomic rollback verified.")
