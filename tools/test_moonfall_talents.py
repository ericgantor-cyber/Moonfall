#!/usr/bin/env python3
"""Regression tests for the Moonfall classless talent-tree prototype."""
import copy
import unittest

from moonfall_talents import evaluate, load, respec, spend, validate


class TalentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tree = load()

    def test_twelve_nodes_three_open_branches(self):
        self.assertEqual(len(self.tree["nodes"]), 12)
        self.assertEqual(len(self.tree["branches"]), 3)
        validate(self.tree)

    def test_level_gives_points_without_auto_allocating_talents(self):
        state = evaluate(5, {}, self.tree)
        self.assertEqual((state["earned"], state["available"], state["spent"]), (4, 4, 0))
        self.assertEqual(state["allocations"], {})

    def test_prerequisite_and_level_gate(self):
        with self.assertRaisesRegex(ValueError, "Missing talent prerequisite"):
            evaluate(5, {"breaker": 1}, self.tree)
        with self.assertRaisesRegex(ValueError, "level gate"):
            evaluate(2, {"iron_will": 1, "breaker": 1}, self.tree)
        state = spend(5, {"iron_will": 1}, "breaker", self.tree)
        self.assertEqual(state["spent"], 2)
        self.assertAlmostEqual(state["bonuses"]["melee_damage"]["percent"], .07)

    def test_spending_and_respec(self):
        state = spend(10, {}, "spirit_well", self.tree)
        self.assertEqual(state["available"], 8)
        self.assertEqual(state["bonuses"]["mana"]["flat"], 8)
        reset = respec(10, self.tree)
        self.assertEqual(reset["available"], 9)
        self.assertEqual(reset["bonuses"]["mana"]["flat"], 0)
        self.assertEqual(reset["allocations"], {})

    def test_rejects_invalid_budget_ranks_and_nodes(self):
        with self.assertRaisesRegex(ValueError, "Not enough talent points"):
            evaluate(1, {"iron_will": 1}, self.tree)
        for choice in ({"iron_will": 6}, {"iron_will": -1}, {"iron_will": True}):
            with self.assertRaisesRegex(ValueError, "Invalid allocated rank"):
                evaluate(10, choice, self.tree)
        with self.assertRaisesRegex(ValueError, "Unknown talent choice"):
            spend(5, {}, "unknown", self.tree)

    def test_multirank_caps_and_scaled_bonuses(self):
        limits = {1: 5, 2: 3, 3: 1}
        for node in self.tree["nodes"]:
            self.assertEqual(node["max_rank"], limits[node["tier"]])
        self.assertEqual(sum(n["max_rank"] for n in self.tree["nodes"]), 36)
        state = evaluate(10, {"iron_will": 5, "breaker": 3}, self.tree)
        self.assertEqual((state["spent"], state["available"]), (8, 1))
        self.assertAlmostEqual(state["bonuses"]["melee_damage"]["percent"], .27)
        with self.assertRaisesRegex(ValueError, "Invalid allocated rank"):
            spend(10, state["allocations"], "breaker", self.tree)
        with self.assertRaisesRegex(ValueError, "Not enough talent points"):
            evaluate(5, {"iron_will": 5}, self.tree)
        reset = respec(10, self.tree, current=state["allocations"])
        self.assertEqual((reset["spent"], reset["available"]), (0, 9))

    def test_multirank_spend_progression(self):
        state = evaluate(10, {}, self.tree)
        for rank in range(1, 6):
            state = spend(10, state["allocations"], "spirit_well", self.tree)
            self.assertEqual(state["allocations"]["spirit_well"], rank)
            self.assertEqual(state["bonuses"]["mana"]["flat"], 8 * rank)
        with self.assertRaisesRegex(ValueError, "Invalid allocated rank"):
            spend(10, state["allocations"], "spirit_well", self.tree)

    def test_rejects_bad_prerequisites(self):
        bad = copy.deepcopy(self.tree)
        bad["nodes"][1]["requires"] = ["nonexistent"]
        with self.assertRaisesRegex(ValueError, "Unknown or self"):
            validate(bad)
        bad = copy.deepcopy(self.tree)
        bad["nodes"][0]["requires"] = ["breaker"]
        with self.assertRaisesRegex(ValueError, "earlier tier"):
            validate(bad)

    def test_malformed_tree_data_raises_value_error_not_runtime_crashes(self):
        broken_values = (None, [], "not a tree", {"schema_version": 1})
        for broken in broken_values:
            with self.subTest(broken=broken), self.assertRaises(ValueError):
                validate(broken)
        for branch in (None, "might", {}, {"id": []}, {"id": ""}):
            broken = copy.deepcopy(self.tree)
            broken["branches"][0] = branch
            with self.subTest(branch=branch), self.assertRaises(ValueError):
                validate(broken)
        for node in (None, "node", {}, {"id": []}, {"id": ""}):
            broken = copy.deepcopy(self.tree)
            broken["nodes"][0] = node
            with self.subTest(node=node), self.assertRaises(ValueError):
                validate(broken)
        for effect in (None, [], "damage"):
            broken = copy.deepcopy(self.tree)
            broken["nodes"][0]["effect"] = effect
            with self.subTest(effect=effect), self.assertRaises(ValueError):
                validate(broken)
        for prerequisite in ({"id": "iron_will"}, ["iron_will"], None, 1):
            broken = copy.deepcopy(self.tree)
            broken["nodes"][1]["requires"] = [prerequisite]
            with self.subTest(prerequisite=prerequisite), self.assertRaises(ValueError):
                validate(broken)

    def test_spend_rejects_malformed_and_invalid_existing_state(self):
        for bad_state in (None, [], "", False):
            with self.assertRaisesRegex(ValueError, "Allocations must be a map"):
                spend(5, bad_state, "iron_will", self.tree)
        # A corrupted save must not be silently repaired by buying a missing
        # prerequisite with the next point.
        with self.assertRaisesRegex(ValueError, "Missing talent prerequisite"):
            spend(5, {"breaker": 1}, "iron_will", self.tree)
        with self.assertRaisesRegex(ValueError, "Invalid allocated rank"):
            spend(5, {"iron_will": True}, "spirit_well", self.tree)
        with self.assertRaisesRegex(ValueError, "Unknown talent choice"):
            spend(5, {"retired_node": 1}, "iron_will", self.tree)

    def test_spend_canonicalizes_zero_rank_from_saved_data(self):
        old = {"iron_will": 0}
        state = spend(5, old, "spirit_well", self.tree)
        self.assertEqual(state["allocations"], {"spirit_well": 1})
        self.assertEqual(old, {"iron_will": 0})
        self.assertEqual(state["spent"], 1)

    def test_full_respec_restores_all_points_without_individual_refunds(self):
        original = {"iron_will": 1, "breaker": 1, "spirit_well": 1}
        state = evaluate(10, original, self.tree)
        self.assertEqual(state["spent"], 3)
        reset = respec(10, self.tree)
        self.assertEqual(reset["spent"], 0)
        self.assertEqual(reset["available"], 9)
        self.assertEqual(reset["allocations"], {})
        self.assertEqual(original, {"iron_will": 1, "breaker": 1, "spirit_well": 1})

    def test_full_respec_checks_saved_allocations_without_mutation(self):
        original = {"iron_will": 1, "breaker": 1, "spirit_well": 1}
        reset = respec(10, self.tree, current=original)
        self.assertEqual(reset["spent"], 0)
        self.assertEqual(reset["available"], 9)
        self.assertEqual(reset["allocations"], {})
        self.assertEqual(original, {"iron_will": 1, "breaker": 1, "spirit_well": 1})

    def test_full_respec_rejects_invalid_saved_state(self):
        for malformed in (None, [], "", (), False):
            with self.assertRaisesRegex(ValueError, "Allocations must be a map"):
                respec(10, self.tree, current=malformed)
        for corrupt in ({"breaker": 1},
                        {"iron_will": True},
                        {"retired_talent": 1},
                        {"iron_will": -1}):
            with self.assertRaises(ValueError):
                respec(10, self.tree, current=corrupt)
        with self.assertRaises(ValueError):
            respec(1, self.tree, current={"iron_will": 1})

    def test_respec_omitted_current_remains_compatible(self):
        fresh = respec(10, self.tree)
        self.assertEqual(fresh["available"], 9)
        self.assertEqual(fresh["allocations"], {})
        with self.assertRaisesRegex(ValueError, "Allocations must be a map"):
            respec(10, self.tree, current=None)


    def test_no_classes_or_cards(self):
        self.assertEqual(evaluate(5, data=self.tree)["ruleset"], "classless_talent_tree")
        self.assertEqual({b["id"] for b in self.tree["branches"]},
                         {"might", "agility", "arcana"})
        self.assertTrue(all("class" not in node["id"] and "card" not in node["id"]
                            for node in self.tree["nodes"]))


if __name__ == "__main__":
    unittest.main()
