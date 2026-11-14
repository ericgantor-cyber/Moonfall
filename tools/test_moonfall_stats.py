#!/usr/bin/env python3
"""Reference stat regression tests; not a substitute for engine builds."""
import copy
import math
import unittest

from moonfall_stats import calculate, load_config, validate_config


class ClasslessStatsTests(unittest.TestCase):
    def setUp(self):
        self.config = load_config()

    def test_base_level_has_no_growth(self):
        result = calculate(1, config=self.config)
        self.assertTrue(all(v == 0 for v in result["attributes"].values()))
        self.assertEqual(result["stats"], {k: float(v) for k, v in self.config["base"].items()})
        self.assertEqual(result["ruleset"], "classless_no_cards")

    def test_every_attribute_grows_automatically(self):
        result = calculate(10, config=self.config)
        self.assertEqual(result["attributes"], {k: 9 for k in self.config["attributes"]})
        self.assertEqual(result["stats"]["health"], 208.0)
        self.assertEqual(result["stats"]["mana"], 145.0)
        self.assertEqual(result["stats"]["stamina"], 136.0)
        self.assertAlmostEqual(result["stats"]["melee_damage"], 12.25)

    def test_gear_and_power_are_separate_from_growth(self):
        a = calculate(10, 0, config=self.config)
        b = calculate(10, 80, {"health": 25, "melee_damage": 2}, self.config)
        self.assertEqual(a["attributes"], b["attributes"])
        self.assertEqual(b["stats"]["health"] - a["stats"]["health"], 25)
        self.assertEqual(b["power"] - a["power"], 20)

    def test_rejects_invalid_scores_and_nan(self):
        for bad_level in (0, 101, 1.2, True):
            with self.assertRaises(ValueError):
                calculate(bad_level, config=self.config)
        for bad_gear in (-1, 5001, 1.1, False):
            with self.assertRaises(ValueError):
                calculate(1, bad_gear, config=self.config)
        for modifiers in ({"unknown": 2}, {"health": math.nan}, {"mana": math.inf}):
            with self.assertRaises(ValueError):
                calculate(1, gear_bonuses=modifiers, config=self.config)

    def test_rejects_negative_and_miswired_rules(self):
        broken = copy.deepcopy(self.config)
        broken["attributes"]["might"]["stat"] = "health"
        with self.assertRaises(ValueError):
            validate_config(broken)
        broken = copy.deepcopy(self.config)
        broken["auto_growth_per_level"]["might"] = -1
        with self.assertRaises(ValueError):
            validate_config(broken)

    def test_no_class_and_card_mechanics(self):
        forbidden = ("class_selection", "upgrade_card", "reroll")
        values = str(self.config).lower()
        self.assertTrue(all(word not in values for word in forbidden))
        self.assertEqual(calculate(5, config=self.config)["ruleset"], "classless_no_cards")


if __name__ == "__main__":
    unittest.main()
