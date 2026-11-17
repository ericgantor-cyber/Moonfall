"""Source-level regressions for Moonfall's single-pass combat HUD enemy scan."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
HUD = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp"


class CombatHudScanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = HUD.read_text(encoding="utf-8")

    def test_only_one_enemy_world_iterator_in_draw_hud(self):
        hud = self.source.split("void AMoonfallCombatHUD::DrawHUD()", 1)[1]
        self.assertEqual(hud.count("TActorIterator<AMoonfallEnemy>"), 1)
        self.assertIn("for (const AMoonfallEnemy* Enemy : NearbyDamageEnemies)", hud)

    def test_nearby_damage_keeps_killing_blow_feedback(self):
        hud = self.source
        self.assertLess(hud.index("NearbyDamageEnemies.Add(E)"),
                        hud.index("if (E->Health <= 0.f) continue;"))
        self.assertIn("DistSq <= FMath::Square(6000.f)", hud)
        self.assertIn("if (VisibleDamageNumbers >= 32) break;", hud)
        self.assertIn("AMoonfallEnemy::DamageHistorySize", hud)

    def test_boss_and_posture_targets_remain_nearest(self):
        hud = self.source
        self.assertIn("if (DistSq < BossDistanceSq)", hud)
        self.assertIn("if (DistSq < PostureDistanceSq", hud)
        self.assertIn("E->PostureBrokenUntil > WorldTime", hud)
        self.assertIn("if (!FMath::IsFinite(DistSq)) continue;", hud)


if __name__ == "__main__":
    unittest.main()
