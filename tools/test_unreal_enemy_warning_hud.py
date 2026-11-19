"""Source-level safety contracts for Moonfall's non-boss attack alerts.

No Unreal native compilation or gameplay testing is performed by this suite.
"""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class EnemyWarnings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy_h = (SRC / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_getter_is_read_only_and_fails_closed(self):
        h = self.enemy_h
        self.assertIn("float GetAttackWarningRemaining(float WorldTime) const;", h)
        self.assertIn("float GetAttackWarningDuration() const;", h)
        self.assertIn("FString GetAttackWarningLabel() const;", h)
        getter = self.enemy.split(
            "float AMoonfallEnemy::GetAttackWarningRemaining(", 1
        )[1].split("float AMoonfallEnemy::GetAttackWarningDuration()", 1)[0]
        for guard in ("bBoss", "Health <= 0.f",
                      "!FMath::IsFinite(WorldTime)",
                      "!FMath::IsFinite(WindupEndsAt)",
                      "!FMath::IsFinite(CurrentWindup)",
                      "WindupEndsAt <= WorldTime",
                      "Remaining <= FMath::Clamp(CurrentWindup,0.1f,5.f)"):
            self.assertIn(guard, getter)
        for mutating in ("WindupEndsAt =", "NextAttackAt =", "SpawnActor",
                         "SetActorLocation", "RandRange", "LineTrace",
                         "GetWorldTimerManager"):
            self.assertNotIn(mutating, getter)

    def test_warning_only_nearby_live_nonboss_during_real_windup(self):
        scanner = self.hud.split(
            "// Reuse a single world scan for boss/posture HUD", 1
        )[1].split("if (Boss && BossDistanceSq <", 1)[0]
        self.assertIn("if (E->Health <= 0.f) continue;", scanner)
        self.assertIn("!E->bBoss && DistSq <= FMath::Square(1050.f)", scanner)
        self.assertIn("E->GetAttackWarningRemaining(WorldTime)", scanner)
        self.assertIn("if (Seconds > 0.f", scanner)
        self.assertIn("Seconds < IncomingThreatSeconds", scanner)
        self.assertIn("DistSq < IncomingThreatDistanceSq", scanner)
        self.assertIn("IncomingThreat = E;", scanner)
        self.assertEqual(self.hud.count("TActorIterator<AMoonfallEnemy>"), 1)

    def test_richer_warning_is_readable_without_color_or_flashing(self):
        block = self.hud.split(
            "// A single text-first warning for the nearest imminent ordinary/elite", 1
        )[1].split("// Show only the nearest engaged non-boss posture target", 1)[0]
        self.assertIn("IsValid(IncomingThreat)", block)
        self.assertIn("Canvas->SizeX >= 480.f && Canvas->SizeY >= 400.f", block)
        self.assertIn("IncomingThreat->GetAttackWarningDuration()", block)
        self.assertIn("IncomingThreat->GetAttackWarningLabel()", block)
        self.assertIn('TO IMPACT', block)
        self.assertIn('GUARD / PARRY / DODGE', block)
        self.assertIn('IMPACT SOON', block)
        self.assertIn('DrawText(', block)
        self.assertNotIn("FMath::Sin(", block)
        self.assertNotIn("SpawnActor", block)
        self.assertNotIn("GetWorldTimerManager", block)
        self.assertNotIn("DrawDebug", block)
        self.assertNotIn("TActorIterator", block)
        self.assertIn("!Boss || BossDistanceSq >= FMath::Square(3000.f)", block)

    def test_labels_follow_existing_hit_damage_not_fake_dodge_only(self):
        body = self.enemy.split(
            "FString AMoonfallEnemy::GetAttackWarningLabel() const", 1
        )[1].split("void AMoonfallEnemy::BeginPlay()", 1)[0]
        for move in ('ELITE CRUSH', 'HEAVY', 'SWEEP', 'STRIKE'):
            self.assertIn(move, body)
        self.assertNotIn('DODGE ONLY', body)
        self.assertIn('const bool bDodgeOnlyStrike = bBoss', self.enemy)
        self.assertIn('TEXT("RED - DODGE ONLY")', self.hud)

    def test_modal_and_stagger_cancel_authoritative_windup(self):
        modal = self.enemy.split(
            "if (!IsValid(Hero) || !FMath::IsFinite(Hero->Health)",1
        )[1].split("// A corrupt nonfinite windup", 1)[0]
        for state in ("Hero->bOptionsOpen", "Hero->bInventoryOpen",
                      "Hero->bAtlasOpen", "Hero->bResidentContextOpen"):
            self.assertIn(state, modal)
        self.assertIn("WindupEndsAt = -1.f;", modal)
        self.assertIn("WindupEndsAt = -1.f;", self.enemy)
        self.assertIn("NextAttackAt = FMath::Max(", self.enemy)

    def test_boss_hud_remains_authoritative(self):
        self.assertIn("Boss->BossWarningEndTime - GetWorld()->GetTimeSeconds()", self.hud)
        self.assertIn("Boss->bDungeonBossAttackDodgeOnly", self.hud)
        self.assertIn('TEXT("BLUE - PARRY / DODGE")', self.hud)
        self.assertIn("Boss->DungeonBossAttackWarningDuration", self.hud)
        self.assertIn("bBoss || !FMath::IsFinite(Health)", self.enemy)


if __name__ == "__main__":
    unittest.main()
