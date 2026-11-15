#!/usr/bin/env python3
"""Source-level contract for mouse-first Atlas interactions (not a runtime test)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "godot-v38" / "scripts"


class AtlasMouseControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.chart = (ROOT / "systems" / "navigation_map.gd").read_text(encoding="utf-8")
        cls.screen = (ROOT / "native_v38_2.gd").read_text(encoding="utf-8")

    def test_wheel_zoom_and_left_click_waypoint_retained(self):
        self.assertIn("MOUSE_BUTTON_WHEEL_UP", self.chart)
        self.assertIn("MOUSE_BUTTON_WHEEL_DOWN", self.chart)
        self.assertIn("zoom_requested.emit(", self.chart)
        self.assertIn("MOUSE_BUTTON_LEFT", self.chart)
        self.assertIn("waypoint_selected.emit(location)", self.chart)

    def test_right_click_clears_waypoint(self):
        self.assertIn("signal waypoint_cleared", self.chart)
        self.assertIn("event.button_index==MOUSE_BUTTON_RIGHT", self.chart)
        self.assertIn("waypoint_cleared.emit()", self.chart)
        self.assertIn('atlas_chart.connect("waypoint_cleared",_atlas_clear)', self.screen)

    def test_no_plus_minus_zoom_buttons(self):
        self.assertNotIn('"ZOOM +"', self.screen)
        self.assertNotIn('"ZOOM -"', self.screen)
        self.assertIn('atlas_chart.connect("zoom_requested",_atlas_zoom)', self.screen)


if __name__ == "__main__":
    unittest.main()
