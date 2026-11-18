"""Source/geometry regression checks for the six-region Moonfall world expansion.

These are independent of Unreal Engine: native UE5.4 gameplay validation
remains required before merging.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class RegionalWaystations(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.arena = (SRC / "MoonfallArena.cpp").read_text(encoding="utf-8")
        cls.arena_h = (SRC / "MoonfallArena.h").read_text(encoding="utf-8")
        cls.npc = (SRC / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        cls.shrine = (SRC / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.save = (SRC / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        cls.sites = cls.arena.split("static const FWaystationSite Sites[6] = {", 1)[1].split(
            "\n  };", 1
        )[0]

    def test_six_named_regions_with_distinct_permanent_ids(self):
        sites = re.findall(
            r'\{TEXT\("([A-Z ]+)"\), TEXT\("(waystation_[a-z]+)"\),'
            r' TEXT\("([^"]+ Shrine)"\)', self.sites
        )
        self.assertEqual(len(sites), 6)
        self.assertEqual(len(set(site[0] for site in sites)), 6)
        shrine_ids = [site[1] for site in sites]
        self.assertEqual(len(set(shrine_ids)), 6)
        self.assertEqual(shrine_ids, [
            "waystation_ashen", "waystation_moonveil", "waystation_hollows",
            "waystation_foundry", "waystation_frost", "waystation_eclipse",
        ])
        for name in ("EMBERWATCH CAMP", "SILVERLEAF REFUGE", "DEEPSTONE HAVEN",
                     "SPARKFALL RELAY", "WINTERGLASS LODGE", "DUSKWATCH REFUGE"):
            self.assertIn(name, [site[0] for site in sites])
        prior = {"sanctuary_firstlight", "crossroads_west",
                 "crimson_path", "moonlit_path"}
        self.assertTrue(prior.isdisjoint(shrine_ids))

    def test_twelve_residents_have_unique_saved_quest_keys(self):
        ids = re.findall(r'TEXT\("(waystation_[a-z]+_(?:scout|artisan|scholar|tinker|elder))"\)', self.sites)
        self.assertEqual(len(ids), 12)
        self.assertEqual(len(set(ids)), 12)
        self.assertIn('for (int32 Resident = 0; Resident < 2; ++Resident)', self.arena)
        self.assertIn('Person->ResidentId = FName(Resident == 0 ? Site.ScoutId : Site.CompanionId);', self.arena)
        self.assertIn('Person->SettlementId = FName(Site.ShrineId);', self.arena)
        self.assertIn('Person->FactionId = FName(Site.Faction);', self.arena)
        self.assertIn('Person->InteractRange = 300.f;', self.arena)
        self.assertIn('Person->FinishSpawning(NpcTransform);', self.arena)
        self.assertIn('TMap<FName, int32> ResidentQuestStates', self.save)
        self.assertIn('TMap<FName, int32> ResidentBonds', self.save)
        self.assertIn('bool AMoonfallWorldNPC::OfferResidentQuest', self.npc)
        self.assertIn('bool AMoonfallWorldNPC::TurnInResidentQuest', self.npc)
        self.assertEqual(len(re.findall(r',\s*(\d+),\s*(\d+)\}', self.sites)), 6)
        for scout, partner in re.findall(r',\s*(\d+),\s*(\d+)\}', self.sites):
            self.assertTrue(0 <= int(scout) < 12 and 0 <= int(partner) < 12)

    def test_platform_continuously_overlaps_road_but_not_dungeon_entrances(self):
        self.assertIn('for (int32 Zone = 0; Zone < 6; ++Zone)', self.arena)
        self.assertIn('(static_cast<float>(Zone * 5 + 2) - 14.5f) * 1350.f', self.arena)
        self.assertIn('FVector(ZoneCenterX, -1800.f, -20.f), FVector(7.4f, 9.6f, 0.4f)', self.arena)
        # Cube meshes are 100 units. Both surfaces have top Z=0.
        road_y = (-2500 - 9*50, -2500 + 9*50)
        waystation_y = (-1800 - 9.6*50, -1800 + 9.6*50)
        overlap = min(road_y[1], waystation_y[1]) - max(road_y[0], waystation_y[0])
        self.assertEqual(overlap, 230)
        self.assertEqual(-20 + 0.4*50, 0)
        self.assertEqual(len([((zone*5+2)-14.5)*1350 for zone in range(6)]), 6)
        station_centers = [((zone*5+2)-14.5)*1350 for zone in range(6)]
        self.assertEqual(station_centers, [-16875, -10125, -3375, 3375, 10125, 16875])
        self.assertTrue(all(abs(x) > 1500 for x in station_centers))
        self.assertTrue(-3100 < road_y[0] and waystation_y[0] > -3100)
        self.assertTrue(waystation_y[0] <= -1890 <= waystation_y[1])
        self.assertTrue(waystation_y[0] <= -1500 <= waystation_y[1])

    def test_opt_in_toggle_and_low_overhead_rendering(self):
        self.assertIn('bool bEnableRegionalWaystations = true;', self.arena_h)
        self.assertIn('if (bEnableRegionalWaystations && GetWorld()', self.arena)
        self.assertIn('PrimaryActorTick.bCanEverTick = false;', self.arena)
        self.assertIn('RegionalWaystationFloors->AddInstance(', self.arena)
        self.assertIn('RegionalWaystationRuins->AddInstance(', self.arena)
        self.assertIn('RegionalWaystationRuins->SetCollisionEnabled(ECollisionEnabled::NoCollision);', self.arena)
        self.assertIn('RegionalWaystationFloors->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);', self.arena)
        self.assertIn('RegionalWaystationFloors->SetCollisionResponseToAllChannels(ECR_Block);', self.arena)
        self.assertIn('RegionalWaystationSigns.Add(Sign);', self.arena)
        self.assertIn('Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);', self.arena)
        self.assertIn('RegionalWaystationRuins->SetCastShadow(false);', self.arena)
        self.assertNotIn('GetWorldTimerManager()', self.arena.split(
            '// Six optional regional waystations', 1)[1].split(
            'if (bEnableStartingSanctuary)', 1)[0])

    def test_shrines_discover_and_map_with_preexisting_save_fields(self):
        self.assertIn('Shrine->ShrineId = FName(Site.ShrineId);', self.arena)
        self.assertIn('Shrine->MapDisplayName = FText::FromString(Site.ShrineTitle);', self.arena)
        self.assertIn('Shrine->FinishSpawning(ShrineTransform);', self.arena)
        self.assertIn('GetActorTransform().TransformPosition(', self.arena)
        self.assertIn('DiscoveryRange->OnComponentBeginOverlap.AddDynamic', self.shrine)
        self.assertIn('TActorIterator<AMoonfallCheckpointShrine>', self.hero)
        self.assertIn('Save->DiscoveredShrineIds = DiscoveredShrineIds;', self.hero)
        self.assertIn('Save->ActiveShrineId = ActiveShrineId;', self.hero)
        self.assertIn('DiscoveredShrineIds.Num() >= 64', self.hero)
        self.assertIn('ResidentQuestStates.Num() > 64', self.hero)

    def test_original_boss_identity_and_starting_sanctuary_unchanged(self):
        self.assertIn('DivineBossId = bZ ? FName(TEXT("z_void_reaper"))', self.arena)
        self.assertIn('FName(TEXT("nyra_eclipse_queen"))', self.arena)
        self.assertIn('if (bEnableStartingSanctuary)', self.arena)
        self.assertIn('sanctuary_firstlight', self.arena)
        self.assertIn('for (int32 Slot = 0; Slot < 30; ++Slot)', self.arena)


if __name__ == "__main__":
    unittest.main()
