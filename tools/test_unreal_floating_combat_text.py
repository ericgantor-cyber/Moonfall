"""Source-level checks for the WoW F-style floating combat results.

These checks verify implementation contracts; they are not a native
UE5.4 compile or visual gameplay test.
"""
import unittest
from pathlib import Path

SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"


class FloatingCombatTextChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h=(SRC/"MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero=(SRC/"MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.enemy=(SRC/"MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud=(SRC/"MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_bounded_queue_never_allocates_or_modifies_damage(self):
        self.assertIn("static constexpr int32 IncomingCombatFloatCapacity = 8;",self.hero_h)
        self.assertIn("FIncomingCombatFloat IncomingCombatFloats[IncomingCombatFloatCapacity];",self.hero_h)
        self.assertIn("void RecordIncomingCombatFloat(FName Outcome, float HealthLost);",self.hero_h)
        body=self.hero.split(
            "void AMoonfallHeroPawn::RecordIncomingCombatFloat(",1
        )[1].split("void AMoonfallHeroPawn::OpenOptions()",1)[0]
        self.assertIn("!bShowFloatingCombatText",body)
        self.assertIn("!FMath::IsFinite(HealthLost)",body)
        self.assertIn("HealthLost < 0.f",body)
        self.assertIn("!FMath::IsFinite(Now)",body)
        for name in ("HIT","PARRY","DODGE","GUARD","MISS"):
            self.assertIn('FName(TEXT("'+name+'"))',body)
        self.assertIn("Entry.StartedAt = Now;",body)
        self.assertIn("(NextIncomingCombatFloatSlot + 1) % IncomingCombatFloatCapacity",body)
        for banned in ("NewObject", "SpawnActor", "Add(", "Emplace(", "AttackDamage",
                       "SetHealth", "Health =", "SaveWorldProgress", "PlaySound"):
            self.assertNotIn(banned,body)

    def test_player_receives_only_authoritatively_classified_incoming_hit(self):
        resolved=self.enemy.split(
            "if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {",1
        )[1].split("if (bComboSecondHitPending)",1)[0]
        self.assertIn("if (!bBoss && !ImpactMoveId.IsNone()",resolved)
        self.assertIn("MoonfallAttackOutcome::Classify(",resolved)
        self.assertIn("Hero->RecordIncomingCombatFloat(OutcomeName, HealthLost);",resolved)
        self.assertIn("OnAttackImpactOutcome(ImpactMoveId, OutcomeName, HealthLost);",resolved)
        self.assertLess(resolved.index("MoonfallAttackOutcome::Classify("),
                        resolved.index("Hero->RecordIncomingCombatFloat("))
        self.assertLess(resolved.index("Hero->RecordIncomingCombatFloat("),
                        resolved.index("OnAttackImpactOutcome("))
        self.assertIn("Hero->ApplyMoonfallDamageFrom(",resolved)
        self.assertIn("Hero->TryRewardPerfectDodge()",resolved)

    def test_hud_projects_existing_outgoing_and_new_incoming_in_one_scan(self):
        self.assertEqual(self.hud.count("TActorIterator<AMoonfallEnemy>"),1)
        floats=self.hud.split(
            "// WoW F-style combat floats share the existing player's projection.",1
        )[1].split("int32 VisibleDamageNumbers = 0;",1)[0]
        self.assertIn("GetOwningPlayerController()",floats)
        self.assertIn("for (int32 Slot = 0; Slot < AMoonfallHeroPawn::IncomingCombatFloatCapacity; ++Slot)",floats)
        self.assertIn("DamageProjectionPC->ProjectWorldLocationToScreen(",floats)
        self.assertIn("WorldTime - Event.StartedAt",floats)
        self.assertIn("Age < 0.f || Age >= 1.1f",floats)
        self.assertIn("Event.Outcome.IsNone()",floats)
        self.assertIn("Screen.X",floats)
        self.assertIn("Canvas->SizeX",floats)
        self.assertIn("Canvas->SizeY",floats)
        self.assertIn("DrawText(ResultText",floats)
        for forbidden in ("TActorIterator", "SpawnActor", "NewObject", "LineTrace",
                          "GetWorldTimerManager", "FMath::Rand"):
            self.assertNotIn(forbidden,floats)
        self.assertIn("if (Hero->bShowFloatingCombatText)\n for (const AMoonfallEnemy* Enemy : NearbyDamageEnemies)",self.hud)
        self.assertIn("Enemy->DamageHistory[Slot]",self.hud)
        self.assertIn("VisibleDamageNumbers >= 32",self.hud)

    def test_text_meaning_is_clear_even_without_color(self):
        block=self.hud.split(
            "// WoW F-style combat floats share the existing player's projection.",1
        )[1].split("int32 VisibleDamageNumbers = 0;",1)[0]
        for phrase in ('TEXT("HIT -%.0f")','TEXT("PARRY!")',
                       'TEXT("DODGE!")','TEXT("BLOCKED!")','TEXT("MISS!")'):
            self.assertIn(phrase,block)
        self.assertIn("FLinearColor::White",block)
        self.assertIn("Hero->CombatTextPalette == 2",block)
        self.assertIn("Hero->CombatTextPalette == 1",block)
        self.assertIn("bHit ? FLinearColor",block)
        self.assertIn("DrawRect(FLinearColor(0.f,0.f,0.f,0.46f*Alpha)",block)
        self.assertIn("Hero->CombatTextScale,0.75f,2.f",block)

    def test_reduce_motion_and_disable_are_real_runtime_controls(self):
        self.assertIn("bool bShowFloatingCombatText = true;",self.hero_h)
        self.assertIn("int32 CombatTextPalette = 0;",self.hero_h)
        block=self.hud.split(
            "// WoW F-style combat floats share the existing player's projection.",1
        )[1].split("int32 VisibleDamageNumbers = 0;",1)[0]
        self.assertIn("if (Hero->bShowFloatingCombatText && DamageProjectionPC",block)
        self.assertIn("Hero->bReduceMotion ? 0.f : Age * 68.f",block)
        self.assertIn("FMath::Clamp(1.f - Age / 1.1f, 0.f, 1.f)",block)
        self.assertIn("FMath::IsFinite(X) || !FMath::IsFinite(Y)",block)
        self.assertIn("!Hero->bShowFloatingCombatText",self.hero)
        self.assertIn("Hero->CombatTextPalette == 2",self.hud.split(
            "// Reuse one controller lookup for every projected damage label",1
        )[-1])

    def test_settings_mouse_and_controller_indices_match_hud(self):
        options=self.hud.split("if (Hero->bOptionsOpen) {",1)[1].split("if (Hero->bInventoryOpen)",1)[0]
        mouse=self.hero.split("// Match all ten options rows",1)[1].split(
            "if (bInventoryOpen)",1)[0]
        self.assertIn("(MenuHeight - 130.f) / 10.f",options)
        self.assertIn("(MenuHeight - 130.f) / 10.f",mouse)
        self.assertIn("for (int32 i = 0; i < 10; ++i)",options)
        self.assertIn("ClickedRow >= 0 && ClickedRow < 10",mouse)
        self.assertIn("OptionsSelection = (OptionsSelection + 9) % 10",self.hero)
        self.assertIn("OptionsSelection = (OptionsSelection + 1) % 10",self.hero)
        self.assertIn('TEXT("Floating results")',options)
        self.assertIn('TEXT("Text palette")',options)
        self.assertIn("Hero->bShowFloatingCombatText ? TEXT(\"On\") : TEXT(\"Off\")",options)
        self.assertIn('TEXT("Contrast")',options)
        self.assertIn('TEXT("Mono")',options)
        self.assertIn("CombatTextPalette = (CombatTextPalette + (Direction < 0 ? 2 : 1)) % 3;",self.hero)
        self.assertIn('TEXT("ShowFloatingCombatText")',self.hero)
        self.assertIn('TEXT("CombatTextPalette")',self.hero)
        self.assertIn("GConfig->Flush(false, GGameUserSettingsIni);",self.hero)
        self.assertIn("CombatTextScale + Direction * 0.25f",self.hero)

    def test_options_geometry_fits_standard_720p_viewport(self):
        width,height=1280,720
        menu_height=min(550,max(280,height-24))
        gap=(menu_height-130)/10
        top=(height-menu_height)/2
        last_y=top+63+9*gap
        row_h=min(38,gap-2)
        help_y=top+menu_height-28
        self.assertLess(last_y+row_h,help_y)
        self.assertGreater(row_h,25)
        self.assertLess(top+menu_height,height)
        self.assertEqual(width,1280)


if __name__=="__main__":
    unittest.main()
