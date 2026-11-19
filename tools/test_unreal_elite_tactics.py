"""Static integration contracts for earned, bounded elite counter-play.

Unreal Engine 5.4 native compilation and gameplay remain separate gates.
"""
import unittest
from pathlib import Path

SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"

class EliteCounterTactics(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy=(SRC/"MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.header=(SRC/"MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.rules=(SRC/"MoonfallEliteTactics.h").read_text(encoding="utf-8")
        cls.hud=(SRC/"MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_only_successful_parry_arms_finite_memory(self):
        body=self.enemy.split("const bool bParried = bDodgeOnlyStrike",1)[1].split(
            "} else if (!bPerfectDodge)",1)[0]
        self.assertIn("if (bParried) {",body)
        self.assertIn("ApplyPostureDamage(FMath::Max(0.f, ParryPostureDamage));",body)
        self.assertIn("MoonfallEliteTactics::CanArm(",body)
        self.assertIn("bElite, bBoss, bSanctuaryPractice, Health > 0.f,",body)
        self.assertIn("Now < PostureBrokenUntil",body)
        self.assertIn("Now >= NextAdaptiveSweepAllowedAt",body)
        self.assertIn("if (bCanArmAdaptation)",body)
        self.assertIn("AdaptiveSweepReadyUntil = Now + MoonfallEliteTactics::ParryMemorySeconds;",body)
        self.assertIn("NextAdaptiveSweepAllowedAt =",body)
        self.assertIn("OnPerfectParried(ParryStaggerSeconds)",body)
        self.assertNotIn("FMath::RandRange",body)
        self.assertNotIn("SpawnActor",body)

    def test_counter_only_on_next_valid_attack_not_during_scan_or_cooldown(self):
        attacks=self.enemy.split(
            "// Alternate quick strikes, heavy attacks and wider sweeps",1
        )[1].split("BossSpecialMove = bBoss && BossPhase",1)[0]
        self.assertIn("int32 Pattern = AttackSequence++ % PatternCount;",attacks)
        self.assertIn("AdaptiveSweepReadyUntil > Now;",attacks)
        self.assertIn("AdaptiveSweepReadyUntil = -1.f;",attacks)
        self.assertIn("MoonfallEliteTactics::SelectPattern(",attacks)
        self.assertIn("bAdaptiveSweepCommitted = bAdaptationReady;",attacks)
        self.assertLess(attacks.index("AdaptiveSweepReadyUntil = -1.f;"),
                        attacks.index("MoonfallEliteTactics::SelectPattern("))
        self.assertIn("if (bAdaptiveSweepCommitted)",self.enemy)
        self.assertIn("MoonfallEliteTactics::CounterSweepWindup",self.enemy)

    def test_nonboss_move_name_is_truthful_and_defense_unchanged(self):
        labels=self.enemy.split(
            "FString AMoonfallEnemy::GetAttackWarningLabel() const {",1
        )[1].split("void AMoonfallEnemy::BeginPlay()",1)[0]
        self.assertIn('TEXT("COUNTER SWEEP")',labels)
        self.assertIn("bAdaptiveSweepCommitted && bElite && !bBoss",labels)
        for name in ('TEXT("ELITE CRUSH")','TEXT("HEAVY")','TEXT("SWEEP")','TEXT("STRIKE")'):
            self.assertIn(name,labels)
        self.assertIn('IncomingThreat->GetAttackWarningLabel()',self.hud)
        self.assertIn('TEXT("GUARD / PARRY / DODGE")',self.hud)
        self.assertIn('const bool bDodgeOnlyStrike = bBoss',self.enemy)
        # The new label never routes through special dodge-only handling.
        self.assertNotIn("CounterSweep",self.enemy.split(
            "const bool bDodgeOnlyStrike = bBoss",1)[1].split(
            "const bool bParried",1)[0])

    def test_audio_animation_hook_is_opt_in_after_real_windup(self):
        self.assertIn("void OnAttackTelegraphCommitted(FName MoveId, float WarningSeconds);",self.header)
        self.assertIn("TObjectPtr<USoundBase> EliteAttackTellSound = nullptr;",self.header)
        commit=self.enemy.split(
            "// Blueprint hooks run synchronously.",1
        )[1].split("\n  }\n }\n}",1)[0]
        self.assertIn("if (!bBoss) {",commit)
        self.assertIn("OnAttackTelegraphCommitted(ActiveNonBossTelegraphMove, CurrentWindup);",commit)
        self.assertIn("IsValid(EliteAttackTellSound)",commit)
        self.assertIn("bElite && !bSanctuaryPractice",commit)
        self.assertIn("Distance <= 1000.f",commit)
        self.assertIn("UGameplayStatics::PlaySoundAtLocation(",commit)
        self.assertIn("EliteAttackTellSound.Get()",commit)
        self.assertIn("ActiveNonBossTelegraphMove = FName(*GetAttackWarningLabel());",self.enemy)
        self.assertIn("EndNonBossTelegraph(false);",self.enemy)
        self.assertIn("WindupEndsAt = Now + CurrentWindup;",self.enemy)
        self.assertLess(
            self.enemy.index("WindupEndsAt = Now + CurrentWindup;",self.enemy.index(
                "const int32 PatternCount =")),
            self.enemy.index("OnAttackTelegraphCommitted(FName(*GetAttackWarningLabel())"))
        self.assertNotIn("GetWorldTimerManager()",commit)
        self.assertNotIn("SpawnActor",commit)

    def test_counter_is_cleared_on_six_cancellation_paths(self):
        checks = {
            "posture shatter": self.enemy.split(
                "PostureBrokenUntil = Now + VulnerabilitySeconds;",1
            )[1].split("OnPostureBroken(VulnerabilitySeconds);",1)[0],
            "wounded elite retreat": self.enemy.split(
                "TacticalRetreatUntil = Now + 2.f;",1
            )[1].split("CachedWaypoint = FVector::ZeroVector;",1)[0],
            "defeat": self.enemy.split(
                "if (Health <= 0.0f) {",1
            )[1].split("SetActorEnableCollision(false);",1)[0],
            "menu and possession": self.enemy.split(
                "if (!IsValid(Hero) || !FMath::IsFinite(Hero->Health)",1
            )[1].split("// A corrupt nonfinite windup",1)[0],
            "corrupt timer": self.enemy.split(
                "if (!FMath::IsFinite(WindupEndsAt)) {",1
            )[1].split("// A shatter follow-up expires",1)[0],
            "leaving combat": self.enemy.split(
                "if (!bInCombatArea) {",1
            )[1].split("// Complete the full two-second",1)[0],
        }
        for label, section in checks.items():
            with self.subTest(reason=label):
                self.assertIn("bAdaptiveSweepCommitted = false;", section)
                self.assertIn("AdaptiveSweepReadyUntil = -1.f;", section)
                self.assertIn("WindupEndsAt = -1.f;", section)
        resolution = self.enemy.split(
            "if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {",1
        )[1].split("// Heavy elite crushes",1)[0]
        self.assertIn("bAdaptiveSweepCommitted = false;", resolution)

    def test_parry_recovery_does_not_erase_new_counter_memory(self):
        # A successful parry causes a brief hit reaction. Clearing the
        # *committed* label must not immediately discard the response that
        # this same parry just armed, unless posture actually shattered.
        stagger = self.enemy.split("if (Staggered) {",1)[1].split(
            "// Reject invalid actor transforms",1)[0]
        self.assertIn("bAdaptiveSweepCommitted = false;",stagger)
        self.assertNotIn("AdaptiveSweepReadyUntil = -1.f;",stagger)
        posture_break = self.enemy.split(
            "PostureBrokenUntil = Now + VulnerabilitySeconds;",1
        )[1].split("OnPostureBroken(VulnerabilitySeconds);",1)[0]
        self.assertIn("AdaptiveSweepReadyUntil = -1.f;",posture_break)

    def test_audio_is_suppressed_after_blueprint_cancels_a_windup(self):
        commit = self.enemy.split(
            "OnAttackTelegraphCommitted(ActiveNonBossTelegraphMove, CurrentWindup);",1
        )[1].split("if (bDivineSignatureActive) {",1)[0]
        self.assertIn("!IsActorBeingDestroyed()",commit)
        self.assertIn("FMath::IsFinite(Health) && Health > 0.f",commit)
        self.assertIn("FMath::IsFinite(WindupEndsAt) && WindupEndsAt > Now",commit)
        self.assertIn("FMath::IsFinite(CurrentWindup) && CurrentWindup > 0.f",commit)
        self.assertIn("UGameplayStatics::PlaySoundAtLocation(",commit)
        self.assertLess(commit.index("WindupEndsAt > Now"),
                        commit.index("UGameplayStatics::PlaySoundAtLocation("))
        self.assertNotIn("GetWorldTimerManager",commit)
        self.assertNotIn("SpawnActor",commit)

    def test_audio_has_independent_per_enemy_two_second_cooldown(self):
        self.assertIn("float NextEliteAttackTellSoundAt = 0.f;",self.header)
        tell = self.enemy.split(
            "OnAttackTelegraphCommitted(ActiveNonBossTelegraphMove, CurrentWindup);",1
        )[1].split("if (bDivineSignatureActive) {",1)[0]
        self.assertIn("Now >= NextEliteAttackTellSoundAt",tell)
        self.assertIn("NextEliteAttackTellSoundAt = Now + 2.f;",tell)
        self.assertLess(tell.index("NextEliteAttackTellSoundAt = Now + 2.f;"),
                        tell.index("UGameplayStatics::PlaySoundAtLocation("))
        self.assertNotIn("NextAttackAt =",tell)
        self.assertNotIn("AdaptiveSweepReadyUntil =",tell)

    def test_no_new_ai_scan_or_saved_property(self):
        self.assertIn("float AdaptiveSweepReadyUntil = -1.f;",self.header)
        self.assertIn("float NextAdaptiveSweepAllowedAt = 0.f;",self.header)
        self.assertIn("bool bAdaptiveSweepCommitted = false;",self.header)
        self.assertIn("constexpr float ParryMemorySeconds = 4.f;",self.rules)
        self.assertIn("constexpr float ParryResponseCooldownSeconds = 8.f;",self.rules)
        self.assertIn("constexpr float CounterSweepWindup = 0.82f;",self.rules)
        self.assertNotIn("TActorIterator",self.rules)
        self.assertNotIn("Random",self.rules)
        self.assertNotIn("UPROPERTY",self.rules)
        self.assertNotIn("SaveWorldProgress",self.rules)
        self.assertNotIn("TActorIterator<AMoonfallEnemy>",self.hud.split(
            "// A single text-first warning for the nearest",1
        )[1].split("// Show only the nearest engaged non-boss",1)[0])


if __name__=="__main__":
    unittest.main()
