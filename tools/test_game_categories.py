#!/usr/bin/env python3
"""Negative regression tests for the nonbreaking Moonfall category registry."""
import copy
import json
import unittest

from check_game_categories import MANIFEST, ROOT, validate
from moonfall_category_plan import build_plan, ordered_tracks, render_markdown


class CategoryRoadmapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.baseline = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def fresh(self):
        return copy.deepcopy(self.baseline)

    def test_all_tracks_are_scheduled_once_after_prerequisites(self):
        plan = build_plan(self.fresh())
        self.assertEqual(plan["total"], 25)
        seen = set()
        for item in plan["tracks"]:
            self.assertNotIn(item["id"], seen)
            self.assertTrue(set(item["depends_on"]).issubset(seen), item["id"])
            self.assertFalse(item["prerequisite_only"])
            seen.add(item["id"])

    def test_category_plan_includes_external_dependencies(self):
        tracks = ordered_tracks(self.fresh(), "interface")
        ids = [item["id"] for item in tracks]
        self.assertIn("map_discovery", ids)
        self.assertIn("world_layout", ids)
        self.assertIn("inventory_equipment", ids)
        self.assertIn("save_resilience", ids)
        self.assertTrue(next(item for item in tracks if item["id"] == "map_discovery")[
            "prerequisite_only"
        ])
        self.assertFalse(next(item for item in tracks if item["id"] == "menus_map")[
            "prerequisite_only"
        ])
        self.assertLess(ids.index("map_discovery"), ids.index("menus_map"))
        self.assertLess(ids.index("world_layout"), ids.index("map_discovery"))

    def test_workplan_is_deterministic_and_readable(self):
        a = build_plan(self.fresh(), category="combat")
        b = build_plan(self.fresh(), category="combat")
        self.assertEqual(a, b)
        markdown = render_markdown(a["tracks"], "combat")
        self.assertIn("## Foundation", markdown)
        self.assertIn("## Integration", markdown)
        self.assertIn("## Polish", markdown)
        self.assertIn("Prerequisite", markdown)
        self.assertIn("not_verified", markdown)

    def test_unknown_category_is_not_silently_accepted(self):
        with self.assertRaisesRegex(ValueError, "Unknown category"):
            ordered_tracks(self.fresh(), "mystery")

    def test_real_registry_valid(self):
        validate(self.fresh(), ROOT)

    def test_rejects_missing_category(self):
        data = self.fresh()
        data["categories"].pop()
        with self.assertRaisesRegex(ValueError, "exactly five"):
            validate(data, ROOT)

    def test_rejects_duplicate_file_owner(self):
        data = self.fresh()
        data["categories"][1]["owners"].append(data["categories"][0]["owners"][0])
        with self.assertRaisesRegex(ValueError, "Duplicate file owner"):
            validate(data, ROOT)

    def test_rejects_missing_source(self):
        data = self.fresh()
        data["categories"][0]["owners"][0] = "missing/deliberately-absent.file"
        with self.assertRaisesRegex(ValueError, "Missing owner source file"):
            validate(data, ROOT)

    def test_rejects_parent_directory_traversal(self):
        data = self.fresh()
        data["categories"][0]["owners"][0] = "../outside.file"
        with self.assertRaisesRegex(ValueError, "Unsafe owner path"):
            validate(data, ROOT)

    def test_rejects_duplicate_track(self):
        data = self.fresh()
        data["categories"][1]["roadmap"][0]["id"] = "hero_identity"
        with self.assertRaisesRegex(ValueError, "Duplicate roadmap track"):
            validate(data, ROOT)

    def test_rejects_unknown_dependency(self):
        data = self.fresh()
        data["categories"][0]["roadmap"][0]["depends_on"] = ["missing_feature"]
        with self.assertRaisesRegex(ValueError, "Unknown or self dependency"):
            validate(data, ROOT)

    def test_rejects_circular_dependency(self):
        data = self.fresh()
        # Both tracks are in the foundation milestone.
        data["categories"][0]["roadmap"][0]["depends_on"] = ["races_classes"]
        with self.assertRaisesRegex(ValueError, "Cyclic roadmap dependency"):
            validate(data, ROOT)

    def test_rejects_future_milestone_dependency(self):
        data = self.fresh()
        data["categories"][0]["roadmap"][0]["depends_on"] = ["crafting"]
        with self.assertRaisesRegex(ValueError, "Later milestone dependency"):
            validate(data, ROOT)

    def test_rejects_unverified_completion_claim(self):
        data = self.fresh()
        data["categories"][0]["roadmap"][0]["verification"] = "complete"
        with self.assertRaisesRegex(ValueError, "Do not claim unverified"):
            validate(data, ROOT)


if __name__ == "__main__":
    unittest.main()
