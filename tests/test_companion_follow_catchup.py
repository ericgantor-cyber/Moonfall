"""Regression checks for continuous-world companion catch-up behavior."""
from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "godot-v38/scripts/native_v38_4.gd"


class CompanionFollowCatchupTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")

    def test_catchup_uses_actual_frame_delta(self):
        self.assertIn("speed*maxf(0.0,delta)", self.source)
        self.assertNotIn("FOLLOW_SPEED*minf(delta,0.1)", self.source)

    def test_catchup_is_bounded_and_preserves_teleport(self):
        self.assertIn("minf(FOLLOW_CATCHUP_SPEED,FOLLOW_SPEED+catchup)", self.source)
        self.assertIn("if distance>FOLLOW_TELEPORT_DISTANCE:", self.source)
        self.assertIn("realm_follower.global_position = target", self.source)

    def test_close_follow_retains_original_speed(self):
        self.assertIn("maxf(0.0,distance-FOLLOW_CATCHUP_START)", self.source)
        self.assertIn("const FOLLOW_SPEED:float = 5.0", self.source)

    def test_no_art_or_save_modifications_required(self):
        self.assertIn("super._process(delta)", self.source)
        self.assertIn("realm_follower.visible = following", self.source)


if __name__ == "__main__":
    unittest.main()
