"""Guard exact UE5/Godot 141-node parity and the versioned talent save contract.

This runs portable C++17 talent rules. It does NOT compile or play Unreal Engine.
"""
from __future__ import annotations

import json
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "godot-v38/data/moonfall_talents.json"
HEADER = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentRules.h"
HERO_H = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.h"
HERO_CPP = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp"
SAVE = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallPersistentSave.h"
TEST_CPP = ROOT / "tools/test_unreal_talent_rules.cpp"
TEST_WEB_CPP = ROOT / "tools/test_unreal_talent_web.cpp"
WEB = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentWeb.h"
HUD = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp"
INPUT = ROOT / "MoonfallUnreal/Config/DefaultInput.ini"

ROW = re.compile(
    r'^\s*\{"([^"]+)","([^"]+)",Branch::(\w+),(\d+),(\d+),(-?\d+),(-?\d+),'
    r'(\d+),Stat::(\w+),([0-9.eE+-]+),(true|false)\},?$',
    re.MULTILINE,
)
BRANCHES = {"might": "Might", "agility": "Agility", "arcana": "Arcana"}
STATS = {
    "health": "Health", "mana": "Mana", "stamina": "Stamina",
    "melee_damage": "MeleeDamage", "ranged_damage": "RangedDamage",
    "magic_damage": "MagicDamage",
}


