#!/usr/bin/env python3
"""Tests for Moonfall's classless WoW Forever-inspired combat stat model."""
import copy
import math
import unittest

from moonfall_combat_ratings import calculate, load, validate


class CombatRatingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.config = load()

    def test_base_character_has_crit_and_haste_only(self):
        result = calculate(config=self.config)
        self.assertEqual(result["percentages"]["critical_strike"], 5)
        self.assertEqual(result["percentages"]["haste"], 0)
        self.assertEqual(result["flat"]["armor"], 0)
        self.assertEqual(result["ruleset"], "moonfall_classless_ratings")

    def test_gear_ratings_modify_combat_percentages(self):
        result = calculate({"critical_strike": 100, "haste": 75}, config=self.config)
        self.assertEqual(result["percentages"]["critical_strike"], 10)
        self.assertEqual(result["percentages"]["haste"], 3)

    def test_clamps_percentages_and_resistances(self):
        result = calculate({"haste": 10000, "critical_strike": 10000},
                           {"fire_resistance": 100, "armor": 100000}, self.config)
        self.assertEqual(result["percentages"]["haste"], 50)
        self.assertEqual(result["percentages"]["critical_strike"], 60)
        self.assertEqual(result["flat"]["fire_resistance"], 75)
        self.assertEqual(result["flat"]["armor"], 25000)

    def test_rejects_invalid_and_unsupported_input(self):
        for values in ({"invalid": 3}, {"hit": 3}, {"dodge": 3},
                       {"block": 3}, {"parry": 3}, {"haste": -1},
                       {"haste": True}, {"haste": 0.5}, {"haste": 10001}):
            with self.assertRaises(ValueError):
                calculate(values, config=self.config)
        for values in ({"invalid": 2}, {"armor": float("nan")},
                       {"armor": float("inf")}, {"armor": "5"}):
            with self.assertRaises(ValueError):
                calculate(flat_bonuses=values, config=self.config)
        limited = copy.deepcopy(self.config)
        limited["limits"]["maximum_total_rating"] = 10000
        with self.assertRaisesRegex(ValueError, "Total combat rating"):
            calculate({"haste": 10000, "critical_strike": 10000}, config=limited)

    def test_invalid_conversion_config_rejected(self):
        bad = copy.deepcopy(self.config)
        bad["ratings"]["haste"]["rating_per_percent"] = 0
        with self.assertRaises(ValueError):
            validate(bad)
        bad = copy.deepcopy(self.config)
        bad["flat_stats"]["armor"]["max"] = -1
        with self.assertRaises(ValueError):
            validate(bad)

    def test_all_ratings_are_classless_and_optional(self):
        result = calculate(config=self.config)
        self.assertEqual(set(result["percentages"]), {"critical_strike", "haste"})
        self.assertEqual(len(result["flat"]), 8)
        self.assertNotIn("class", self.config)
        self.assertNotIn("upgrade_cards", self.config)
        self.assertNotIn("reroll", self.config)


if __name__ == "__main__":
    unittest.main()
