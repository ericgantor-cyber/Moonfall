"""Source integration checks for Moonfall's AAA-directed camera impact polish."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
UE = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class AAAImpactIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (UE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.decl = (UE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.policy = (UE / "MoonfallImpactFeedback.h").read_text(encoding="utf-8")

    def test_only_confirmed_enemy_damage_adds_camera_weight(self):
        start = self.hero.index("const float ActualSpellDamage = FMath::Clamp(")
        section = self.hero[start:self.hero.index(
            "if (ActualSpellDamage > 0.f && Enemy->Health > 0.f", start)]
        self.assertIn("if (ActualSpellDamage > 0.f) {", section)
        self.assertIn("HitComboStage = bComboAlive ?", section)
        self.assertIn("MoonfallImpactFeedback::ConfirmedHit(", section)
        self.assertLess(section.index("if (ActualSpellDamage > 0.f) {"),
                        section.index("MoonfallImpactFeedback::ConfirmedHit("))
        self.assertLess(section.index("HitComboStage = bComboAlive ?"),
                        section.index("MoonfallImpactFeedback::ConfirmedHit("))
        self.assertIn("HitComboStage, Enemy->bBoss, Enemy->Health <= 0.f", section)
        self.assertIn("bReduceMotion, bOptionsOpen || bInventoryOpen || bAtlasOpen",
                      section)
        self.assertIn("ImpactCameraOffset = HitFeedback.Offset;", section)
        self.assertIn("LastOffensiveImpactAt = HitFeedback.LastAt;", section)
        self.assertIn("OnSuccessfulHitCombo(HitComboStage, Hand == 0);", section)

    def test_reuses_bounded_spring_arm_and_accessibility(self):
        self.assertIn('#include "MoonfallImpactFeedback.h"', self.hero)
        self.assertIn("float LastOffensiveImpactAt = -100.f;", self.decl)
        self.assertIn("float ImpactCameraOffset = 0.f;", self.decl)
        self.assertIn(
            "ImpactCameraOffset = FMath::Clamp(FMath::FInterpTo(", self.hero)
        self.assertIn(
            "SafeDistance + (bReduceMotion ? 0.f : ImpactCameraOffset)",
            self.hero,
        )
        self.assertIn("if (bReduceMotion) ImpactCameraOffset = 0.f;", self.hero)
        self.assertIn("MaxImpulse = 24.f", self.policy)
        self.assertIn("MinHitGapSeconds = 0.09f", self.policy)
        self.assertIn("Stage == 4 || Stage == 8 || Stage == 12", self.policy)
        self.assertIn("std::isfinite(DealtDamage)", self.policy)
        self.assertIn("if (bReduceMotion) return {0.f, SafeLast, false};",
                      self.policy)

    def test_spell_impacts_are_blueprint_authored_not_debug_wireframes(self):
        self.assertIn(
            "void OnSpellImpact(int32 School, bool bLeftHand, FVector ImpactPoint,",
            self.decl)
        self.assertIn("float AppliedDamage, bool bCritical, bool bKillingBlow);",
                      self.decl)
        self.assertIn("BlueprintImplementableEvent", self.decl)
        start = self.hero.index("bool AMoonfallHeroPawn::TryCast(")
        end = self.hero.index("void AMoonfallHeroPawn::CastLeft()", start)
        casting = self.hero[start:end]
        confirmed = casting.index("if (ActualSpellDamage > 0.f) {")
        impact = casting.index("OnSpellImpact(School, Hand == 0,")
        self.assertGreater(impact, confirmed)
        self.assertGreater(impact, casting.index("OnSuccessfulHitCombo(", confirmed))
        self.assertGreater(impact, casting.index("ParryCounterUntil = -1.f;", confirmed))
        self.assertLess(impact, casting.index("Mana -= Costs[School] * Multiplier;"))
        self.assertIn(
            "if (ActualSpellDamage > 0.f && !Hit.ImpactPoint.ContainsNaN())",
            casting,
        )
        self.assertIn("const bool bSpellKillingBlow = Enemy->Health <= 0.f;", casting)
        self.assertLess(
            casting.index("const bool bSpellKillingBlow"),
            casting.index("OnSuccessfulHitCombo(", confirmed),
        )
        self.assertIn(
            "Hit.ImpactPoint, ActualSpellDamage,\n     bCriticalSpell, bSpellKillingBlow",
            casting,
        )
        self.assertEqual(casting.count("OnSpellImpact("), 1)
        self.assertIn("OnSpellCast(School, Hand == 0);", casting)
        self.assertNotIn("DrawDebugSphere(", casting)
        self.assertNotIn("DrawDebugLine(", casting)
        self.assertNotIn("DrawDebugString(", casting)
        self.assertNotIn("SpawnActor(", casting)
        self.assertNotIn("SetGlobalTimeDilation(", casting)

    def test_no_gameplay_state_or_art_rewrite(self):
        # Read-only presentation path: neither rewarding extra damage nor
        # blocking movement/casts with time dilation.
        for invalid in ("SetGlobalTimeDilation(", "SetTimeDilation(",
                        "SpawnActor(", "TakeSpellDamage(", "AddExperience(",
                        "AwardExperience(", "DrawText(", "GEngine->AddOnScreenDebugMessage",
                        "UCameraShake", "UGameplayStatics"):
            self.assertNotIn(invalid, self.policy)
        self.assertNotIn("UPROPERTY(", self.policy)
        self.assertNotIn("ZOriginalArt", self.policy)
        self.assertNotIn("NyraOriginalArt", self.policy)


if __name__ == "__main__":
    unittest.main()
