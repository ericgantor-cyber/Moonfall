"""Regression checks for the dependent #1052-on-#1055 HUD integration.

These tests verify source contracts, not native Unreal compilation.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class HUDCombatIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.layout = (SRC / "MoonfallHUDLayout.h").read_text(encoding="utf-8")
        cls.float_rules = (SRC / "MoonfallCombatFloatLayout.h").read_text(encoding="utf-8")

    def test_eleven_options_preserve_floating_text_choices(self):
        options = self.hud.split("if (Hero->bOptionsOpen && !Hero->bHUDLayoutEditing) {", 1)[1].split(
            "if (Hero->bInventoryOpen)", 1)[0]
        for name in ('TEXT("Floating results")', 'TEXT("Text palette")',
                     'TEXT("HUD layout editor")', 'TEXT("Contrast")',
                     'TEXT("Mono")', 'TEXT("Classic")'):
            self.assertIn(name, options)
        self.assertIn('for (int32 i = 0; i < 11; ++i)', options)
        self.assertIn('const float MenuRowGap = (MenuHeight - 130.f) / 11.f;', options)
        self.assertIn('FMath::Clamp(Hero->OptionsSelection, 0, 10)', options)
        self.assertIn('OptionsSelection = (OptionsSelection + 10) % 11;', self.hero)
        self.assertIn('OptionsSelection = (OptionsSelection + 1) % 11;', self.hero)
        self.assertIn('OptionsSelection == 10', self.hero)

    def test_mouse_click_geometry_matches_rendered_option_rows(self):
        mouse = self.hero.split("// Match all eleven options rows", 1)[1].split(
            "if (bInventoryOpen)", 1)[0]
        self.assertIn('const float RowGap = (MenuHeight - 130.f) / 11.f;', mouse)
        self.assertIn('ClickedRow >= 0 && ClickedRow < 11', mouse)
        self.assertIn('OptionsSelection = ClickedRow;', mouse)
        self.assertIn('AdjustOption(-1);', mouse)
        self.assertIn('AdjustOption(1);', mouse)

    def test_three_panels_share_same_viewport_bounded_geometry(self):
        self.assertIn('constexpr int32 PanelCount = 3;', self.layout)
        self.assertIn('constexpr int32 Orb = 0;', self.layout)
        self.assertIn('constexpr int32 Quest = 1;', self.layout)
        self.assertIn('constexpr int32 Boss = 2;', self.layout)
        self.assertIn('FMath::IsFinite(ViewWidth)', self.layout)
        self.assertIn('FMath::IsFinite(ViewHeight)', self.layout)
        self.assertIn('MinScale = 0.70f', self.layout)
        self.assertIn('MaxScale = 1.60f', self.layout)
        self.assertIn('Width = FMath::Min(Width', self.layout)
        self.assertIn('Height = FMath::Min(Height', self.layout)
        for panel in ('Orb', 'Quest', 'Boss'):
            self.assertIn('MoonfallHUDLayout::Geometry(MoonfallHUDLayout::'+panel, self.hud)
        self.assertIn('const auto R = MoonfallHUDLayout::Geometry(Panel', self.hud)
        self.assertIn('const auto R = MoonfallHUDLayout::Geometry(Panel', self.hero)

    def test_hero_panel_state_is_transient_and_settings_are_independent(self):
        for s in (
            'FVector2D HudPanelOffsets[3]', 'float HudPanelScales[3]',
            'bool bHUDLayoutEditing = false;', 'int32 HudSelectedPanel = 0;',
            'int32 HudDraggingPanel = INDEX_NONE;',
            'void LoadHUDLayout();', 'void SaveHUDLayout() const;',
            'void ResetHUDLayout();', 'void ExitHUDLayoutEditor();',
            'void AdjustHUDPanelScale(int32 Direction);',
        ):
            self.assertIn(s, self.hero_h)
        for s in ('TEXT("Moonfall.HUDLayout")','TEXT("Panel%dX")',
                  'TEXT("Panel%dY")','TEXT("Panel%dScale")',
                  'GGameUserSettingsIni', 'MoonfallHUDLayout::SafeScale',
                  'MoonfallHUDLayout::SafeOffset', 'GConfig->Flush(false, GGameUserSettingsIni)'):
            self.assertIn(s, self.hero)
        self.assertIn('LoadHUDLayout();', self.hero)
        self.assertIn('if (bHUDLayoutEditing) SaveHUDLayout();', self.hero)
        self.assertNotIn('SaveWorldProgress', self.layout)
        self.assertNotIn('UPROPERTY', self.layout)

    def test_mouse_keyboard_controller_drag_and_resize(self):
        for s in ('void AMoonfallHeroPawn::HandleHUDLayoutMousePress()',
                  'void AMoonfallHeroPawn::ReleaseInventoryPointer()',
                  'void AMoonfallHeroPawn::ZoomCameraIn()',
                  'void AMoonfallHeroPawn::ZoomCameraOut()',
                  'void AMoonfallHeroPawn::OptionsUp()',
                  'void AMoonfallHeroPawn::OptionsDown()',
                  'void AMoonfallHeroPawn::OptionsLeft()',
                  'void AMoonfallHeroPawn::OptionsRight()'):
            self.assertIn(s, self.hero)
        self.assertIn('if (bHUDLayoutEditing) { AdjustHUDPanelScale(1); return; }', self.hero)
        self.assertIn('if (bHUDLayoutEditing) { AdjustHUDPanelScale(-1); return; }', self.hero)
        self.assertIn('if (bHUDLayoutEditing) { HandleHUDLayoutMousePress(); return; }',self.hero)
        self.assertIn('if (bHUDLayoutEditing) { ExitHUDLayoutEditor(); return; }',self.hero)
        self.assertIn('MoonfallHUDLayout::SafeOffset(',self.hero)
        self.assertIn('HudDraggingPanel = INDEX_NONE;',self.hero)
        self.assertIn('TEXT("RESET")',self.hud)
        self.assertIn('TEXT("DONE")',self.hud)

    def test_boss_telegraphs_remain_timed_and_separate_from_outcome_text(self):
        for s in ('Boss->BossWarningEndTime',
                  'Boss->bDungeonBossAttackDodgeOnly',
                  'TEXT("RED - DODGE ONLY")',
                  'TEXT("BLUE - PARRY / DODGE")',
                  'TEXT("IMPACT SOON")',
                  'MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Boss'):
            self.assertIn(s, self.hud)
        self.assertIn('const float Width = FMath::Max(8.f, BossRect.Width - 12.f * S);',self.hud)
        self.assertIn('const bool bCanShowQuestTracker',self.hud)
        self.assertIn('MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Quest',self.hud)
        self.assertIn('MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Orb',self.hud)

    def test_enemy_only_floating_text_kept_with_hero_silhouette_protection(self):
        self.assertEqual(self.hud.count('TActorIterator<AMoonfallEnemy>'), 1)
        floats=self.hud.split('// All floating combat text is anchored exclusively to the attacker.',1)[1]
        for s in ('Enemy->CombatOutcomeHistory[Slot]',
                  'MoonfallCombatFloatLayout::TryPlace(',
                  'HeroScreenExclusion',
                  'EnemyDamageLocation + FVector(0.f,0.f,245.f+Rise)',
                  'TEXT("HIT YOU -%.0f")',
                  'TEXT("PARRY!")','TEXT("DODGE!")','TEXT("BLOCKED!")','TEXT("MISS!")',
                  'MoonfallCombatFloatLayout::MaxVisibleAttackOutcomes'):
            self.assertIn(s,floats)
        self.assertNotIn('Hero->GetActorLocation()',floats)
        self.assertIn('MaxVisibleDealtDamage = 32;',self.float_rules)
        self.assertIn('MaxVisibleAttackOutcomes = 16;',self.float_rules)

    def test_no_god_art_save_or_combat_mechanics_mutation_in_layout_policy(self):
        for banned in ('Z ', 'Nyra ', 'Art', 'SaveWorldProgress', 'UPROPERTY', 'Damage',
                       'TActorIterator', 'SpawnActor', 'GetWorldTimerManager'):
            self.assertNotIn(banned,self.layout)
        self.assertIn('const float Radius = OrbRect.Width / 2.5f;',self.hud)
        self.assertIn('Hero->bReduceMotion',self.hud)


if __name__=='__main__':
    unittest.main()
