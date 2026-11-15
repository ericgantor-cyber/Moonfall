#!/usr/bin/env python3
"""Regression tests for the Moonfall classless talent-tree prototype."""
import copy
import unittest

from moonfall_talents import evaluate, load, respec, spend, validate


class TalentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tree = load()

    def test_141_nodes_three_open_branches(self):
        self.assertEqual(len(self.tree["nodes"]), 141)
        self.assertEqual({branch: sum(node["branch"] == branch for node in self.tree["nodes"])
                          for branch in ("might", "agility", "arcana")},
                         {"might": 47, "agility": 47, "arcana": 47})
        self.assertEqual(max(node["tier"] for node in self.tree["nodes"]), 14)
        self.assertEqual(len(self.tree["branches"]), 3)
        validate(self.tree)

    def test_every_expanded_talent_chain_is_reachable(self):
        nodes = {node["id"]: node for node in self.tree["nodes"]}
        self.assertEqual(len(nodes), 141)
        for branch in ("might", "agility", "arcana"):
            for leaf in [node for node in nodes.values()
                         if node["branch"] == branch and node["tier"] == 14]:
                unlocked = {}
                def purchase_dependencies(node_id):
                    for prerequisite in nodes[node_id]["requires"]:
                        purchase_dependencies(prerequisite)
                    unlocked[node_id] = 1
                purchase_dependencies(leaf["id"])
                result = evaluate(100, unlocked, self.tree)
                self.assertEqual(result["spent"], len(unlocked))
                self.assertIn(leaf["id"], result["allocations"])

    def test_new_33_talents_have_unique_ids_and_valid_gates(self):
        added = [node for node in self.tree["nodes"] if node["tier"] >= 12]
        self.assertEqual(len(added), 33)
        self.assertEqual({branch: sum(n["branch"] == branch for n in added)
                          for branch in ("might", "agility", "arcana")},
                         {"might": 11, "agility": 11, "arcana": 11})
        self.assertEqual({tier: sum(n["tier"] == tier for n in added)
                          for tier in (12, 13, 14)},
                         {12: 12, 13: 12, 14: 9})
        gates = {12: 86, 13: 91, 14: 96}
        self.assertTrue(all(n["min_level"] == gates[n["tier"]] for n in added))
        self.assertEqual(len({node["id"] for node in self.tree["nodes"]}), 141)

    def test_new_capstone_path_survives_full_respec(self):
        nodes = {n["id"]: n for n in self.tree["nodes"]}
        chosen = {}
        def unlock(node_id):
            for requirement in nodes[node_id]["requires"]:
                unlock(requirement)
            chosen[node_id] = 1
        unlock("might_eclipse_champion")
        state = evaluate(100, chosen, self.tree)
        self.assertIn("might_eclipse_champion", state["allocations"])
        self.assertLess(state["spent"], state["earned"])
        with self.assertRaisesRegex(ValueError, "level gate"):
            evaluate(95, chosen, self.tree)
        reset = respec(100, self.tree, current=chosen)
        self.assertEqual(reset["spent"], 0)
        self.assertEqual(reset["available"], 198)

    def test_new_tiers_respect_gates_and_full_respec(self):
        fresh = evaluate(16, {"iron_will": 1, "breaker": 1, "guard": 1,
                              "titan_strike": 1}, self.tree)
        self.assertTrue(fresh["available"] > 0)
        target = "might_ashen_grip"
        with self.assertRaisesRegex(ValueError, "level gate"):
            evaluate(15, {**fresh["allocations"], target: 1}, self.tree)
        purchased = spend(16, fresh["allocations"], target, self.tree)
        self.assertEqual(purchased["spent"], 5)
        assert purchased["bonuses"]["melee_damage"]["percent"] > fresh["bonuses"]["melee_damage"]["percent"]
        reset = respec(16, self.tree, current=purchased["allocations"])
        self.assertEqual(reset["spent"], 0)
        self.assertEqual(reset["available"], 30)

    def test_level_gives_points_without_auto_allocating_talents(self):
        state = evaluate(5, {}, self.tree)
        self.assertEqual((state["earned"], state["available"], state["spent"]), (8, 8, 0))
        self.assertEqual(state["allocations"], {})

    def test_two_points_per_level_and_multirank_roots(self):
        self.assertEqual(self.tree["points_per_level"], 2)
        for level, expected in ((1, 0), (2, 2), (5, 8), (10, 18), (100, 198)):
            self.assertEqual(evaluate(level, {}, self.tree)["available"], expected)
        self.assertEqual({n["id"]: n["max_rank"] for n in self.tree["nodes"] if n["tier"] == 1},
                         {"iron_will": 5, "swift_step": 5, "spirit_well": 5})
        built = evaluate(10, {"iron_will": 5, "breaker": 3}, self.tree)
        self.assertEqual(built["spent"], 8)
        self.assertAlmostEqual(built["bonuses"]["melee_damage"]["percent"], .27)

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
        self.assertEqual(state["available"], 17)
        self.assertEqual(state["bonuses"]["mana"]["flat"], 8)
        reset = respec(10, self.tree)
        self.assertEqual(reset["available"], 18)
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
        self.assertEqual(reset["available"], 18)
        self.assertEqual(reset["allocations"], {})
        self.assertEqual(original, {"iron_will": 1, "breaker": 1, "spirit_well": 1})

    def test_full_respec_checks_saved_allocations_without_mutation(self):
        original = {"iron_will": 1, "breaker": 1, "spirit_well": 1}
        reset = respec(10, self.tree, current=original)
        self.assertEqual(reset["spent"], 0)
        self.assertEqual(reset["available"], 18)
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
        self.assertEqual(fresh["available"], 18)
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
