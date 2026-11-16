#!/usr/bin/env python3
"""Guard synchronization between the prototype talent JSON and Godot resources.

Source-level checks are not a substitute for running Godot engine smoke tests.
"""
import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "config/moonfall_talents.json"
GODOT = ROOT / "godot-v38/data/moonfall_talents.json"
SCRIPT = ROOT / "godot-v38/scripts/systems/moonfall_talent_tree.gd"
SMOKE = ROOT / "godot-v38/tests/talent_tree_runtime.gd"


class GodotTalentBridgeTests(unittest.TestCase):
    def test_godot_bundles_exact_reference_tree(self):
        self.assertEqual(json.loads(REFERENCE.read_text(encoding="utf-8")),
                         json.loads(GODOT.read_text(encoding="utf-8")))

    def test_godot_script_loads_bundled_data_and_provides_full_respec(self):
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn('res://data/moonfall_talents.json', source)
        self.assertIn('static func evaluate(', source)
        self.assertIn('static func spend(', source)
        self.assertIn('static func respec(', source)
        self.assertNotIn('static func refund(', source)
        self.assertIn('"Missing talent prerequisite"', source)
        self.assertIn('"Not enough talent points"', source)
        self.assertIn('return evaluate(level, {})', source)

    def test_native_smoke_test_covers_purchase_and_reset(self):
        source = SMOKE.read_text(encoding="utf-8")
        for fragment in ('TALENTS.evaluate(', 'TALENTS.spend(', 'TALENTS.respec(',
                         'MOONFALL TALENT TREE RUNTIME PASS', 'quit(1)'):
            self.assertIn(fragment, source)


if __name__ == "__main__":
    unittest.main()
