"""Source and asset contracts for Moonfall Unreal loot artwork.

No Unreal engine compilation or packaged-game rendering is claimed.
"""
import struct
import ast
import zlib
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
ICON=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootAtlas.png"
VARIANTS=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootVariants_Atlas.png"
RELICS=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootRelics_Atlas.png"
MASTERWORKS=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootMasterworks_Atlas.png"
ASCENDANTS=ROOT/"MoonfallUnreal"/"Data"/"LootArt"/"Moonfall_LootAscendants_Atlas.png"
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

    def test_variant_atlas_has_nine_square_transparent_cells(self):
        raw=VARIANTS.read_bytes()
        self.assertEqual(raw[:8],bytes.fromhex("89504e470d0a1a0a"))
        self.assertGreater(len(raw),4096)
        self.assertLess(len(raw),512*1024)
        width,height=struct.unpack(">II",raw[16:24])
        self.assertEqual((width,height),(120,120))
        self.assertEqual((width//3,height//3),(40,40))
        self.assertIn(b"tRNS",raw,"Generated icons must keep transparency")

    def test_relic_atlas_has_nine_square_cells(self):
        raw=RELICS.read_bytes()
        self.assertEqual(raw[:8],bytes.fromhex("89504e470d0a1a0a"))
        self.assertLess(len(raw),512*1024)
        self.assertEqual(struct.unpack(">II",raw[16:24]),(120,120))
        self.assertIn(b"tRNS",raw,"Relic atlas requires real transparency")

    def test_masterwork_atlas_is_compact_and_transparent(self):
        raw=MASTERWORKS.read_bytes()
        self.assertEqual(raw[:8],bytes.fromhex("89504e470d0a1a0a"))
        self.assertGreater(len(raw),1024)
        self.assertLess(len(raw),512*1024)
        self.assertEqual(struct.unpack(">II",raw[16:24]),(120,120))
        self.assertIn(b"tRNS",raw)
        self.assertEqual((120//3,120//3),(40,40))

    def test_ascendant_atlas_is_real_transparent_png(self):
        raw=ASCENDANTS.read_bytes()
        self.assertEqual(raw[:8],bytes.fromhex("89504e470d0a1a0a"))
        self.assertGreater(len(raw),2048)
        self.assertLess(len(raw),512*1024)
        self.assertEqual(struct.unpack(">II",raw[16:24]),(120,120))
        self.assertIn(b"tRNS",raw)

    @classmethod
    def importer_validator(cls):
        # Load *only* the pure validator from the Unreal Editor script.
        # Do not import unreal, which is unavailable on GitHub-hosted Linux.
        source=IMPORTER.read_text(encoding="utf-8")
        parsed=ast.parse(source)
        fn=next(n for n in parsed.body
                if isinstance(n,ast.FunctionDef) and n.name=="validate_atlas")
        module=ast.Module(body=[fn],type_ignores=[])
        code=compile(module,str(IMPORTER),"exec")
        context={"Path":Path,"struct":struct,"zlib":zlib,
                 "PNG_SIGNATURE":bytes.fromhex("89504e470d0a1a0a"),
                 "MAX_PNG_BYTES":512*1024}
        exec(code,context)
        return context["validate_atlas"]

    def test_all_five_png_files_pass_real_chunk_crc_checks(self):
        validate=self.importer_validator()
        for path,size in ((ICON,(120,80)),(VARIANTS,(120,120)),
                          (RELICS,(120,120)),(MASTERWORKS,(120,120)),
                          (ASCENDANTS,(120,120))):
            with self.subTest(filename=path.name):
                validate(path,size)

    def test_png_validator_rejects_corruption_before_import(self):
        validate=self.importer_validator()
        original=RELICS.read_bytes()
        with TemporaryDirectory() as folder:
            path=Path(folder)/"bad.png"
            tests=(
                (original[:28]+bytes([original[28]^1])+original[29:],"checksum"),
                (original[:-8],"Truncated|Incomplete"),
                (original[:16]+bytes((0,0,1,0))+original[20:],
                 "checksum|dimensions"),
                (original+b"junk","Incomplete"),
            )
            for altered,expected in tests:
                path.write_bytes(altered)
                with self.subTest(error=expected),self.assertRaisesRegex(
                    ValueError,expected):
                    validate(path,(120,120))

    def test_trait_art_mapping_never_changes_item_codes(self):
        rules=(SRC/"MoonfallLootArtRules.h").read_text(encoding="utf-8")
        self.assertIn("constexpr FCell Select(",rules)
        self.assertIn("EAtlas::Masterwork",rules)
        self.assertIn("EAtlas::Ascendant",rules)
        self.assertIn("bAscendantLoaded && Rarity == 5 && Tier >= 22",rules)
        self.assertIn("bMasterworkLoaded && Rarity >= 4 && Tier >= 16",rules)
        self.assertIn("constexpr bool ValidCell(",rules)
        self.assertIn("bRelicLoaded && Rarity >= 3 && Tier >= 8",rules)
        self.assertIn("Trait == 3 ? 2 : 0",rules)
        self.assertIn("(Tier + Rarity + Trait) % 3",rules)
        self.assertIn("MoonfallLootArt::Select(",self.hud)
        self.assertIn("MoonfallLootArt::ValidEquipment(",self.hud)
        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootRelicAtlas',self.hud_h)
        self.assertIn('DrawLootIcon(Slot, Rarity, Affix, Tier,',self.hud)
        self.assertIn('DrawLootIcon(SelectedSlot, SelectedRarity, SelectedTrait, SelectedTier,',self.hud)
        self.assertIn('Pip <= Rarity',self.hud)
        self.assertIn('T_MoonfallLootRelics.T_MoonfallLootRelics',self.hud)
        self.assertIn('T_MoonfallLootMasterworks.T_MoonfallLootMasterworks',self.hud)
        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootMasterworkAtlas',self.hud_h)
        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootAscendantAtlas',self.hud_h)
        self.assertIn('T_MoonfallLootAscendants.T_MoonfallLootAscendants',self.hud)
        self.assertIn('MoonfallLootArt::ValidCell(Cell)',self.hud)

        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootVariantAtlas',self.hud_h)
        self.assertIn('DrawLootIcon(int32 Slot, int32 Rarity, int32 Trait, int32 Tier,',self.hud_h)









        self.assertIn('SelectedCode == SelectedTrait * 100000 + SelectedSlot * 10000',self.hud)
        self.assertIn('EquipX - 49.f, EquipY - 7.f, 40.f',self.hud)
        # The source image is purely decorative and cannot add or mutate gear.
        self.assertNotIn('InventoryItemCodes.Add(',self.hud[:self.hud.index('void AMoonfallCombatHUD::DrawHUD()')])

    def test_import_path_matches_hud_references(self):
        importer=IMPORTER.read_text(encoding="utf-8")
        self.assertIn('Moonfall_LootAtlas.png',importer)
        self.assertIn('Moonfall_LootVariants_Atlas.png',importer)
        self.assertIn('Moonfall_LootRelics_Atlas.png',importer)
        self.assertIn('Moonfall_LootMasterworks_Atlas.png',importer)
        self.assertIn('Moonfall_LootAscendants_Atlas.png',importer)
        self.assertIn('T_MoonfallLootAscendants',importer)
        self.assertIn('T_MoonfallLootMasterworks',importer)
        self.assertIn('Existing Moonfall art asset cannot be loaded:',importer)
        self.assertIn('T_MoonfallLootRelics',importer)
        self.assertIn('zlib.crc32(',importer)
        self.assertIn('validate_atlas(SOURCE_DIR / filename',importer)
        self.assertIn('Non-texture asset occupies Moonfall art slot:',importer)
        self.assertIn('T_MoonfallLootVariants',importer)
        self.assertIn('bytes.fromhex("89504e470d0a1a0a")',importer)
        self.assertIn('struct.unpack(">IIBBBBB", chunk_data)',importer)
        self.assertIn('/Game/Moonfall/LootArt',importer)
        self.assertIn('T_MoonfallLootAtlas',importer)
        self.assertIn('unreal.AssetImportTask()',importer)
        self.assertIn('replace_existing',importer)
        self.assertIn('save_loaded_asset(asset)',importer)
        self.assertIn('/Game/Moonfall/LootArt/T_MoonfallLootAtlas.T_MoonfallLootAtlas',self.hud)
        self.assertIn('UPROPERTY() TObjectPtr<UTexture2D> LootIconAtlas',self.hud_h)
        self.assertIn('/Game/Moonfall/LootArt/T_MoonfallLootVariants.T_MoonfallLootVariants',self.hud)
        self.assertIn('IsValid(LootIconAtlas)',self.hud)
        self.assertIn('static_cast<float>(Cell.Column)/Cell.Columns',self.hud)
        self.assertIn('static_cast<float>(Cell.Row)/Cell.Rows',self.hud)
        self.assertIn('Canvas->K2_DrawTexture',self.hud)

    def test_art_is_safe_for_existing_item_codes(self):
        self.assertIn('DrawLootIcon(Slot, Rarity, Affix, Tier, Left + 17.f',self.hud)
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
        self.assertIn("const float SizeAccent",self.pickup)
        self.assertIn("const float BeaconWidth",self.pickup)
        self.assertIn("PickupDetailA->SetHiddenInGame(!IsValid(",self.pickup)
        self.assertIn("PickupDetailB->SetHiddenInGame(!IsValid(",self.pickup)
        self.assertIn("RarityBeacon->SetRelativeScale3D(",self.pickup)
        self.assertIn("PickupDetailA->SetMaterial(0, PickupMaterial)",self.pickup)
        self.assertIn("PickupDetailB->SetMaterial(0, PickupMaterial)",self.pickup)
        self.assertEqual(self.pickup.count('PickupMesh->CreateAndSetMaterialInstanceDynamic(0)'),1,
                         "Rarity geometry must share one material instance")
        self.assertNotIn("PickupDetailA->CreateAndSetMaterialInstanceDynamic",self.pickup)
        self.assertNotIn("PickupDetailB->CreateAndSetMaterialInstanceDynamic",self.pickup)
        self.assertEqual(self.pickup.count('PrimaryActorTick.bCanEverTick = false'),1)
        for property_name in ('PickupDetailA','PickupDetailB'):
            self.assertIn('UStaticMeshComponent* '+property_name,self.pickup_h)
            self.assertIn(property_name+'->SetCollisionEnabled(ECollisionEnabled::NoCollision)',self.pickup)
        self.assertNotIn('Z_character',self.hud)
        self.assertNotIn('Nyra_character',self.hud)


if __name__=="__main__":
    unittest.main()
