"""Source and asset contracts for Moonfall Unreal loot artwork.

No Unreal engine compilation or packaged-game rendering is claimed.
"""
import struct
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
ICON=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootAtlas.png"
IMPORTER=ROOT/"MoonfallUnreal"/"Build"/"Import-Loot-Art.py"


class LootArtTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pickup=(SRC/"MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        cls.pickup_h=(SRC/"MoonfallGroundLoot.h").read_text(encoding="utf-8")
        cls.hud=(SRC/"MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.hud_h=(SRC/"MoonfallCombatHUD.h").read_text(encoding="utf-8")

    def test_atlas_is_real_png_with_six_square_cells(self):
        raw=ICON.read_bytes()
        self.assertEqual(raw[:8],bytes.fromhex("89504e470d0a1a0a"))
        self.assertGreater(len(raw),1024)
        self.assertLess(len(raw),512*1024)
        width,height=struct.unpack(">II",raw[16:24])
        self.assertEqual((width,height),(120,80))
        self.assertEqual(width//3,height//2)

    def test_import_path_matches_hud_references(self):
        importer=IMPORTER.read_text(encoding="utf-8")
        self.assertIn('Moonfall_LootAtlas.png',importer)
        self.assertIn('/Game/Moonfall/LootArt',importer)
        self.assertIn('T_MoonfallLootAtlas',importer)
        self.assertIn('unreal.AssetImportTask()',importer)
        self.assertIn('replace_existing',importer)
        self.assertIn('save_loaded_asset(asset)',importer)
        self.assertIn('/Game/Moonfall/LootArt/T_MoonfallLootAtlas.T_MoonfallLootAtlas',self.hud)
        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootIconAtlas',self.hud_h)
        self.assertIn('if (IsValid(LootIconAtlas))',self.hud)
        self.assertIn('FVector2D(float(Slot)/3.f,0.f)',self.hud)
        self.assertIn('FVector2D(1.f/3.f,0.5f)',self.hud)
        self.assertIn('Canvas->K2_DrawTexture',self.hud)

    def test_art_is_safe_for_existing_item_codes(self):
        self.assertIn('DrawLootIcon(Slot, Rarity, Left + 17.f',self.hud)
        self.assertIn('const bool bValid = Code > 0',self.hud)
        self.assertIn('if (bValid)\n    DrawLootIcon(',self.hud)
        self.assertIn('if (Slot == 0)',self.pickup)
        self.assertIn('else if (Slot == 1)',self.pickup)
        self.assertIn('GearSphere',self.pickup)
        self.assertLess(self.pickup.index('ItemCode = InItemCode;'),
                        self.pickup.index('// Slot changes silhouette'))
        self.assertIn('InItemCode != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier',self.pickup)
        self.assertIn('PersistentDropId',self.pickup)
        self.assertIn('CollectPersistedGroundEquipmentCode',self.pickup)
        self.assertIn('CollectDungeonTreasureCode',self.pickup)
        self.assertIn('TryCollect()',self.pickup)
        self.assertEqual(self.pickup.count('PrimaryActorTick.bCanEverTick = false'),1)
        for property_name in ('PickupDetailA','PickupDetailB'):
            self.assertIn('UStaticMeshComponent* '+property_name,self.pickup_h)
            self.assertIn(property_name+'->SetCollisionEnabled(ECollisionEnabled::NoCollision)',self.pickup)
        self.assertNotIn('Z_character',self.hud)
        self.assertNotIn('Nyra_character',self.hud)


if __name__=="__main__":
    unittest.main()
