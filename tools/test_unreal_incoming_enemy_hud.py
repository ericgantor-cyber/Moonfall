"""Source contracts for a single truthful ordinary/elite enemy HUD warning."""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class IncomingEnemyHUDTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.header = (SRC / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_warning_is_authoritative_and_read_only(self):
        self.assertIn("float GetAttackWarningRemaining(float WorldTime) const;", self.header)
        method = self.enemy.split("float AMoonfallEnemy::GetAttackWarningRemaining(", 1)[1].split("float AMoonfallEnemy::GetAttackWarningDuration(", 1)[0]
        for value in ("bBoss", "Health <= 0.f", "!FMath::IsFinite(WorldTime)",
                      "!FMath::IsFinite(WindupEndsAt)", "WindupEndsAt <= WorldTime",
                      "Remaining <= FMath::Clamp(CurrentWindup,0.1f,5.f)"):
            self.assertIn(value, method)
        for mutating in ("WindupEndsAt =", "NextAttackAt =", "SpawnActor", "SetActorLocation", "RandRange"):
            self.assertNotIn(mutating, method)

    def test_existing_world_scan_selects_one_warning(self):
        scan = self.hud.split("// Reuse a single world scan for boss/posture HUD",1)[1].split("if (Boss && BossDistanceSq <",1)[0]
        for value in ("const AMoonfallEnemy* IncomingThreat = nullptr;", "if (E->Health <= 0.f) continue;",
                      "if (!E->bBoss && DistSq <= FMath::Square(1050.f))",
                      "E->GetAttackWarningRemaining(WorldTime)",
                      "Seconds < IncomingThreatSeconds", "DistSq < IncomingThreatDistanceSq"):
            self.assertIn(value, scan)

    def test_boss_warnings_take_priority_without_flash(self):
        render = self.hud.split("// A single non-flashing alert for the next ordinary/elite strike.",1)[1].split("// Show only the nearest engaged non-boss posture target",1)[0]
        for value in ("(!Boss || BossDistanceSq >= FMath::Square(3000.f))",
                      "IncomingThreatSeconds <= 0.3f", "GUARD / PARRY / DODGE",
                      "GetAttackWarningLabel()", "GetAttackWarningDuration()"):
            self.assertIn(value, render)
        self.assertNotIn("TActorIterator", render)
        self.assertNotIn("Sin(", render)
        self.assertNotIn("SpawnActor", render)

    def test_guardian_and_existing_elite_combat_preserved(self):
        for text in ("MoonfallGuardianFootprint::Hit", "MoonfallEliteTactics::SelectPattern", "RefreshAttackTellMarker"):
            self.assertIn(text, self.enemy)

if __name__ == "__main__":
    unittest.main()
