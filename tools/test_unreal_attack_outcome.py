"""Offline integration tests for truthful hit and combat-audio outcomes.

No native UE5.4 compilation, authored audio, or gameplay testing here.
"""
import unittest
from pathlib import Path

SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"


class RealAttackOutcomeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy=(SRC/"MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.header=(SRC/"MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.rules=(SRC/"MoonfallAttackOutcome.h").read_text(encoding="utf-8")
        cls.hud=(SRC/"MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_event_can_distinguish_miss_guard_parry_dodge_hit(self):
        self.assertIn("void OnAttackImpactOutcome(FName MoveId, FName Outcome, float HealthLost);",self.header)
        for outcome in ("HIT","PARRY","DODGE","GUARD","MISS"):
            self.assertIn('TEXT("'+outcome+'")',self.enemy)
        for state in ("Miss","Parry","Dodge","Hit","Guard"):
            self.assertIn("EResult::"+state,self.rules)
        self.assertIn("const float HeroHealthBefore = Hero->Health;",self.enemy)
        self.assertIn("const float HealthLost = FMath::Max(0.f, HeroHealthBefore - Hero->Health);",self.enemy)

    def test_outcome_resolves_after_collision_and_original_defenses(self):
        frame=self.enemy.split("if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {",1
          )[1].split("// Keep the enemy committed to a telegraphed strike",1)[0]
        for anchor in ("const FName ImpactMoveId = ActiveNonBossTelegraphMove;",
            "EndNonBossTelegraph(true);",
            "const bool bWithinBasicAttack", "const bool bBlocked =",
            "Hero->TryRewardPerfectDodge()","Hero->ApplyMoonfallDamageFrom(",
            "MoonfallAttackOutcome::Classify(","OnAttackImpactOutcome("):
            self.assertIn(anchor,frame)
        self.assertLess(frame.index("const FName ImpactMoveId"),
                        frame.index("EndNonBossTelegraph(true);"))
        self.assertLess(frame.index("Hero->ApplyMoonfallDamageFrom("),
                        frame.index("MoonfallAttackOutcome::Classify("))
        self.assertLess(frame.index("MoonfallAttackOutcome::Classify("),
                        frame.index("OnAttackImpactOutcome("))
        self.assertIn("bImpactReachedAndClear = true;",frame)
        self.assertIn("bImpactPerfectDodged = bPerfectDodge;",frame)
        self.assertIn("bImpactParried = bParried;",frame)
        self.assertEqual(frame.count("OnAttackImpactOutcome("),1)

    def test_blueprint_callbacks_recheck_enemy_and_hero_lifetime(self):
        frame=self.enemy.split("if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {",1
          )[1].split("// Keep the enemy committed to a telegraphed strike",1)[0]
        after_resolve=frame.split("EndNonBossTelegraph(true);",1)[1].split(
            "// Heavy elite crushes",1)[0]
        self.assertIn("!IsValid(Hero)",after_resolve)
        self.assertIn("!FMath::IsFinite(Hero->Health)",after_resolve)
        self.assertIn("IsActorBeingDestroyed()",after_resolve)
        after_defense=frame.split("OnAttackImpactOutcome(",1)[1]
        self.assertIn("!IsActorBeingDestroyed()",after_defense)
        self.assertIn("FMath::IsFinite(Health) && Health > 0.f",after_defense)

    def test_optional_sound_plays_only_for_real_hit_or_parry(self):
        self.assertIn("TObjectPtr<USoundBase> AttackHitSound = nullptr;",self.header)
        self.assertIn("TObjectPtr<USoundBase> AttackParrySound = nullptr;",self.header)
        self.assertIn("float NextAttackImpactSoundAt = 0.f;",self.header)
        impact=self.enemy.split("OnAttackImpactOutcome(ImpactMoveId, OutcomeName, HealthLost);",1
          )[1].split("if (bComboSecondHitPending)",1)[0]
        self.assertIn("MoonfallAttackOutcome::MayPlayHitSound(Outcome)",impact)
        self.assertIn("MoonfallAttackOutcome::MayPlayParrySound(Outcome)",impact)
        self.assertIn("NextAttackImpactSoundAt = Now + 0.4f;",impact)
        self.assertIn("Distance <= 1000.f",impact)
        self.assertIn("if (IsValid(Sound)) {",impact)
        self.assertIn("UGameplayStatics::PlaySoundAtLocation(",impact)
        self.assertNotIn("SpawnActor",impact)
        self.assertNotIn("TActorIterator",impact)

    def test_original_boss_gods_and_damage_rules_unmodified(self):
        self.assertIn("if (!bBoss && !ImpactMoveId.IsNone()",self.enemy)
        self.assertIn("const bool bDodgeOnlyStrike = bBoss",self.enemy)
        self.assertIn('FName(TEXT("z_void_reaper"))',self.enemy)
        self.assertIn('FName(TEXT("nyra_eclipse_queen"))',self.enemy)
        self.assertIn("BossSpecialMove == 5",self.enemy)
        self.assertIn("Hero->ApplyMoonfallDodgeOnlyDamageFrom(",self.enemy)
        self.assertEqual(self.hud.count("TActorIterator<AMoonfallEnemy>"),1)
        self.assertNotIn("Saved",self.rules)
        self.assertNotIn("Random",self.rules)


if __name__=="__main__":
    unittest.main()