class UnrealTalentParityTests(unittest.TestCase):
    def test_all_141_godot_nodes_and_prerequisites_match(self) -> None:
        data = json.loads(SOURCE.read_text(encoding="utf-8"))
        source = HEADER.read_text(encoding="utf-8")
        nodes = data["nodes"]
        self.assertEqual(data["schema_version"], 1)
        self.assertEqual(data["points_per_level"], 2)
        self.assertEqual(len(nodes), 141)
        captured = ROW.findall(source)
        self.assertEqual(len(captured), len(nodes), "generated catalog differs from Godot data")
        ids = {node["id"]: i for i, node in enumerate(nodes)}
        for position, (entry, raw) in enumerate(zip(nodes, captured)):
            (name, title, branch, tier, min_level, first, second, max_rank,
             stat, amount, is_percent) = raw
            with self.subTest(node=name):
                self.assertEqual(name, entry["id"])
                self.assertEqual(title, entry["title"])
                self.assertEqual(branch, BRANCHES[entry["branch"]])
                self.assertEqual(int(tier), entry["tier"])
                self.assertEqual(int(min_level), entry["min_level"])
                self.assertEqual(int(max_rank), entry["max_rank"])
                self.assertEqual(entry["cost_per_rank"], 1)
                requirements = [ids[parent] for parent in entry["requires"]]
                self.assertEqual([int(first), int(second)],
                                 (requirements + [-1, -1])[:2])
                self.assertEqual(stat, STATS[entry["effect"]["stat"]])
                self.assertAlmostEqual(float(amount), entry["effect"]["amount_per_rank"])
                self.assertEqual(is_percent == "true", entry["effect"]["kind"] == "percent")
        self.assertEqual([sum(n["branch"] == name for n in nodes) for name in BRANCHES],
                         [47, 47, 47])

    def test_unreal_gameplay_save_hooks(self) -> None:
        h = HERO_H.read_text(encoding="utf-8")
        cpp = HERO_CPP.read_text(encoding="utf-8")
        save = SAVE.read_text(encoding="utf-8")
        self.assertIn("int32 Version = 13", save)
        self.assertIn("UPROPERTY() TArray<int32> TalentRanks;", save)
        self.assertIn("UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=\"Moonfall|Talents\") TArray<int32> TalentRanks;", h)
        for fn in ("GetAvailableTalentPoints", "SpendTalentNode",
                   "GetTalentRank", "GetTalentBonus", "RespecAllTalents"):
            self.assertIn("AMoonfallHeroPawn::" + fn, cpp)
        self.assertIn("ReadMoonfallTalentRanks(Level,TalentRanks", cpp)
        self.assertIn("Save->TalentRanks = TalentRanks;", cpp)
        self.assertIn("Save->Version > 13", cpp)
        self.assertIn("ReadMoonfallTalentRanks(Save->Level,Save->TalentRanks", cpp)
        self.assertIn("Save->Version >= 13 ? Save->TalentRanks : TArray<int32>{}", cpp)
        self.assertIn("bPersistentSaveWritesBlocked = true;", cpp)

    def test_offensive_arcana_bonuses_reach_real_spell_damage(self) -> None:
        cpp = HERO_CPP.read_text(encoding="utf-8")
        cast = cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        hostile_start = cast.index("if (School != 3) {")
        talent_read = cast.index('GetTalentBonus(FName(TEXT("magic_damage")), true)')
        damage_formula = cast.index("* MightMultiplier * TalentMagicMultiplier;")
        hit = cast.index("Enemy->TakeSpellDamage(SpellDamage);")
        healing = cast.index("if (School == 3) {")
        self.assertLess(hostile_start, talent_read)
        self.assertLess(talent_read, damage_formula)
        self.assertLess(damage_formula, hit)
        self.assertLess(hit, healing)
        self.assertIn("FMath::Clamp(\n      GetTalentBonus(", cast)
        self.assertIn("const float TalentMagicMultiplier = 1.f + TalentMagicBonus;", cast)
        # The safety ceiling is above the highest legitimate Godot allocation.
        godot = json.loads(SOURCE.read_text(encoding="utf-8"))
        max_bonus = sum(
            node["max_rank"] * node["effect"]["amount_per_rank"]
            for node in godot["nodes"]
            if node["effect"]["stat"] == "magic_damage"
        )
        self.assertLess(max_bonus, 1.0)

    def test_live_resource_caps_and_save_roundtrip_hooks(self) -> None:
        cpp = HERO_CPP.read_text(encoding="utf-8")
        rules = HEADER.read_text(encoding="utf-8")
        self.assertIn("inline bool ApplyResourceDelta(Resources& Live", rules)
        self.assertIn("Candidate.Health=Live.Health<=0.0 ? 0.0", rules)
        self.assertIn("bool ReconcileMoonfallTalentResources(AMoonfallHeroPawn* Hero", cpp)
        apply = cpp.split("bool ReconcileMoonfallTalentResources(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetTalentNodeCount()", 1)[0]
        for field in ("MaxHealth", "Health", "MaxMana", "Mana", "MaxStamina", "Stamina"):
            self.assertIn(f"Hero->{field}=static_cast<float>(Current.{field});", apply)
        for method, boundary in (("SpendTalentNode", "RespecAllTalents"),
                                 ("RespecAllTalents", "AwardExperience")):
            action = cpp.split(f"bool AMoonfallHeroPawn::{method}(", 1)[1].split(
                f"AMoonfallHeroPawn::{boundary}(", 1)[0]
            self.assertLess(action.index("ReconcileMoonfallTalentResources(this,Before,After)"),
                            action.index("SaveWorldProgress();"))
            self.assertIn("Health<=0.f", action, "A defeated hero cannot gain capacity")
        save = cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        self.assertIn("BaseHealthCap=static_cast<double>(MaxHealth)", save)
        self.assertIn("ValidTalentSummary.Flat[static_cast<int>(MoonfallTalents::Stat::Health)]", save)
        self.assertIn("Save->MaxHealth = static_cast<float>(BaseHealthCap);", save)
        load = cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("PreviousSummary.Flat[static_cast<int>(MoonfallTalents::Stat::Mana)]", load)
        self.assertIn("PreviousSummary.Flat[static_cast<int>(MoonfallTalents::Stat::Stamina)]", load)
        self.assertIn("LoadedSummary.Flat[static_cast<int>(MoonfallTalents::Stat::Health)]", load)
        self.assertIn("MaxHealth = static_cast<float>(LoadedHealthCap);", load)
        self.assertIn("MaxMana = static_cast<float>(LoadedManaCap);", load)
        self.assertIn("MaxStamina = static_cast<float>(LoadedStaminaCap);", load)
        self.assertLess(load.index("if (!std::isfinite(LoadedHealthCap)"),
                        load.index("TeleportTo(SavedCheckpoint"))

    def test_live_talent_web_controls_and_shared_canvas_geometry(self) -> None:
        cpp = HERO_CPP.read_text(encoding="utf-8")
        hud = HUD.read_text(encoding="utf-8")
        keys = INPUT.read_text(encoding="utf-8")
        web = WEB.read_text(encoding="utf-8")
        hero = HERO_H.read_text(encoding="utf-8")
        for action, key in (("TalentWeb", "K"), ("TalentSpend", "Enter"),
                            ("TalentRespec", "BackSpace")):
            self.assertIn(f'ActionName="{action}",Key={key}', keys)
        self.assertIn("bTalentWebOpen", hero)
        self.assertIn("void AMoonfallHeroPawn::ToggleTalentWeb()", cpp)
        self.assertIn("void AMoonfallHeroPawn::TalentPointerClick()", cpp)
        self.assertIn("void AMoonfallHeroPawn::TalentConfirmRespec()", cpp)
        self.assertIn("void AMoonfallHeroPawn::TalentSpendSelected()", cpp)
        self.assertIn("TalentRespecArmUntil=Now+3.0f", cpp)
        self.assertIn("if (bTalentWebOpen) {TalentPointerClick();return;}", cpp)
        self.assertIn("if (bTalentWebOpen) {TalentSpendSelected();return;}", cpp)
        self.assertIn("if (bTalentWebOpen) {TalentChangeBranch(-1);return;}", cpp)
        self.assertIn("if (bTalentWebOpen) {TalentChangeBranch(1);return;}", cpp)
        self.assertIn("if (bTalentWebOpen) {TalentConfirmRespec();return;}", cpp)
        self.assertIn("if (bTalentWebOpen) {OptionsUp();return;}", cpp)
        stats = cpp.split("void AMoonfallHeroPawn::ToggleCharacterStats()", 1)[1].split(
            "void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()", 1)[0]
        self.assertIn("if (bTalentWebOpen || bOptionsOpen", stats)
        # Gamepad A has both Jump and CharacterStats bindings. Spending
        # must consume the A event without toggling the inventory panel.
        self.assertIn('ActionName="CharacterStats",Key=Gamepad_FaceButton_Bottom', keys)
        self.assertIn("if (!bTalentWebOpen && !bCharacterStatsOpen)", cpp)
        self.assertIn("MoonfallTalentWeb::HitNode(", cpp)
        self.assertIn("MoonfallTalentWeb::NodeRect(", hud)
        self.assertIn("MoonfallTalentWeb::SpendRect(", hud)
        self.assertIn("MoonfallTalentWeb::RespecRect(", hud)
        self.assertLess(hud.index("if (Hero->bTalentWebOpen)"),
                        hud.index("if (Hero->bAtlasOpen)"))
        self.assertIn("inline Rect NodeRect(", web)
        self.assertIn("inline int HitNode(", web)
        self.assertIn("inline int Move(", web)
        self.assertIn("if (bTalentWebOpen && Health<=0.f)ToggleTalentWeb();", cpp)
        # Prevent accidental input fallthrough into combat and item pickups
        # while the constellation owns the mouse or gamepad.
        for method_name in ("TryCast", "MoveForward", "MoveRight",
                            "PickupNearestGroundLoot", "ActivateRacePower"):
            self.assertIn("bTalentWebOpen",
                cpp.split(f"AMoonfallHeroPawn::{method_name}(", 1)[1].split("}", 1)[0])
        # The original 141 talent slots are drawn from the same canonical catalog.
        self.assertIn("for(int I=0;I<MoonfallTalents::NodeCount;++I)", hud)

    def test_portable_talent_web_navigation_cpp(self) -> None:
        compiler = shutil.which("g++") or shutil.which("clang++")
        self.assertIsNotNone(compiler, "C++17 required for native web geometry checks")
        with tempfile.TemporaryDirectory(prefix="moonfall_web_") as tmp:
            exe = Path(tmp) / "talent_web"
            subprocess.run(
                [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-pedantic", str(TEST_WEB_CPP), "-o", str(exe)],
                check=True, cwd=ROOT, capture_output=True, text=True,
            )
            subprocess.run([str(exe)], check=True, cwd=ROOT,
                           capture_output=True, text=True)

    def test_portable_cplusplus_rules(self) -> None:
        compiler = shutil.which("g++") or shutil.which("clang++")
        self.assertIsNotNone(compiler, "A C++17 compiler is required for talent parity checks")
        with tempfile.TemporaryDirectory(prefix="moonfall_talents_") as tmp:
            executable = Path(tmp) / "talent_rules"
            subprocess.run(
                [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-pedantic", str(TEST_CPP), "-o", str(executable)],
                check=True, cwd=ROOT, capture_output=True, text=True,
            )
            subprocess.run([str(executable)], check=True, cwd=ROOT,
                           capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
