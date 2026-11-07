"""Safety and coverage gate for the planned Godot-to-Unreal migration.

Static verification only; does not imply an Unreal build, feature parity, or gameplay success.
"""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "engine-comparison/UE5_MIGRATION_PARITY.json"


class UnrealMigrationParityGate(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_source_projects_remain_separate(self):
        self.assertTrue((ROOT / self.data["unreal_project"]).is_file())
        self.assertTrue((ROOT / self.data["godot_reference"]).is_file())
        self.assertIn("Godot remains production", self.data["decision"])
        self.assertTrue(self.data["constraints"]["do_not_replace_godot_without_build"])

    def test_no_untested_feature_claimed_complete(self):
        features = self.data["features"]
        ids = [feature["id"] for feature in features]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertIn("windows_release", ids)
        for feature in features:
            with self.subTest(feature=feature["id"]):
                self.assertNotEqual(feature["status"], "verified")
                self.assertTrue(feature["gate"])
                for relative_path in feature["unreal"]:
                    self.assertTrue((ROOT / relative_path).is_file(), relative_path)

    def test_quest_tracker_uses_persistent_combat_and_loot_flags(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype"
        header = (base / "MoonfallPawn.h").read_text(encoding="utf-8")
        pawn = (base / "MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "MoonfallHud.cpp").read_text(encoding="utf-8")
        saves = (base / "MoonfallProgressSave.h").read_text(encoding="utf-8")
        for method in ("GetQuestDefeatedBeasts", "GetQuestClearedChambers"):
            self.assertIn(method + "() const;", header)
            self.assertIn("AMoonfallPawn::" + method + "() const", pawn)
            self.assertIn("Hero->" + method + "()", hud)
        self.assertIn("DefeatedEnemyMask & (1 << Id)", pawn)
        self.assertIn("ClaimedChamberMask & (1 << Id)", pawn)
        self.assertIn("DefeatedGuardianMask & (1 << Id)", pawn)
        for flag in ("DefeatedEnemyMask", "ClaimedChamberMask", "DefeatedGuardianMask"):
            self.assertIn(flag, saves)
            self.assertIn("Progress->" + flag + " = " + flag + ";", pawn)
        self.assertIn("WORLD QUESTS", hud)
        self.assertIn("Defeat Z", hud)
        self.assertIn("Hero->IsBossDefeated()", hud)

    def test_region_fog_does_not_reveal_unvisited_map_markers(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype"
        pawn = (base / "MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "MoonfallHud.cpp").read_text(encoding="utf-8")
        saves = (base / "MoonfallProgressSave.h").read_text(encoding="utf-8")
        self.assertIn("int32 DiscoveredRegionMask = 0;", header)
        self.assertIn("int32 DiscoveredRegionMask = 0;", saves)
        self.assertIn("Progress->DiscoveredRegionMask = DiscoveredRegionMask;", pawn)
        self.assertIn("DiscoveredRegionMask = Progress->DiscoveredRegionMask &", pawn)
        self.assertIn("DiscoveredRegionMask |= 1 << MoonfallWorldRules::RegionAt(", pawn)
        self.assertIn("if ((DiscoveredRegionMask & RegionBit) == 0)", pawn)
        self.assertIn("SaveProgress(); // Persist discoveries", pawn)
        self.assertIn("bool AMoonfallPawn::IsMapPositionDiscovered(", pawn)
        self.assertIn("int32 AMoonfallPawn::GetDiscoveredRegionCount()", pawn)
        self.assertIn("Hero->IsMapPositionDiscovered(Chamber)", hud)
        self.assertIn("Hero->IsMapPositionDiscovered(Enemy)", hud)
        self.assertIn("Hero->IsMapPositionDiscovered(B)", hud)
        self.assertIn("DISCOVERED %d/6", hud)

    def test_character_and_input_constraints(self):
        constraints = self.data["constraints"]
        self.assertEqual(constraints["preserve_original_art"],
                         ["Z_Original.webp", "Nyra_Original.png"])
        self.assertEqual(constraints["boss_roles"], ["Z", "Nyra"])
        self.assertTrue(constraints["click_only_attack"])
        self.assertTrue(constraints["continuous_world"])
        self.assertTrue(constraints["no_waves"])
        self.assertEqual(constraints["windows"], ["10", "11"])
        input_text = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', input_text)
        self.assertNotIn('ActionName="Attack",Key=SpaceBar', input_text)


if __name__ == "__main__":
    unittest.main()
