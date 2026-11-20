"""Source-level regression checks for Moonfall divine-family presentation.

Run: python -m unittest tests.test_divine_family_roster
These checks do not imply a successful Unreal/Godot compile or playtest.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class DivineFamilyRosterTests(unittest.TestCase):
    def test_godot_new_game_lore_has_three_divine_members(self):
        text = read("godot-v38/scripts/native_v11.gd")
        for label in ("Z • Void Reaper", "Nyra • Eclipse Queen",
                      "Morvella Velmora • Goddess of Death"):
            self.assertIn(label, text)
        self.assertIn("these gods are not playable", text)

    def test_unreal_optional_boss_preset_keeps_existing_gods(self):
        arena = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp")
        identity = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        self.assertIn('TEXT("z_void_reaper")', arena)
        self.assertIn('TEXT("nyra_eclipse_queen")', arena)
        self.assertIn('TEXT("death_goddess_crimson_reaper")', identity)
        self.assertIn("bSingleDivineEncounter", arena)
        self.assertIn("bDeathGoddessEncounter", arena)

    def test_unreal_hud_uses_divine_title_and_four_phase_total(self):
        hud = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp")
        self.assertIn("Boss->DivineBossTitle", hud)
        self.assertIn('TEXT("Morvella Velmora")', read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp"))
        self.assertIn("GODS OF MOONFALL", hud)
        self.assertIn("MoonfallDeathGoddess::IsDeathGoddess", hud)
        self.assertIn(" ? 4 : 3", hud)

    def test_phase_thresholds_and_stable_divine_roles(self):
        rules = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        enemy = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")
        for threshold in ("0.75f", "0.50f", "0.25f"):
            self.assertIn(threshold, rules)
        self.assertIn("PhaseForHealthRatio(HealthRatio)", enemy)
        self.assertIn('TEXT("z_void_reaper")', enemy)
        self.assertIn('TEXT("nyra_eclipse_queen")', enemy)


if __name__ == "__main__":
    unittest.main()
