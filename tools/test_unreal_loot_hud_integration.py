"""Contract tests for the stacked #1053 loot art + #1056 HUD integration.

Checks native source and art references only. A real UE5.4 build and
in-game rendering remain mandatory and unverified here.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
ART = ROOT / "MoonfallUnreal" / "Data" / "LootArt"


class LootHUDIntegrationChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.hud_h = (SRC / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.hero_h = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.loot = (SRC / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        cls.rules = (SRC / "MoonfallLootArtRules.h").read_text(encoding="utf-8")

    def test_loot_art_integration_keeps_both_existing_hud_modules(self):
        for source in ('#include "MoonfallLootArtRules.h"',
                       '#include "MoonfallHUDLayout.h"',
                       '#include "MoonfallCombatFloatLayout.h"'):
            self.assertIn(source, self.hud)
        self.assertEqual(self.hud.count("void AMoonfallCombatHUD::DrawHUD()"), 1)
        self.assertEqual(self.hud.count("void AMoonfallCombatHUD::DrawLootIcon("), 1)
        self.assertEqual(self.hud.count("AMoonfallCombatHUD::AMoonfallCombatHUD()"), 1)
        self.assertIn("MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Orb", self.hud)
        self.assertIn("MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Quest", self.hud)
        self.assertIn("MoonfallHUDLayout::Geometry(MoonfallHUDLayout::Boss", self.hud)

    def test_all_six_original_atlas_blobs_import_and_render(self):
        self.assertEqual(len(list(ART.glob("Moonfall_Loot*Atlas*.png"))), 6)
        self.assertIn("Canvas->K2_DrawTexture(IconTexture", self.hud)
        for name in ("LootIconAtlas", "LootVariantAtlas", "LootRelicAtlas",
                     "LootMasterworkAtlas", "LootAscendantAtlas",
                     "LootWayfarerAtlas"):
            self.assertIn(name + " = nullptr;", self.hud_h)
        self.assertIn("if (IsValid(IconTexture) && MoonfallLootArt::ValidCell(Cell))", self.hud)
        self.assertIn("} else {\n  // Accessible procedural silhouettes", self.hud)
        self.assertIn("MoonfallLootArt::Select(", self.hud)
        self.assertIn("MoonfallLootArt::ValidEquipment(", self.hud)

    def test_inventory_icons_appear_only_for_valid_equipment(self):
        bag = self.hud.split("if (Hero->bInventoryOpen)", 1)[1].split(
            "if (Boss && BossDistanceSq", 1)[0]
        self.assertIn("Hero->InventoryItemCodes.IsValidIndex(Hero->InventorySelection)", bag)
        self.assertIn("SelectedCode == SelectedTrait * 100000 + SelectedSlot * 10000", bag)
        self.assertIn("DrawLootIcon(SelectedSlot, SelectedRarity, SelectedTrait, SelectedTier,", bag)
        self.assertIn("const float IconSide = FMath::Clamp(RowSpacing - 5.f, 16.f, 30.f);", bag)
        self.assertIn("if (bValid)\n    DrawLootIcon(Slot, Rarity, Affix, Tier,", bag)
        self.assertIn("const FString CompactRow =", bag)
        self.assertIn("Hero->GetInventoryDisplayOrder()", bag)
        self.assertIn("Hero->GetInventoryVisibleRows()", bag)
        self.assertIn("Hero->HitTestInventoryRow()", bag)
        self.assertNotIn("InventoryItemCodes.Add(", bag)
        self.assertNotIn("TryCollect()", bag)

    def test_loot_art_does_not_move_combat_text_to_player(self):
        # Combat attribution is fixed in #1055. Loot icons must not
        # introduce a second enemy scan or alter the player exclusion.
        self.assertEqual(self.hud.count("TActorIterator<AMoonfallEnemy>"), 1)
        combat = self.hud.split(
            "// All floating combat text is anchored exclusively to the attacker.", 1
        )[1]
        for marker in ("Enemy->CombatOutcomeHistory[Slot]",
                       "HeroScreenExclusion", "MoonfallCombatFloatLayout::TryPlace(",
                       "EnemyDamageLocation + FVector(0.f,0.f,245.f+Rise)",
                       "TEXT(\"HIT YOU -%.0f\")", "TEXT(\"PARRY!\")",
                       "TEXT(\"DODGE!\")", "TEXT(\"BLOCKED!\")",
                       "TEXT(\"MISS!\")"):
            self.assertIn(marker, combat)
        self.assertNotIn("Hero->GetActorLocation()", combat)

    def test_eleven_options_and_hud_editor_remain_in_sync(self):
        menu = self.hud.split(
            "if (Hero->bOptionsOpen && !Hero->bHUDLayoutEditing) {", 1
        )[1].split("if (Hero->bInventoryOpen)", 1)[0]
        for x in ('TEXT("Floating results")', 'TEXT("Text palette")',
                  'TEXT("HUD layout editor")', 'TEXT("Classic")',
                  'TEXT("Contrast")', 'TEXT("Mono")',
                  "for (int32 i = 0; i < 11; ++i)",
                  "const float MenuRowGap = (MenuHeight - 130.f) / 11.f;"):
            self.assertIn(x, menu)
        self.assertIn("ClickedRow >= 0 && ClickedRow < 11", self.hero)
        self.assertIn("OptionsSelection == 10", self.hero)
        self.assertIn("Moonfall.HUDLayout", self.hero)

    def test_pickup_rendering_does_not_change_pickup_ownership_or_collision(self):
        for x in ("PickupDetailA", "PickupDetailB", "RarityBeacon",
                  "CollectPersistedGroundEquipmentCode",
                  "CollectDungeonTreasureCode", "PersistentDropId"):
            self.assertIn(x, self.loot)
        self.assertIn("PickupDetailA->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.loot)
        self.assertIn("PickupDetailB->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.loot)
        self.assertIn("PickupDetailA->SetMaterial(0, PickupMaterial)", self.loot)
        self.assertIn("PickupDetailB->SetMaterial(0, PickupMaterial)", self.loot)

    def test_new_visual_helpers_do_not_modify_save_format_or_character_art(self):
        renderer = self.hud.split("void AMoonfallCombatHUD::DrawLootIcon(", 1
            )[1].split("void AMoonfallCombatHUD::DrawHUD()", 1)[0]
        for forbidden in ("SaveWorldProgress", "InventoryItemCodes.Add(",
                          "ApplyMoonfallDamage", "SpawnActor", "SetActorLocation",
                          "TActorIterator", "Z_character", "Nyra_character"):
            self.assertNotIn(forbidden, renderer)
        self.assertNotIn("UPROPERTY", self.rules)
        self.assertNotIn("UTexture2D", self.rules)
        self.assertNotIn("Health =", renderer)
        self.assertIn("DrawRect(", renderer)

    def test_gallery_and_importer_require_real_ue_editor_import(self):
        importer = ROOT / "MoonfallUnreal" / "Build" / "Import-Loot-Art.py"
        gallery = ART / "Moonfall_LootArt_Gallery.html"
        self.assertTrue(importer.is_file() and gallery.is_file())
        txt = importer.read_text(encoding="utf-8")
        self.assertIn('DESTINATION = "/Game/Moonfall/LootArt"', txt)
        self.assertIn("unreal.AssetImportTask()", txt)
        self.assertIn("validate_atlas(", txt)
        self.assertIn("save_loaded_asset(asset)", txt)
        self.assertIn("/Game/Moonfall/LootArt/T_MoonfallLootAtlas.T_MoonfallLootAtlas", self.hud)
        self.assertIn("Original", (ART / "README.md").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
