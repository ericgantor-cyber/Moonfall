#!/usr/bin/env python3
"""Deterministic design checks for Moonfall Unreal Stage 328 balance formulas.

Run: python3 tools/check_balance.py
These are mathematical checks, NOT Unreal gameplay, compile, or animation tests.
Update these formulas when changing corresponding C++ rules.
"""
import math
import unittest

LEVELS = (1, 10, 30, 50, 75, 100)

def enemy_hp_scale(level):
    x = max(1, min(100, level)) - 1
    return 1 + .035 * min(x, 30) + .018 * max(0, x - 30)

def enemy_damage_scale(level):
    x = max(1, min(100, level)) - 1
    return 1 + .012 * min(x, 30) + .006 * max(0, x - 30)

def xp_reward(level, kind):
    base = {"normal": 25, "elite": 50, "boss": 125}[kind]
    bonus = 1 + .018 * min(max(1, min(level, 100)) - 1, 40)
    # FMath::RoundToInt for nonnegative values.
    return math.floor(base * bonus + .5)

def spell_power(level):
    level = max(1, min(100, level))
    return 1 + .025 * min(level - 1, 30) + .012 * max(0, level - 31)

def baseline_casts_to_kill(level, base_hp=80, base_spell_damage=18):
    # Illustrative unmodified spell, no mana/cooldown/misses or armor.
    return math.ceil(base_hp * enemy_hp_scale(level) /
                     (base_spell_damage * spell_power(level)))

class BalanceRegression(unittest.TestCase):
    def test_monotonic_scaling(self):
        for scale in (enemy_hp_scale, enemy_damage_scale, spell_power):
            seq = [scale(n) for n in range(1, 101)]
            self.assertTrue(all(a <= b for a, b in zip(seq, seq[1:])))
            self.assertEqual(seq[0], 1)

    def test_xp_order_and_growth(self):
        for level in LEVELS:
            self.assertLess(xp_reward(level, "normal"), xp_reward(level, "elite"))
            self.assertLess(xp_reward(level, "elite"), xp_reward(level, "boss"))
        for kind in ("normal", "elite", "boss"):
            self.assertEqual(xp_reward(100, kind), xp_reward(41, kind))

    def test_spawn_snapshot_does_not_match_dynamic_relevel(self):
        # Enemy HP is computed at spawn, and must not be recalculated on level-up.
        spawn_level, later_level = 10, 30
        snapshot_hp = 80 * enemy_hp_scale(spawn_level)
        self.assertGreater(80 * enemy_hp_scale(later_level), snapshot_hp)
        self.assertAlmostEqual(snapshot_hp, 105.2)

    def test_expected_milestones(self):
        self.assertAlmostEqual(enemy_hp_scale(1), 1.0)
        self.assertAlmostEqual(enemy_hp_scale(31), 2.05)
        self.assertAlmostEqual(enemy_hp_scale(100), 3.292)
        self.assertAlmostEqual(enemy_damage_scale(31), 1.36)
        self.assertAlmostEqual(spell_power(31), 1.75)
        self.assertAlmostEqual(spell_power(100), 2.578)

    def test_casts_to_kill_not_explosive(self):
        # A narrow formula-level guard; real TTK requires playtesting.
        casts = [baseline_casts_to_kill(n) for n in LEVELS]
        self.assertTrue(all(1 <= count <= 15 for count in casts), casts)

if __name__ == "__main__":
    print("Level | HP x | Enemy damage x | Spell x | Normal XP | Casts to kill*")
    for level in LEVELS:
        print(f"{level:>5} | {enemy_hp_scale(level):>4.2f} |"
              f" {enemy_damage_scale(level):>14.2f} |"
              f" {spell_power(level):>7.2f} |"
              f" {xp_reward(level, 'normal'):>9} |"
              f" {baseline_casts_to_kill(level):>13}")
    print("*Illustrative 80 HP enemy, 18 damage spell; ignores cooldowns, mana and gear.")
    unittest.main()
