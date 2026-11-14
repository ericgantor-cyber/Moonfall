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

    def test_base_character_has_default_hit_crit_dodge(self):
        result = calculate(config=self.config)
        self.assertEqual(result["percentages"]["critical_strike"], 5)
        self.assertEqual(result["percentages"]["hit"], 90)
        self.assertEqual(result["percentages"]["dodge"], 5)
        self.assertEqual(result["flat"]["armor"], 0)
        self.assertEqual(result["ruleset"], "moonfall_classless_ratings")

    def test_gear_ratings_modify_combat_percentages(self):
        result = calculate({"critical_strike": 100, "hit": 90, "haste": 75}, config=self.config)
        self.assertEqual(result["percentages"]["critical_strike"], 10)
        self.assertEqual(result["percentages"]["hit"], 95)
        self.assertEqual(result["percentages"]["haste"], 3)

    def test_clamps_percentages_and_resistances(self):
        result = calculate({"hit": 10000, "critical_strike": 10000},
                           {"fire_resistance": 100, "armor": 100000}, self.config)
        self.assertEqual(result["percentages"]["hit"], 100)
        self.assertEqual(result["percentages"]["critical_strike"], 60)
        self.assertEqual(result["flat"]["fire_resistance"], 75)
        self.assertEqual(result["flat"]["armor"], 25000)

    def test_rejects_invalid_and_unsupported_input(self):
        for values in ({"invalid": 3}, {"hit": -1}, {"hit": True},
                       {"hit": 0.5}, {"hit": 10001}):
            with self.assertRaises(ValueError):
                calculate(values, config=self.config)
        for values in ({"invalid": 2}, {"armor": float("nan")},
                       {"armor": float("inf")}, {"armor": "5"}):
            with self.assertRaises(ValueError):
                calculate(flat_bonuses=values, config=self.config)
        with self.assertRaisesRegex(ValueError, "Total combat rating"):
            calculate({key: 10000 for key in self.config["ratings"]}, config=self.config)

    def test_invalid_conversion_config_rejected(self):
        bad = copy.deepcopy(self.config)
        bad["ratings"]["hit"]["rating_per_percent"] = 0
        with self.assertRaises(ValueError):
            validate(bad)
        bad = copy.deepcopy(self.config)
        bad["flat_stats"]["armor"]["max"] = -1
        with self.assertRaises(ValueError):
            validate(bad)

    def test_all_ratings_are_classless_and_optional(self):
        result = calculate(config=self.config)
        self.assertEqual(len(result["percentages"]), 6)
        self.assertEqual(len(result["flat"]), 8)
        self.assertNotIn("class", self.config)
        self.assertNotIn("upgrade_cards", self.config)
        self.assertNotIn("reroll", self.config)


if __name__ == "__main__":
    unittest.main()
