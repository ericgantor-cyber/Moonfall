"""UE5 melee and enemy-flinch regression contracts (source checks, not a native UE5 playtest)."""
from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class CombatIntegrityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        cls.header = (SRC / "MoonfallHeroPawn.h").read_text()
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text()

    def test_injury_interrupts_buffer_only_for_actual_health_loss(self):
        damage = self.hero.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        before, injury = damage.split("const float DamageTaken = PreviousHealth - Health;", 1)
        self.assertNotIn("SwordBuffer.Clear();", before)  # Parries and blocks retain follow-up.
        self.assertIn("DamageAfterGuard = 0.f;", before)
        injury = injury.split("if (Health <= 0.f) {", 1)[0]
        self.assertIn("if (DamageTaken > 0.f) {", injury)
        self.assertIn("SwordBuffer.Clear();", injury)
        self.assertLess(injury.index("if (DamageTaken > 0.f) {"), injury.index("SwordBuffer.Clear();"))
        self.assertLess(injury.index("SwordBuffer.Clear();"), injury.index("HitComboStage = 0;"))

    def test_right_hand_weapon_swap_clears_pending_attacks(self):
        weapon = self.hero.split("void AMoonfallHeroPawn::ReadyRightHandWeapon()", 1)[1].split(
            "void AMoonfallHeroPawn::ToggleSpellBookShortcut()", 1)[0]
        self.assertLess(weapon.index("SwordBuffer.Clear();"),
                        weapon.index("bRightHandSpellReadied = false;"))
        self.assertIn("OnRightHandLoadoutChanged(false, RightSpell);", weapon)

    def test_misses_and_absorbed_hits_really_reset_combo(self):
        self.assertIn("void ResetMeleeHitCombo();", self.header)
        reset = self.hero.split("void AMoonfallHeroPawn::ResetMeleeHitCombo()", 1)[1].split(
            "void AMoonfallHeroPawn::MeleeAttack()", 1)[0]
        for statement in ("HitComboStage = 0;", "LastHitComboTarget.Reset();",
                          "LastSuccessfulHitAt = -100.f;"):
            self.assertIn(statement, reset)
        melee = self.hero.split("void AMoonfallHeroPawn::MeleeAttack()", 1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()", 1)[0]
        miss = melee.split("if (!bLivingTarget) {", 1)[1].split("\n }", 1)[0]
        invalid = melee.split("if (!FMath::IsFinite(Damage) || Damage <= 0.f) {", 1)[1].split("\n }", 1)[0]
        for branch in (miss, invalid):
            self.assertIn("ResetMeleeHitCombo();", branch)
            self.assertIn("OnMeleeSwing(CurrentSwordAngle, false);", branch)
        self.assertIn("if (bConnected) {", melee)
        absorbed = melee.split("} else {\n  // Fully absorbed strikes cannot retain a powered combo.", 1)[1]
        self.assertIn("ResetMeleeHitCombo();", absorbed)
        self.assertLess(melee.index("Stamina = FMath::Clamp(Stamina - Cost"),
                        melee.index("if (!bLivingTarget) {"))
        self.assertIn("OnSuccessfulHitCombo(HitComboStage, false);", melee)

    def test_cancelled_enemy_attacks_hide_warning_before_next_tick(self):
        enemy = self.enemy
        posture = enemy.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::RegisterComboMilestone", 1)[0]
        retreat = enemy.split("TacticalRetreatUntil = Now + 2.f;", 1)[1].split(
            "if (bShatterFinisher && ActualDamage > 0.f)", 1)[0]
        death = enemy.split("if (Health <= 0.0f) {", 1)[1].split(
            "SetActorTickEnabled(false);", 1)[0]
        for state, block in (("posture", posture), ("retreat", retreat), ("fatal", death)):
            with self.subTest(state=state):
                self.assertIn("WindupEndsAt = -1.f;", block)
                self.assertIn("AttackTellMarker->SetVisibility(false);", block)
                self.assertIn("IsValid(AttackTellMarker) && AttackTellMarker->IsVisible()", block)
                self.assertLess(block.index("WindupEndsAt = -1.f;"),
                                block.index("AttackTellMarker->SetVisibility(false);"))
        # Death turns off actor Tick, so marker cleanup cannot be deferred.
        self.assertIn("SetActorTickEnabled(false);", enemy)
        self.assertLess(death.index("AttackTellMarker->SetVisibility(false);"),
                        death.index("SetActorEnableCollision(false);"))

    def test_nonboss_hit_interrupts_active_windup_without_pathfinding(self):
        damage = self.enemy.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::", 1)[0]
        self.assertIn("if (!bBoss) HitReactionEndsAt = FMath::Max(HitReactionEndsAt, LastHitTime + 0.22f);", damage)
        interrupt = damage.split("if (!bBoss && ActualDamage > 0.f && Health > 0.f", 1)[1].split(
            "if (Health <= 0.0f) {", 1)[0]
        for value in ("FMath::IsFinite(Now)", "WindupEndsAt > Now", "WindupEndsAt = -1.f;",
                      "BossWarningEndTime = -1.f;", "bComboSecondHitPending = false;",
                      "bComboSecondHitActive = false;", "LockedAttackDirection = FVector::ZeroVector;",
                      "Now + 0.35f", "AttackTellMarker->SetVisibility(false);"):
            self.assertIn(value, interrupt)
        self.assertLess(damage.index("if (!bBoss && ActualDamage > 0.f && Health > 0.f"),
                        damage.index("if (Health <= 0.0f) {"))
        for costly in ("TActorIterator<", "LineTrace", "Sweep"):
            self.assertNotIn(costly, interrupt)


if __name__ == "__main__":
    unittest.main()
