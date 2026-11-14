#!/usr/bin/env python3
"""Negative regression tests for the nonbreaking Moonfall category registry."""
import copy
import json
import unittest

from check_game_categories import MANIFEST, ROOT, validate


class CategoryRoadmapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.baseline = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def fresh(self):
        return copy.deepcopy(self.baseline)

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
