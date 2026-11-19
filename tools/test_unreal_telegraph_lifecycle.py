"""Regression checks for nonboss attack presentation life-cycle.

This does not compile Unreal 5.4 or run an authored animation montage.
The events are optional and preserve all existing hit/damage rules.
"""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class AttackTelegraphLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.header = (SRC / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_three_optional_events_use_same_move_identity(self):
        for signature in (
            "void OnAttackTelegraphCommitted(FName MoveId, float WarningSeconds);",
            "void OnAttackTelegraphResolved(FName MoveId);",
            "void OnAttackTelegraphInterrupted(FName MoveId);",
        ):
            self.assertIn(signature, self.header)
        self.assertIn("FName ActiveNonBossTelegraphMove = NAME_None;", self.header)
        self.assertIn("void EndNonBossTelegraph(bool bResolved);", self.header)
        commit = self.enemy.split(
            "// Blueprint hooks run synchronously.",1
        )[1].split("if (bDivineSignatureActive) {",1)[0]
        self.assertIn("if (!bBoss) {", commit)
        self.assertIn(
            "ActiveNonBossTelegraphMove = FName(*GetAttackWarningLabel());",
            commit)
        self.assertIn(
            "OnAttackTelegraphCommitted(ActiveNonBossTelegraphMove, CurrentWindup);",
            commit)
        self.assertLess(
            commit.index("ActiveNonBossTelegraphMove = FName(*GetAttackWarningLabel());"),
            commit.index("OnAttackTelegraphCommitted("))
        self.assertNotIn("SpawnActor", commit)

    def test_end_clears_identity_before_invoking_blueprint(self):
        helper = self.enemy.split(
            "void AMoonfallEnemy::EndNonBossTelegraph(bool bResolved) {",1
        )[1].split("float AMoonfallEnemy::GetAttackWarningRemaining(",1)[0]
        self.assertIn("const FName FinishedMove = ActiveNonBossTelegraphMove;", helper)
        self.assertIn("if (FinishedMove.IsNone()) return;",helper)
        self.assertIn("ActiveNonBossTelegraphMove = NAME_None;",helper)
        self.assertIn("if (bResolved) OnAttackTelegraphResolved(FinishedMove);",helper)
        self.assertIn("else OnAttackTelegraphInterrupted(FinishedMove);",helper)
        self.assertLess(helper.index("ActiveNonBossTelegraphMove = NAME_None;"),
                        helper.index("OnAttackTelegraphResolved(FinishedMove);"))
        self.assertLess(helper.index("ActiveNonBossTelegraphMove = NAME_None;"),
                        helper.index("OnAttackTelegraphInterrupted(FinishedMove);"))
        self.assertNotIn("RandRange",helper)
        self.assertNotIn("GetWorldTimerManager",helper)

    def test_resolution_only_at_authoritative_impact_deadline(self):
        attack = self.enemy.split(
            "if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {",1
        )[1].split("// Heavy elite crushes",1)[0]
        self.assertIn("WindupEndsAt = -1.f;",attack)
        self.assertIn("bAdaptiveSweepCommitted = false;",attack)
        self.assertIn("EndNonBossTelegraph(true);",attack)
        self.assertLess(attack.index("WindupEndsAt = -1.f;"),
                        attack.index("EndNonBossTelegraph(true);"))
        self.assertIn(
            "if (IsActorBeingDestroyed() || !FMath::IsFinite(Health) || Health <= 0.f) return;",
            attack)
        self.assertEqual(
            self.enemy.count("EndNonBossTelegraph(true);"),1,
            "No additional event fired for hit/miss or repeated ticking")
        self.assertIn("const bool bWithinBasicAttack",self.enemy)
        self.assertIn("const bool bWithinDivineMarker",self.enemy)

    def test_cancel_paths_do_not_misreport_attacks_as_resolved(self):
        sections = {
            "posture": self.enemy.split(
                "PostureBrokenUntil = Now + VulnerabilitySeconds;",1
            )[1].split("OnPostureBroken(VulnerabilitySeconds);",1)[0],
            "retreat": self.enemy.split("TacticalRetreatUntil = Now + 2.f;",1
            )[1].split("CachedWaypoint = FVector::ZeroVector;",1)[0],
            "defeat": self.enemy.split("if (Health <= 0.0f) {",1
            )[1].split("SetActorEnableCollision(false);",1)[0],
            "menus": self.enemy.split(
                "if (!IsValid(Hero) || !FMath::IsFinite(Hero->Health)",1
            )[1].split("// A corrupt nonfinite windup",1)[0],
            "nonfinite": self.enemy.split(
                "if (!FMath::IsFinite(WindupEndsAt)) {",1
            )[1].split("// A shatter follow-up expires",1)[0],
            "stagger": self.enemy.split("if (Staggered) {",1
            )[1].split("// Reject invalid actor transforms",1)[0],
            "practice reset": self.enemy.split(
                "if (bSanctuaryPractice && Distance > 450.f) {",1
            )[1].split("// Far-away enemies use a slower think rate",1)[0],
            "disengage": self.enemy.split("if (!bInCombatArea) {",1
            )[1].split("// Complete the full two-second",1)[0],
        }
        for name, section in sections.items():
            with self.subTest(path=name):
                self.assertIn("WindupEndsAt = -1.f;",section)
                self.assertIn("EndNonBossTelegraph(false);",section)
                self.assertLess(section.index("WindupEndsAt = -1.f;"),
                                section.index("EndNonBossTelegraph(false);"))
                self.assertNotIn("EndNonBossTelegraph(true);",section)

    def test_committed_event_can_be_canceled_before_audio(self):
        block = self.enemy.split(
            "OnAttackTelegraphCommitted(ActiveNonBossTelegraphMove, CurrentWindup);",1
        )[1].split("if (bDivineSignatureActive) {",1)[0]
        self.assertIn("IsActorBeingDestroyed()",block)
        self.assertIn("FMath::IsFinite(Health) || Health <= 0.f",block)
        self.assertIn("!FMath::IsFinite(WindupEndsAt) || WindupEndsAt <= Now",block)
        self.assertIn("EndNonBossTelegraph(false);",block)
        self.assertIn("UGameplayStatics::PlaySoundAtLocation(",block)
        self.assertLess(block.index("EndNonBossTelegraph(false);"),
                        block.index("UGameplayStatics::PlaySoundAtLocation("))
        self.assertIn("Now >= NextEliteAttackTellSoundAt",block)

    def test_no_new_scan_or_gameplay_mutation(self):
        self.assertEqual(self.hud.count("TActorIterator<AMoonfallEnemy>"),1)
        helper=self.enemy.split(
            "void AMoonfallEnemy::EndNonBossTelegraph(bool bResolved) {",1
        )[1].split("float AMoonfallEnemy::GetAttackWarningRemaining(",1)[0]
        for forbidden in (
            "Health =", "AttackDamage =", "Stamina =", "NextAttackAt =",
            "SetActorLocation", "SpawnActor", "TActorIterator",
        ):
            self.assertNotIn(forbidden, helper)
        self.assertIn("void OnDivineAttackTelegraphed",self.header)
        self.assertIn("void OnDungeonBossAttackTelegraphed",self.header)
        self.assertIn("const bool bDodgeOnlyStrike = bBoss",self.enemy)


if __name__ == "__main__":
    unittest.main()
