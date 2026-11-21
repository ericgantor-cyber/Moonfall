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

    def test_partial_nav_routes_follow_reachable_waypoints(self):
        # Unreal may produce a useful partial route when the target is behind
        # an unreachable doorway. Follow its first corner rather than the
        # projected destination that may be on the opposite side of a wall.
        path = self.source.split("UNavigationPath* Path = Nav->FindPathToLocationSynchronously(", 1)[1].split(
            "if (CachedWaypoint.ContainsNaN())", 1)[0]
        self.assertIn("bNavigationRouteUnavailable = !IsValid(Path) || Path->PathPoints.Num() <= 1", path)
        self.assertIn("? Path->PathPoints[1] : FVector::ZeroVector;", path)
        self.assertNotIn("!Path->IsPartial()", path)
        # Preserve one cached query with no extra navigation probes.
        self.assertEqual(self.source.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(self.source.count("Nav->ProjectPointToNavigation("), 1)
        self.assertLess(path.index("!IsValid(Path) ||"), path.index("Path->PathPoints[1]"))
        self.assertIn("NextPathRefreshAt = Now + PathRefreshBase", self.source)

    def test_nonfinite_nav_waypoints_fail_closed_without_repath_spam(self):
        # Unreal can return a path object whose next corner contains NaN or
        # infinity. Never feed such a corner to pursuit movement.
        nav = self.source.split("// Solve paths only when the cache expires.", 1)[1].split(
            "FVector Direction = Target - GetActorLocation();", 1
        )[0]
        self.assertIn("Path->PathPoints[1].ContainsNaN()", nav)
        self.assertLess(nav.index("Path->PathPoints[1].ContainsNaN()"),
                        nav.index("? Path->PathPoints[1] : FVector::ZeroVector;"))
        cached = nav.split("if (CachedWaypoint.ContainsNaN()) {", 1)[1].split(
            "if (bHasCachedNavigationWaypoint) {", 1
        )[0]
        self.assertIn("CachedWaypoint = FVector::ZeroVector;", cached)
        self.assertIn("bNavigationRouteUnavailable = true;", cached)
        self.assertIn("NextPathRefreshAt <= Now", cached)
        self.assertIn("NextPathRefreshAt = Now + 0.25f;", cached)
        self.assertNotIn("NextPathRefreshAt = 0.f;", cached)
        self.assertLess(cached.index("bNavigationRouteUnavailable = true;"),
                        cached.index("NextPathRefreshAt = Now + 0.25f;"))
        self.assertLess(nav.index("if (CachedWaypoint.ContainsNaN())"),
                        nav.index("if (bNavigationRouteUnavailable) return;"))
        self.assertEqual(nav.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(nav.count("Nav->ProjectPointToNavigation("), 1)

    def test_unreachable_navmesh_goal_does_not_trigger_direct_wall_chase(self):
        # A target projection alone does not guarantee a reachable path.
        # Refuse direct pursuit until cached navigation finds a real corner.
        header = (SOURCE.parent / "MoonfallEnemy.h").read_text(encoding="utf-8")
        self.assertIn("bool bNavigationRouteUnavailable = false;", header)
        nav = self.source.split("// Solve paths only when the cache expires.", 1)[1].split(
            "FVector Direction = Target - GetActorLocation();", 1)[0]
        self.assertIn("bNavigationRouteUnavailable = !IsValid(Path) || Path->PathPoints.Num() <= 1", nav)
        self.assertIn("bNavigationRouteUnavailable = true;", nav)
        self.assertIn("CachedWaypoint = FVector::ZeroVector;", nav)
        self.assertIn("bNavigationRouteUnavailable = false;", nav)
        self.assertIn("if (bNavigationRouteUnavailable) return;", nav)
        self.assertNotIn("CachedWaypoint = Target;", nav)
        self.assertLess(nav.index("Nav->FindPathToLocationSynchronously("),
                        nav.index("if (bNavigationRouteUnavailable) return;"))
        # This change adds no queries or collision sweeps and preserves
        # the original timed cache even for missing/invalid nav routes.
        self.assertIn("NextPathRefreshAt = Now + PathRefreshBase", nav)
        self.assertEqual(nav.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(nav.count("Nav->ProjectPointToNavigation("), 1)
        self.assertEqual(nav.count("AddActorWorldOffset("), 0)

    def test_reaching_partial_corner_never_restores_direct_wall_chase(self):
        # A waypoint can be close enough to trigger repathing without being
        # the hero's reachable destination. On that tick, keep steering along
        # the corner rather than reverting to the far target behind a wall.
        waypoint = self.source.split("if (bHasCachedNavigationWaypoint) {", 1)[1].split(
            "} else {\n   // Preserve direct chase without NavSystem", 1
        )[0]
        self.assertIn("Target = CachedWaypoint;", waypoint)
        self.assertIn("if (ToWaypoint.SizeSquared() < FMath::Square(65.f))", waypoint)
        self.assertIn("NextPathRefreshAt = Now + 0.25f;", waypoint)
        self.assertIn("NextPathRefreshAt > Now + 0.25f", waypoint)
        self.assertIn("!FMath::IsFinite(NextPathRefreshAt)", waypoint)
        self.assertLess(waypoint.index("Target = CachedWaypoint;"),
                        waypoint.index("if (ToWaypoint.SizeSquared()"))
        self.assertLess(waypoint.index("if (ToWaypoint.SizeSquared()"),
                        waypoint.index("NextPathRefreshAt = Now + 0.25f;"))
        self.assertNotIn("NextPathRefreshAt = 0.f;", waypoint)
        self.assertNotIn("} else {\n     Target = ", waypoint)
        # Refresh is scheduled, not performed again in the same frame.
        self.assertEqual(self.source.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(self.source.count("Nav->ProjectPointToNavigation("), 1)

    def test_world_origin_navigation_corner_is_a_valid_waypoint(self):
        # A reachable (0,0,0) waypoint must not be mistaken for a missing
        # cache. Validity comes from path results, not vector magnitude.
        header = (SOURCE.parent / "MoonfallEnemy.h").read_text(encoding="utf-8")
        self.assertIn("bool bHasCachedNavigationWaypoint = false;", header)
        nav = self.source.split("// Solve paths only when the cache expires.", 1)[1].split(
            "FVector Direction = Target - GetActorLocation();", 1
        )[0]
        self.assertIn("bHasCachedNavigationWaypoint = !bNavigationRouteUnavailable;", nav)
        self.assertIn("CachedWaypoint = bHasCachedNavigationWaypoint", nav)
        self.assertIn("if (bHasCachedNavigationWaypoint) {", nav)
        self.assertIn("Target = CachedWaypoint;", nav)
        self.assertNotIn("if (!CachedWaypoint.IsNearlyZero())", nav)
        self.assertLess(nav.index("bHasCachedNavigationWaypoint = !bNavigationRouteUnavailable;"),
                        nav.index("if (bHasCachedNavigationWaypoint) {"))
        # All existing cache invalidations clear the explicit validity bit.
        lines = self.source.splitlines()
        resets = [index for index, line in enumerate(lines)
                  if line.strip() == "CachedWaypoint = FVector::ZeroVector;"]
        self.assertGreaterEqual(len(resets), 15)
        for index in resets:
            self.assertGreater(index, 0)
            self.assertEqual(lines[index - 1].strip(),
                             "bHasCachedNavigationWaypoint = false;")
        # Losing NavSystem discards the old waypoint but keeps the original
        # direct-chase fallback and allows immediate refresh when Nav returns.
        no_nav = nav.split("// Preserve direct chase without NavSystem", 1)[1].split(
            "if (bNavigationRouteUnavailable) return;", 1
        )[0]
        self.assertIn("bNavigationRouteUnavailable = false;", no_nav)
        self.assertIn("bHasCachedNavigationWaypoint = false;", no_nav)
        self.assertIn("NextPathRefreshAt = 0.f;", no_nav)
        self.assertEqual(self.source.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_partial_path_retry_deadline_is_not_sliding(self):
        waypoint = self.source.split("if (bHasCachedNavigationWaypoint) {", 1)[1].split(
            "} else {\n   // Preserve direct chase without NavSystem", 1
        )[0]
        arrived = waypoint.split("if (ToWaypoint.SizeSquared() < FMath::Square(65.f))", 1)[1]
        # The strict deadline comparison ensures repeated 0.1-second AI
        # ticks do not postpone an already scheduled retry forever.
        self.assertIn("|| NextPathRefreshAt > Now + 0.25f)", arrived)
        self.assertIn("NextPathRefreshAt = Now + 0.25f;", arrived)
        self.assertLess(arrived.index("NextPathRefreshAt > Now + 0.25f"),
                        arrived.index("NextPathRefreshAt = Now + 0.25f;"))
        # A pre-existing urgent retry stays urgent; invalid deadlines
        # become short bounded retries instead of immediate query loops.
        def next_retry(now, scheduled):
            if scheduled != scheduled or scheduled > now + 0.25:
                return now + 0.25
            return scheduled
        scheduled = next_retry(5.0, 5.8)
        for now in (5.1, 5.2):
            self.assertEqual(next_retry(now, scheduled), scheduled)
        self.assertEqual(next_retry(5.0, 0.0), 0.0)
        self.assertEqual(next_retry(5.0, float("nan")), 5.25)
        self.assertEqual(self.source.count("Nav->FindPathToLocationSynchronously("), 1)

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
        self.assertIn("if (!IsValid(Hero) || !Hero->IsPlayerControlled())", tick)
        self.assertIn("CachedHero = Hero;", tick)
        self.assertIn("const AMoonfallHeroPawn* Viewer = bBoss ? Hero : nullptr;", tick)

    def test_character_and_resident_panels_cancel_enemy_windups(self):
        tick = self.source.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        menu_guard = tick.split("// Pause hostile behavior during any gameplay modal, including Atlas, and", 1)[1].split(
            "// A corrupt nonfinite windup", 1
        )[0]
        self.assertIn("Hero->bOptionsOpen || Hero->bInventoryOpen", menu_guard)
        self.assertIn("Hero->bAtlasOpen", menu_guard)
        self.assertIn("Hero->bCharacterStatsOpen || Hero->bResidentContextOpen", menu_guard)
        self.assertIn("CancelDivineSignature();", menu_guard)
        self.assertIn("WindupEndsAt = -1.f;", menu_guard)
        self.assertIn("bComboSecondHitPending = false;", menu_guard)
        self.assertIn("NextAttackAt = FMath::Max(", menu_guard)

    def test_unpossessed_old_pawn_is_not_retained_as_target(self):
        tick = self.source.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("if (!IsValid(Hero) || !Hero->IsPlayerControlled())", tick)
        self.assertLess(tick.index("!Hero->IsPlayerControlled()"),
                        tick.index("Hero->bOptionsOpen || Hero->bInventoryOpen"))

    def test_hero_cache_refreshes_after_possession_switch(self):
        header = (SOURCE.parent / "MoonfallEnemy.h").read_text(encoding="utf-8")
        self.assertIn("float NextHeroCacheRefreshAt = 0.f;", header)
        tick = self.source.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("else if (Now >= NextHeroCacheRefreshAt)", tick)
        self.assertEqual(tick.count("NextHeroCacheRefreshAt = Now + 1.f"), 2)
        self.assertIn("NextHeroCacheRefreshAt = GetWorld()->GetTimeSeconds() + 1.f", self.source)
        self.assertEqual(self.source.count("0.05f * static_cast<float>(GetUniqueID() % 7u)"), 3)
        self.assertEqual(tick.count("UGameplayStatics::GetPlayerPawn(this, 0)"), 2)

    def test_distant_sight_traces_keep_scouts_and_elites_responsive(self):
        perception = self.source.split("const bool bFastPerception =", 1)[1].split(";", 1)[0]
        self.assertIn("bElite ||", perception)
        self.assertIn('ActorHasTag(FName(TEXT("DungeonScout")))', perception)
        self.assertIn('ActorHasTag(FName(TEXT("MoonfallDungeonEncounter")))', perception)
        self.assertIn("&&", perception)
        self.assertLess(perception.index('ActorHasTag(FName(TEXT("DungeonScout")))'),
                        perception.index('ActorHasTag(FName(TEXT("MoonfallDungeonEncounter")))'))
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
