"""Enemy-only WoW F-style combat floating text source regressions.

Source checks alone cannot prove native Unreal 5.4 build/playback.
"""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class FloatingCombatTextChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.enemy_h = (SRC / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.layout = (SRC / "MoonfallCombatFloatLayout.h").read_text(encoding="utf-8")

    def test_player_owns_no_incoming_text_queue_or_recording(self):
        for forbidden in ("IncomingCombatFloats", "IncomingCombatFloatCapacity",
                          "RecordIncomingCombatFloat", "NextIncomingCombatFloatSlot"):
            self.assertNotIn(forbidden, self.hero_h)
            self.assertNotIn(forbidden, self.hero)
            self.assertNotIn(forbidden, self.enemy)
            self.assertNotIn(forbidden, self.hud)
        self.assertIn("bool bShowFloatingCombatText = true;", self.hero_h)
        self.assertIn("int32 CombatTextPalette = 0;", self.hero_h)

    def test_attacker_owns_bounded_transient_result_history(self):
        self.assertIn("static constexpr int32 CombatOutcomeHistorySize = 4;", self.enemy_h)
        self.assertIn("FCombatOutcomeFloat CombatOutcomeHistory[CombatOutcomeHistorySize];", self.enemy_h)
        self.assertIn("void RecordCombatOutcomeFloat(FName Outcome, float HealthLost, float WorldTime);", self.enemy_h)
        writer = self.enemy.split(
            "void AMoonfallEnemy::RecordCombatOutcomeFloat(", 1
        )[1].split("void AMoonfallEnemy::EndNonBossTelegraph(", 1)[0]
        for token in ("!FMath::IsFinite(WorldTime)", "!FMath::IsFinite(HealthLost)",
                      "HealthLost < 0.f", "IsActorBeingDestroyed()",
                      "Entry.StartedAt = WorldTime;",
                      "(NextCombatOutcomeSlot + 1) % CombatOutcomeHistorySize"):
            self.assertIn(token, writer)
        for status in ("HIT", "PARRY", "DODGE", "GUARD", "MISS"):
            self.assertIn('FName(TEXT("' + status + '"))', writer)
        for mutating in ("Hero->", "SpawnActor", "TActorIterator",
                         "SetActorLocation", "SaveWorldProgress", "NewObject", "Add("):
            self.assertNotIn(mutating, writer)

    def test_actual_resolved_defense_is_recorded_on_attacker_before_blueprint(self):
        block = self.enemy.split("if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {", 1
            )[1].split("if (bComboSecondHitPending)", 1)[0]
        self.assertIn("if (!bBoss && !ImpactMoveId.IsNone()", block)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", block)
        self.assertIn("MoonfallAttackOutcome::Classify(", block)
        self.assertIn("if (Hero->bShowFloatingCombatText)", block)
        self.assertIn("RecordCombatOutcomeFloat(OutcomeName, HealthLost, Now);", block)
        self.assertIn("OnAttackImpactOutcome(ImpactMoveId, OutcomeName, HealthLost);", block)
        self.assertLess(block.index("MoonfallAttackOutcome::Classify("),
                        block.index("RecordCombatOutcomeFloat("))
        self.assertLess(block.index("RecordCombatOutcomeFloat("),
                        block.index("OnAttackImpactOutcome("))

    def test_world_projection_uses_enemy_only_never_hero_position(self):
        hud = self.hud
        self.assertEqual(hud.count("TActorIterator<AMoonfallEnemy>"), 1)
        projection = hud.split(
            "// All floating combat text is anchored exclusively to the attacker.", 1
        )[1]
        self.assertIn("for (const AMoonfallEnemy* Enemy : NearbyDamageEnemies)", projection)
        self.assertIn("const FVector EnemyDamageLocation = Enemy->GetActorLocation();", projection)
        self.assertIn("EnemyDamageLocation + FVector(0.f,0.f,245.f+Rise)", projection)
        self.assertIn("ProjectWorldLocationToScreen(WorldTextPosition,Screen)", projection)
        self.assertIn("Enemy->CombatOutcomeHistory[Slot]", projection)
        self.assertNotIn("Hero->GetActorLocation()", projection)
        self.assertNotIn("BasePosition", projection)
        self.assertNotIn("IncomingCombatFloats", projection)
        self.assertEqual(projection.count("Enemy->GetActorLocation()"), 1)

    def test_same_bounded_scan_collects_outgoing_and_incoming_per_enemy(self):
        source = self.hud
        scan = source.split(
            "for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {", 1
        )[1].split("if (Boss && BossDistanceSq", 1)[0]
        for token in ("NearbyDamageEnemies.Num() < MoonfallCombatFloatLayout::MaxNearbyCombatActors",
                      "bHasRecentDamage", "bHasRecentOutcome",
                      "E->DamageHistory[Slot]", "E->CombatOutcomeHistory[Slot]",
                      "if (bHasRecentDamage || bHasRecentOutcome) NearbyDamageEnemies.Add(E);"):
            self.assertIn(token, scan)
        self.assertLess(scan.index("NearbyDamageEnemies.Add(E);"),
                        scan.index("if (E->Health <= 0.f) continue;"))
        self.assertIn("AMoonfallEnemy::DamageHistorySize", scan)
        self.assertIn("AMoonfallEnemy::CombatOutcomeHistorySize", scan)
        self.assertEqual(source.count("TActorIterator<AMoonfallEnemy>"), 1)
        self.assertIn("MaxVisibleAttackOutcomes", source)
        self.assertIn("MaxVisibleDealtDamage", source)

    def test_five_results_label_enemy_source_not_player(self):
        block = self.hud.split(
            "// Enemy-only source labels, in fixed two-column lanes above the attacker.", 1
        )[1].split("for (int32 Slot = 0; Slot < AMoonfallEnemy::DamageHistorySize", 1)[0]
        for label in ('TEXT("HIT YOU -%.0f")', 'TEXT("PARRY!")',
                      'TEXT("DODGE!")', 'TEXT("BLOCKED!")', 'TEXT("MISS!")'):
            self.assertIn(label, block)
        self.assertIn("Hero->CombatTextPalette == 2", block)
        self.assertIn("Hero->CombatTextPalette == 1", block)
        self.assertIn("MoonfallCombatFloatLayout::Opacity(Age)", block)
        self.assertIn("MoonfallCombatFloatLayout::Rise(Age, Hero->bReduceMotion)", block)
        self.assertIn("MoonfallCombatFloatLayout::LaneX(LocalDisplayed)", block)
        self.assertIn("MoonfallCombatFloatLayout::LaneY(LocalDisplayed)", block)
        self.assertIn("FMath::Clamp(Hero->CombatTextScale,0.75f,2.f)", block)
        self.assertIn("MoonfallCombatFloatLayout::TryPlace(", block)
        self.assertIn("bHeroScreenValid,HeroScreenExclusion,PlacedOutcomes,", block)
        self.assertIn("VisibleOutcomeLabels,Placement", block)
        self.assertIn("PlacedOutcomes[VisibleOutcomeLabels++] = Placement;", block)
        self.assertNotIn("Hero->GetActorLocation()", block)
        self.assertNotIn("SpawnActor", block)
        self.assertNotIn("TActorIterator", block)
        self.assertNotIn("LineTrace", block)

    def test_hero_silhouette_has_strict_exclusion_even_when_enemy_is_close(self):
        projection = self.hud.split(
            "// All floating combat text is anchored exclusively to the attacker.",1
        )[1]
        self.assertIn("FVector2D HeroOnScreen = FVector2D::ZeroVector;", projection)
        self.assertIn("HeroLocation + FVector(0.f,0.f,100.f),HeroOnScreen", projection)
        self.assertIn("HeroScreenExclusion{", projection)
        self.assertIn("HeroOnScreen.X-84.f,HeroOnScreen.Y-110.f,168.f,216.f", projection)
        self.assertIn("bHeroScreenValid,HeroScreenExclusion", projection)
        self.assertIn("MoonfallCombatFloatLayout::TryPlace(", projection)
        self.assertNotIn("Hero->GetActorLocation()", projection)
        # Player screen position is a protected obstacle, never an anchor.
        self.assertIn("EnemyDamageLocation + FVector(0.f,0.f,245.f+Rise)", projection)

    def test_recent_outcomes_take_priority_over_expired_old_events(self):
        block = self.hud.split(
            "// Most recent resolution first.",1
        )[1].split("for (int32 Slot = 0; Slot < AMoonfallEnemy::DamageHistorySize",1)[0]
        self.assertIn("Enemy->NextCombatOutcomeSlot", block)
        self.assertIn("+ AMoonfallEnemy::CombatOutcomeHistorySize-1-AgeIndex", block)
        self.assertIn("% AMoonfallEnemy::CombatOutcomeHistorySize", block)
        self.assertIn("int32 LocalDisplayed = 0;", block)
        self.assertIn("++LocalDisplayed;", block)
        self.assertIn("PlacedOutcomes[VisibleOutcomeLabels++] = Placement;",block)
        self.assertIn("MoonfallCombatFloatLayout::MaxVisibleAttackOutcomes",block)
        for dangerous in ("FMath::Rand", "TActorIterator", "SpawnActor",
                          "GetWorldTimerManager", "NewObject"):
            self.assertNotIn(dangerous,block)

    def test_outgoing_damage_labels_stay_above_enemies(self):
        hud = self.hud
        self.assertIn("Enemy->DamageHistory[Slot]", hud)
        self.assertIn("const FVector WorldTextPosition = EnemyDamageLocation +", hud)
        self.assertIn("VisibleDamageNumbers < MoonfallCombatFloatLayout::MaxVisibleDealtDamage", hud)
        self.assertIn("SHATTER -%.0f", hud)
        self.assertIn('TEXT("-%.0f")', hud)
        self.assertIn("if (Hero->bShowFloatingCombatText)", hud)
        self.assertIn("Hero->CombatTextScale", hud)

    def test_accessibility_settings_remain_persistent_and_mouse_driven(self):
        options = self.hud.split("if (Hero->bOptionsOpen && !Hero->bHUDLayoutEditing) {",1)[1].split(
            "if (Hero->bInventoryOpen)",1)[0]
        mouse = self.hero.split("// Match all eleven options rows",1)[1].split(
            "if (bInventoryOpen)",1)[0]
        for text in ('TEXT("Floating results")', 'TEXT("Text palette")',
                     'TEXT("Contrast")', 'TEXT("Mono")'):
            self.assertIn(text, options)
        self.assertIn("(MenuHeight - 130.f) / 11.f", options)
        self.assertIn("(MenuHeight - 130.f) / 11.f", mouse)
        self.assertIn("ClickedRow >= 0 && ClickedRow < 11", mouse)
        self.assertIn("for (int32 i = 0; i < 11; ++i)", options)
        self.assertIn("OptionsSelection = (OptionsSelection + 10) % 11", self.hero)
        self.assertIn("OptionsSelection = (OptionsSelection + 1) % 11", self.hero)
        self.assertIn('TEXT("ShowFloatingCombatText")', self.hero)
        self.assertIn('TEXT("CombatTextPalette")', self.hero)
        self.assertIn("CombatTextScale + Direction * 0.25f", self.hero)
        self.assertIn("GConfig->Flush(false, GGameUserSettingsIni);", self.hero)


if __name__ == "__main__":
    unittest.main()
