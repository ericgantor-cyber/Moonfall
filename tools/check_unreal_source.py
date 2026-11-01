#!/usr/bin/env python3
"""Fast source-contract checks; not a substitute for compiling Unreal."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class MoonfallIntegrationChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero_cpp = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.enemy_h = (ROOT / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.enemy_cpp = (ROOT / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_progression_declared_and_defined(self):
        for name in ("AwardExperience", "AwardEquipmentDrop", "RewardPerfectDodge"):
            self.assertIn(name + "(", self.hero_h)
            self.assertIn("AMoonfallHeroPawn::" + name + "(", self.hero_cpp)

    def test_boss_health_and_attack_warning_contract(self):
        for name in ("BossMaxHealth", "BossWarningEndTime", "BossPhase"):
            self.assertIn(name, self.enemy_h)
            self.assertIn("Boss->" + name, self.hud)
        self.assertIn("void AMoonfallEnemy::BeginPlay()", self.enemy_cpp)
        self.assertIn("Hero->AwardEquipmentDrop", self.enemy_cpp)

    def test_equipment_stats_affect_combat(self):
        for name in ("WeaponTier", "ArmorTier", "AccessoryTier"):
            self.assertIn(name, self.hero_h)
            self.assertIn(name, self.hero_cpp)
        self.assertIn("InventoryItemCodes.Num() >= 30", self.hero_cpp)

    def test_dodge_and_blocked_hit_checks_retained(self):
        self.assertIn("GetSafeNormal2D()", self.enemy_cpp)
        self.assertIn("LineTraceSingleByChannel", self.enemy_cpp)
        self.assertIn("ApplyMoonfallDamage", self.enemy_cpp)

if __name__ == "__main__":
    unittest.main()
