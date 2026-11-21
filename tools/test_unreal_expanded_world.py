"""Static/source regression tests for Moonfall's streamed UE5 outdoor map.

No Unreal Editor available in GitHub hosted source CI. These assertions check
precise geometry, generation limits, and integration contracts, NOT gameplay.
"""
from itertools import combinations
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
BUILD = ROOT / "MoonfallUnreal" / "Build"


def read(path):
    return path.read_text(encoding="utf-8")


class ExpandedOverworldTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.world_h = read(SOURCE / "MoonfallExpandedWorld.h")
        cls.world = read(SOURCE / "MoonfallExpandedWorld.cpp")
        cls.tile_h = read(SOURCE / "MoonfallOverworldTile.h")
        cls.tile = read(SOURCE / "MoonfallOverworldTile.cpp")
        cls.mode_h = read(SOURCE / "MoonfallUnrealGameMode.h")
        cls.mode = read(SOURCE / "MoonfallUnrealGameMode.cpp")
        cls.startup = read(BUILD / "Create-StartupLevel.py")
        cls.retrofit = read(BUILD / "Upgrade-ExpandedWorldNavigation.py")

    def test_six_full_size_provinces_plus_a_separate_island(self):
        self.assertIn("MainRegionCount = 6;", self.world_h)
        self.assertIn("RegionCount = 7;", self.world_h)
        self.assertIn("TilesPerSide = 5;", self.tile_h)
        self.assertIn("TileSize = 20000.f;", self.tile_h)
        self.assertIn("TilesPerRegion = 25;", self.world_h)
        # Region centers, in centimeters (Unreal's default world unit).
        center_section = self.world.split("static const FVector Centers[RegionCount] =", 1)[1].split("};", 1)[0]
        centers = re.findall(
            r"FVector\\((-?[0-9]+)\\.f,\\s*(-?[0-9]+)\\.f,\\s*0\\.f\\)", center_section
        )
        self.assertEqual(len(centers), 7)
        centers = [(int(x), int(y)) for x, y in centers]
        tile_side_cm = 5 * 20000
        for a, b in combinations(centers, 2):
            # Distinct world zones cannot overlap in either XY dimension.
            self.assertTrue(abs(a[0] - b[0]) >= tile_side_cm
                            or abs(a[1] - b[1]) >= tile_side_cm, (a, b))
        self.assertEqual(6 * (tile_side_cm / 100000) ** 2, 6.0)
        self.assertEqual(7 * (tile_side_cm / 100000) ** 2, 7.0)
        self.assertEqual(centers[6], (0, 302000))

    def test_all_regions_named_and_have_deterministic_blockout(self):
        for label in ("ASHEN WILDS", "MOONVEIL FOREST", "HOLLOW DEPTHS",
                      "IRON FOUNDRY", "FROSTBOUND REACH", "ECLIPSE WASTES",
                      "GOTHIC ISLE"):
            self.assertIn('TEXT("' + label + '")', self.world)
        self.assertIn("FRandomStream Random(3907 + RegionIndex * 911", self.tile)
        self.assertIn("constexpr int32 PropsPerTile = 18;", self.tile)
        self.assertIn("if (Column == 2 && Row == 2)", self.tile)
        self.assertIn("Buildings->AddInstance(", self.tile)
        self.assertIn("RegionSign->SetText(", self.tile)
        self.assertIn("if (FMath::Abs(X) < 1800.f || FMath::Abs(Y) < 1800.f) continue;", self.tile)

    def test_walkable_tile_ground_but_decorations_do_not_collide(self):
        self.assertIn("Terrain->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)", self.tile)
        self.assertIn("Terrain->SetCollisionResponseToAllChannels(ECR_Block)", self.tile)
        self.assertIn("Terrain->SetCanEverAffectNavigation(true)", self.tile)
        for component in ("Scenery", "Buildings", "RegionSign"):
            self.assertIn(component + "->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.tile)
        for component in ("Scenery", "Buildings"):
            self.assertIn(component + "->SetCanEverAffectNavigation(false)", self.tile)
        self.assertIn("FVector(TileSize / 100.f, TileSize / 100.f, 1.2f)", self.tile)
        self.assertIn("FVector(0.f, 0.f, -60.f)", self.tile)

    def test_streaming_is_time_sliced_lod_not_eager_spawn(self):
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", self.world)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", self.tile)
        self.assertNotIn("void AMoonfallExpandedWorld::Tick(", self.world)
        self.assertNotIn("void AMoonfallOverworldTile::Tick(", self.tile)
        self.assertIn("MaxResidentTiles = 25;", self.world_h)
        self.assertIn("MaxSpawnsPerRefresh = 3;", self.world_h)
        self.assertIn("LoadRadius = 46000.f;", self.world_h)
        self.assertIn("UnloadRadius = 60000.f;", self.world_h)
        self.assertIn("GetPlayerPawn(this, 0)", self.world)
        self.assertIn("ResidentTiles.Num() >= MaxResidentTiles", self.world)
        self.assertIn("Spawns >= MaxSpawnsPerRefresh", self.world)
        self.assertIn("Candidates.Sort(", self.world)
        self.assertIn("SpawnActorDeferred<AMoonfallOverworldTile>(", self.world)
        self.assertIn("Tile->FinishSpawning(TileTransform)", self.world)
        self.assertIn("Tile->RegionIndex = Region;", self.world)
        self.assertIn("ResidentTiles.Remove(Id);", self.world)
        self.assertIn("if (GetWorld()) GetWorldTimerManager().ClearTimer(StreamingTimer)", self.world)
        self.assertNotIn("SetLifeSpan(", self.world)

    def test_existing_arena_and_thirty_dungeons_remain_in_place(self):
        self.assertIn("bool bGenerateExpandedOverworld = true;", self.mode_h)
        self.assertIn("bool bGenerateDungeonWorld = true;", self.mode_h)
        self.assertIn("if (bGenerateExpandedOverworld)", self.mode)
        self.assertIn("TActorIterator<AMoonfallExpandedWorld>", self.mode)
        self.assertLess(self.mode.index("if (bGenerateExpandedOverworld)"),
                        self.mode.index("if (!bGenerateDungeonWorld) return;"))
        self.assertIn("SpawnDungeonBatch();", self.mode)
        self.assertIn("static constexpr int32 TotalDungeons = DungeonZoneCount * DungeonsPerZone;", self.mode_h)
        self.assertIn("static constexpr int32 DungeonZoneCount = 6;", self.mode_h)
        self.assertIn("static constexpr int32 DungeonsPerZone = 5;", self.mode_h)
        self.assertNotIn("MoonfallHeroPawn.cpp", self.world)
        self.assertNotIn("MoonfallEnemy.cpp", self.world)

    def test_all_six_outdoor_regions_have_walkable_connections(self):
        self.assertIn("Bridge the existing Shattered Sanctuary north gate", self.world)
        self.assertIn("FVector(0.f, 4400.f, 0.f)", self.world)
        self.assertIn("FVector(0.f, 15000.f, 0.f)", self.world)
        self.assertIn("const float RowCenters[2] = {62000.f, 182000.f};", self.world)
        self.assertIn("const float Columns[3] = {-120000.f, 0.f, 120000.f};", self.world)
        self.assertIn("for (const float X : Columns)", self.world)
        self.assertIn("ConnectingRoads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)", self.world)
        self.assertIn("ConnectingRoads->SetCanEverAffectNavigation(true)", self.world)
        # Do not quietly replace the planned gothic boat with a walkable sea bridge.
        self.assertIn("Do not fake a", self.world)
        self.assertIn("FVector(0.f, 238000.f, 0.f)", self.world)
        self.assertIn("FVector(0.f, 246000.f, 0.f)", self.world)
        self.assertNotIn("FVector(0.f, 238000.f, 0.f), FVector(0.f, 246000.f, 0.f)", self.world)

    def test_navigation_bounds_include_expanded_world_without_replacing_dungeon_bounds(self):
        self.assertIn('dungeon_nav.set_actor_label("Moonfall_DungeonWorld_NavBounds")', self.startup)
        self.assertIn('province_nav.set_actor_label("Moonfall_ExpandedWorld_NavBounds")', self.startup)
        self.assertIn("province_origin.y + province_extent.y < 352000.0", self.startup)
        self.assertIn("province_origin.x - province_extent.x > -170500.0", self.startup)
        self.assertIn("bGenerateNavigationOnlyAroundNavigationInvokers=True",
                      read(ROOT / "MoonfallUnreal/Config/DefaultEngine.ini"))
        self.assertIn("RuntimeGeneration=Dynamic",
                      read(ROOT / "MoonfallUnreal/Config/DefaultEngine.ini"))

    def test_existing_map_retrofit_is_backup_first_and_idempotent(self):
        compile(self.startup, "Create-StartupLevel.py", "exec")
        compile(self.retrofit, "Upgrade-ExpandedWorldNavigation.py", "exec")
        self.assertIn("if len(matches) > 1:", self.retrofit)
        self.assertIn("if matches:", self.retrofit)
        self.assertIn("if not sufficient_bounds(matches[0])", self.retrofit)
        self.assertIn("shutil.copy2(level_file, backup)", self.retrofit)
        self.assertIn("filecmp.cmp(level_file, backup, shallow=False)", self.retrofit)
        self.assertLess(self.retrofit.index("shutil.copy2(level_file, backup)"),
                        self.retrofit.index("nav = actors.spawn_actor_from_class("))
        self.assertIn("if not sufficient_bounds(nav):", self.retrofit)
        self.assertIn("if not unreal.EditorLoadingAndSavingUtils.save_current_level():", self.retrofit)
        self.assertNotIn("new_level(", self.retrofit)


if __name__ == "__main__":
    unittest.main()
