"""Regression checks for bounded Moonfall enemy navigation refresh behavior."""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1]
          / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")

class EnemyPathBudgetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")

    def test_only_one_synchronous_path_query_still_cached(self):
        self.assertEqual(self.source.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertLess(
            self.source.index("if (!FMath::IsFinite(NextPathRefreshAt) || Now >= NextPathRefreshAt)"),
            self.source.index("Nav->FindPathToLocationSynchronously("),
        )

    def test_far_mobs_only_receive_lower_repath_frequency(self):
        self.assertIn("(!bBoss && !bElite && Distance > 1100.f) ? 0.85f : 0.5f", self.source)
        self.assertIn("NextPathRefreshAt = Now + PathRefreshBase", self.source)
        self.assertIn("GetUniqueID() % 5u", self.source)

    def test_combat_safety_and_existing_sweep_remain(self):
        self.assertIn("AddActorWorldOffset(Direction.GetSafeNormal() * SafeChaseSpeed * ChaseMultiplier * SafeAIDelta, true, &Hit)", self.source)
        self.assertIn("const bool bInCombatArea = Distance < 2200.f;", self.source)
        self.assertIn("if (!bBoss && !bCanSeeHero)", self.source)

if __name__ == "__main__":
    unittest.main()
