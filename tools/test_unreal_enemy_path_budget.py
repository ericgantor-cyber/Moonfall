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

    def test_enemy_ai_reuses_weak_player_reference(self):
        header = (SOURCE.parent / "MoonfallEnemy.h").read_text(encoding="utf-8")
        self.assertIn("TWeakObjectPtr<AMoonfallHeroPawn> CachedHero;", header)
        self.assertIn("class AMoonfallHeroPawn;", header)
        tick = self.source.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("AMoonfallHeroPawn* Hero = CachedHero.Get();", tick)
        self.assertIn("if (!IsValid(Hero))", tick)
        self.assertIn("CachedHero = Hero;", tick)
        self.assertIn("const AMoonfallHeroPawn* Viewer = bBoss ? Hero : nullptr;", tick)

    def test_hero_cache_refreshes_after_possession_switch(self):
        header = (SOURCE.parent / "MoonfallEnemy.h").read_text(encoding="utf-8")
        self.assertIn("float NextHeroCacheRefreshAt = 0.f;", header)
        tick = self.source.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("else if (Now >= NextHeroCacheRefreshAt)", tick)
        self.assertEqual(tick.count("NextHeroCacheRefreshAt = Now + 1.f;"), 2)
        self.assertEqual(tick.count("UGameplayStatics::GetPlayerPawn(this, 0)"), 2)

    def test_distant_sight_traces_keep_scouts_and_elites_responsive(self):
        self.assertIn('const bool bFastPerception = bElite || ActorHasTag(FName(TEXT("DungeonScout")));', self.source)
        self.assertIn("!bFastPerception && Distance > 1200.f ? 0.7f : 0.45f", self.source)
        self.assertIn("NextSightCheckAt = Now + SightInterval", self.source)

    def test_combat_safety_and_existing_sweep_remain(self):
        self.assertIn("AddActorWorldOffset(Direction.GetSafeNormal() * SafeChaseSpeed * ChaseMultiplier * SafeAIDelta, true, &Hit)", self.source)
        self.assertIn("const bool bInCombatArea = Distance < 2200.f;", self.source)
        self.assertIn("bWithinAttackArc && Distance <= AttackRange * CurrentAttackRangeMultiplier", self.source)
        self.assertNotIn("bWithinAttackArc && Difference.Size() <=", self.source)
        self.assertIn("if (!bBoss && !bCanSeeHero", self.source)

    def test_idle_mesh_updates_skip_identical_scale(self):
        self.assertIn("const FVector DesiredScale(", self.source)
        self.assertIn("Body->GetRelativeScale3D().Equals(DesiredScale, KINDA_SMALL_NUMBER)", self.source)
        self.assertIn("Body->SetRelativeScale3D(DesiredScale);", self.source)

if __name__ == "__main__":
    unittest.main()
