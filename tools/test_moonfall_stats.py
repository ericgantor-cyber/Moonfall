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

    def test_malformed_stat_config_reports_value_error(self):
        # Corrupted configuration should not leak AttributeError/TypeError.
        for broken in (None, [], "", {"schema_version": 1}):
            with self.subTest(broken=broken), self.assertRaises(ValueError):
                validate_config(broken)
        for key, values in {
            "attributes": (None, [], "invalid", {"might": {}}),
            "auto_growth_per_level": (None, [], "invalid"),
            "base": (None, [], "invalid"),
            "power": (None, [], "invalid"),
            "limits": (None, [], "invalid"),
        }.items():
            for value in values:
                broken = copy.deepcopy(self.config)
                broken[key] = value
                with self.subTest(field=key, value=value), self.assertRaises(ValueError):
                    validate_config(broken)
        broken = copy.deepcopy(self.config)
        broken["attributes"]["might"] = None
        with self.assertRaisesRegex(ValueError, "Invalid attribute rule"):
            validate_config(broken)

    def test_stat_levels_cannot_exceed_talent_tree_limits(self):
        for minimum, maximum in ((0, 100), (1, 101), (0, 101)):
            broken = copy.deepcopy(self.config)
            broken["limits"]["min_level"] = minimum
            broken["limits"]["max_level"] = maximum
            with self.subTest(minimum=minimum, maximum=maximum):
                with self.assertRaisesRegex(ValueError, "talent tree level range"):
                    validate_config(broken)
                with self.assertRaises(ValueError):
                    calculate(maximum, config=broken)
        # Both boundaries of a valid range are supported.
        self.assertEqual(calculate(1, config=self.config)["talents"]["level"], 1)
        self.assertEqual(calculate(100, config=self.config)["talents"]["level"], 100)

    def test_unified_character_sheet_includes_combat_ratings(self):
        result = calculate(
            10, 80, {"health": 25}, self.config,
            combat_ratings={"haste": 75, "critical_strike": 100},
            combat_bonuses={"armor": 250, "fire_resistance": 15},
        )
        self.assertEqual(result["stats"]["health"], 233)
        self.assertEqual(result["combat"]["percentages"]["haste"], 3)
        self.assertEqual(result["combat"]["percentages"]["critical_strike"], 10)
        self.assertEqual(result["combat"]["flat"]["armor"], 250)
        self.assertEqual(result["combat"]["flat"]["fire_resistance"], 15)
        self.assertEqual(result["power"], 40)

    def test_invalid_combat_ratings_fail_the_unified_sheet(self):
        with self.assertRaises(ValueError):
            calculate(10, config=self.config, combat_ratings={"haste": -2})
        for removed in ("hit", "dodge", "block", "parry"):
            with self.assertRaises(ValueError):
                calculate(10, config=self.config, combat_ratings={removed: 10})

    def test_equipped_gear_and_talents_stack_once(self):
        # At level 5, Might grows by four (10 * 1.1 = 11 melee damage).
        # Iron Will gives +3% and Spirit Well adds 8 mana.
        result = calculate(
            5, gear_bonuses={"melee_damage": 2},
            talent_allocations={"iron_will": 1, "spirit_well": 1},
            config=self.config,
        )
        self.assertAlmostEqual(result["stats"]["melee_damage"], (11 + 2) * 1.03)
        self.assertEqual(result["stats"]["mana"], 128)
        self.assertEqual(result["talents"]["spent"], 2)
        self.assertEqual(result["talents"]["available"], 2)
        self.assertEqual(result["combat"]["percentages"]["haste"], 0)

    def test_invalid_talents_rejected_by_unified_character_sheet(self):
        for allocations in ({"breaker": 1}, {"iron_will": True},
                            {"iron_will": 6}, {"unknown": 1}):
            with self.assertRaises(ValueError):
                calculate(5, config=self.config, talent_allocations=allocations)
        with self.assertRaisesRegex(ValueError, "Not enough talent points"):
            calculate(1, config=self.config, talent_allocations={"iron_will": 1})

    def test_no_talent_allocation_preserves_previous_base_stats(self):
        original = calculate(10, config=self.config)
        self.assertEqual(original["stats"]["health"], 208)
        self.assertEqual(original["stats"]["mana"], 145)
        self.assertEqual(original["stats"]["stamina"], 136)
        self.assertEqual(original["talents"]["spent"], 0)

    def test_empty_wrong_type_gear_bonuses_are_rejected(self):
        for wrong in ([], "", (), False):
            with self.assertRaisesRegex(ValueError, "Unknown gear bonus stat"):
                calculate(10, gear_bonuses=wrong, config=self.config)

    def test_no_class_and_card_mechanics(self):
        # Policy text is allowed to mention forbidden mechanics to explicitly
        # prohibit them. Check the actual machine-readable configuration keys.
        forbidden = {"class_selection", "upgrade_cards", "reroll_options"}
        self.assertFalse(forbidden.intersection(self.config.keys()))
        self.assertFalse(forbidden.intersection(self.config["attributes"].keys()))
        self.assertIn("No upgrade cards", self.config["rules"][1])
        self.assertEqual(calculate(5, config=self.config)["ruleset"], "classless_no_cards")


if __name__ == "__main__":
    unittest.main()
