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

    def test_gothic_isle_atlas_reader_reuses_card_and_hides_locked_pages(self):
        hero = read(SOURCE / "MoonfallHeroPawn.cpp")
        header = read(SOURCE / "MoonfallHeroPawn.h")
        hud = read(SOURCE / "MoonfallCombatHUD.cpp")
        isle = read(SOURCE / "MoonfallIsleChronicle.h")
        self.assertIn("bool bAtlasIsleReader = false;", header)
        self.assertIn("bAtlasIsleReader = !bAtlasIsleReader;", hero)
        self.assertIn("bAtlasIsleReader = false;", hero)
        self.assertIn("MoonfallIsleChronicle::NextReadableChapter(", hero)
        self.assertIn("MoonfallIsleChronicle::ChapterState(ClickedBeat,", hero)
        self.assertIn("== MoonfallIsleChronicle::EState::Hidden) return;", hero)
        self.assertIn("const bool bReadingIsle = Hero->bAtlasIsleReader", hud)
        self.assertIn("MoonfallIsleChronicle::RevealedChapter(ChapterIndex,", hud)
        self.assertIn("const FString Lore = bReadingIsle", hud)
        self.assertIn("IsleBeat ? FString(UTF8_TO_TCHAR(IsleBeat->Lore))", hud)
        self.assertIn("MoonfallChronicleUI::Layout(", hero)
        self.assertIn("MoonfallChronicleUI::Layout(", hud)
        self.assertIn("if (!HarborDiscovered && !CathedralDiscovered)", isle)
        self.assertIn("const bool Epilogue = CathedralDiscovered", isle)
        # No new persistence or direct unlocking when the reader opens.
        atlas = hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1]
        self.assertNotIn("DiscoveredDungeonMask =", atlas.split("if (bAtlasOpen)", 1)[1].split(
            "// All native modal panels", 1)[0])

    def test_atlas_fast_travel_requires_discovery_confirmation_and_safe_landing(self):
        hero = read(SOURCE / "MoonfallHeroPawn.cpp")
        header = read(SOURCE / "MoonfallHeroPawn.h")
        hud = read(SOURCE / "MoonfallCombatHUD.cpp")
        travel = hero.split("bool AMoonfallHeroPawn::TryAtlasFastTravel(", 1)[1].split(
            "bool AMoonfallHeroPawn::HasDiscoveredShrine(", 1)[0]
        self.assertIn("AtlasDiscoveredShrineIds", header)
        self.assertIn("AtlasFastTravelPendingShrine = NAME_None", header)
        self.assertIn("AtlasFastTravelConfirmUntil = 0.f", header)
        self.assertIn("HasDiscoveredShrine(ShrineId)", travel)
        self.assertIn("!AtlasDiscoveredShrineIds.Contains(ShrineId)", travel)
        self.assertIn("AtlasFastTravelPendingShrine != ShrineId", travel)
        self.assertIn("GetRealTimeSeconds()", travel)
        self.assertIn("Now > AtlasFastTravelConfirmUntil", travel)
        self.assertIn("AtlasFastTravelConfirmUntil = Now + 5.f;", travel)
        self.assertLess(travel.index("if (!Target) {"),
                        travel.index("AtlasFastTravelConfirmUntil = Now + 5.f;"))
        self.assertIn("AtlasFastTravelConfirmUntil = 0.f;", hero)
        self.assertIn("GetRealTimeSeconds() <= Hero->AtlasFastTravelConfirmUntil", hud)
        self.assertIn("It->ShrineId == ShrineId", travel)
        self.assertIn("Target->GetRespawnLocation()", travel)
        self.assertIn("OverlapBlockingTestByChannel", travel)
        self.assertIn("TeleportTo(Landing, GetActorRotation(), false, false)", travel)
        self.assertNotIn("ActiveShrineId =", travel)
        self.assertNotIn("DiscoveredShrineIds.Add(", travel)
        self.assertNotIn("SaveWorldProgress(", travel)
        self.assertIn("AtlasDiscoveredShrineIds.Add(Shrine->ShrineId);", hero)
        self.assertIn("TryAtlasFastTravel(AtlasDiscoveredShrineIds[Index]);", hero)
        self.assertIn("Click twice to fast travel", hud)
        self.assertIn("AtlasFastTravelPendingShrine = NAME_None;", hero)

    def test_context_controller_atlas_open_preserves_npc_and_ferry_actions(self):
        hero = read(SOURCE / "MoonfallHeroPawn.cpp")
        keys = read(ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini")
        self.assertIn('ActionName="ResidentContext",Key=Gamepad_DPad_Up', keys)
        self.assertIn('ActionName="Options",Key=Gamepad_Special_Right', keys)
        context = hero.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()", 1)[1].split(
            "void AMoonfallHeroPawn::CastRightMouse()", 1)[0]
        self.assertIn("if (bOptionsOpen || bInventoryOpen || bAtlasOpen)", context)
        self.assertIn("AMoonfallWorldNPC* Resident = GetNearestResidentNPC();", context)
        self.assertIn("Ferry->TryBoard(this)", context)
        self.assertIn("ToggleAtlas();", context)
        self.assertLess(context.index("GetNearestResidentNPC()"),
                        context.index("Ferry->TryBoard(this)"))
        self.assertLess(context.index("Ferry->TryBoard(this)"),
                        context.index("ToggleAtlas();"))
        self.assertIn("if (bAtlasOpen) { ToggleAtlas(); return; }", hero)
        self.assertNotIn("SaveWorldProgress(", context)
        self.assertNotIn("TeleportTo(", context)

    def test_controller_fast_travel_cycles_loaded_shrines_without_save_mutations(self):
        hero = read(SOURCE / "MoonfallHeroPawn.cpp")
        header = read(SOURCE / "MoonfallHeroPawn.h")
        hud = read(SOURCE / "MoonfallCombatHUD.cpp")
        keys = read(ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini")
        self.assertIn('ActionName="AtlasTravelSelect",Key=Gamepad_FaceButton_Left', keys)
        self.assertIn('ActionName="AtlasTravelSelect",Key=F', keys)
        self.assertIn('BindAction(TEXT("AtlasTravelSelect")', hero)
        self.assertIn("void AMoonfallHeroPawn::AtlasTravelSelect()", hero)
        self.assertIn("void AMoonfallHeroPawn::CycleAtlasTravel(int32 Direction)", hero)
        self.assertIn("void AtlasTravelSelect();", header)
        self.assertIn("void CycleAtlasTravel(int32 Direction);", header)
        select = hero.split("void AMoonfallHeroPawn::AtlasTravelSelect()", 1)[1].split(
            "void AMoonfallHeroPawn::CycleAtlasTravel(", 1)[0]
        cycling = hero.split("void AMoonfallHeroPawn::CycleAtlasTravel(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryAtlasFastTravel(", 1)[0]
        self.assertIn("TryAtlasFastTravel(AtlasFastTravelPendingShrine);", select)
        self.assertIn("CycleAtlasTravel(1);", select)
        self.assertIn("!HasDiscoveredShrine(Id)", cycling)
        self.assertIn("!It->IsActorBeingDestroyed()", cycling)
        self.assertIn("if (!bLoaded) continue;", cycling)
        self.assertIn("TryAtlasFastTravel(Id);", cycling)
        self.assertIn("if (bAtlasChronicleFocus) { StepAtlasChronicle(-1); return; }", hero)
        self.assertIn("CycleAtlasTravel(-1); return;", hero)
        self.assertIn("CycleAtlasTravel(1); return;", hero)
        self.assertIn("FAST TRAVEL:", hud)
        self.assertIn("SelectedTravelIndex", hud)
        self.assertNotIn("SaveWorldProgress(", cycling)
        self.assertNotIn("ActiveShrineId =", cycling)
        self.assertNotIn("TeleportTo(", cycling)

    def test_six_full_size_provinces_plus_a_separate_island(self):
        self.assertIn("MainRegionCount = 6;", self.world_h)
        self.assertIn("RegionCount = 7;", self.world_h)
        self.assertIn("TilesPerSide = 5;", self.tile_h)
        self.assertIn("TileSize = 20000.f;", self.tile_h)
        self.assertIn("TilesPerRegion = 25;", self.world_h)
        # Region centers, in centimeters (Unreal's default world unit).
        center_section = self.world.split("static const FVector Centers[RegionCount] =", 1)[1].split("};", 1)[0]
        centers = re.findall(
            r"FVector\(\s*(-?\d+)\.f,\s*(-?\d+)\.f,\s*0\.f\s*\)", center_section
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

    def test_gothic_isle_reuses_mainland_tiles_with_new_named_districts(self):
        # The island extends the same deterministic 25-tile streamer rather
        # than creating eager or always-ticking settlement actors.
        section = self.tile.split("if (RegionIndex == 6) {", 1)[1].split(
            "// Each province gets a recognizable town/landmark cluster", 1)[0]
        self.assertIn("constexpr int32 IslandOutpostProps = 6;", section)
        self.assertIn("Buildings->AddInstance(", section)
        self.assertIn("RegionSign->SetText(FText::FromString(Landmark));", section)
        for label in ("BLACKBELL CATHEDRAL", "RAVEN'S LANDING",
                      "ASHEN GRAVES", "DUSKWATCH KEEP", "THORNWRAITH GROVE"):
            self.assertIn(label, section)
        for marker in ("Column == 2 && Row == 2", "Column == 2 && Row == 0",
                       "Column == 4 && Row == 2", "Column == 0 && Row == 2",
                       "Column == 2 && Row == 4"):
            self.assertIn(marker, section)
        self.assertIn("if (RegionIndex != 6)", self.tile)
        self.assertIn("if (FMath::Abs(X) < 1800.f || FMath::Abs(Y) < 1800.f) continue;", self.tile)
        self.assertNotIn("SpawnActor<", section)
        self.assertNotIn("SetCollisionEnabled(", section)
        self.assertIn("MaxResidentTiles = 25;", self.world_h)

    def test_gothic_isle_uses_existing_persistent_shrines_and_npc_quests(self):
        island = self.world.split("void AMoonfallExpandedWorld::SpawnGothicIsleServices()", 1)[1].split(
            "void AMoonfallExpandedWorld::BeginPlay()", 1)[0]
        self.assertIn("SpawnGothicIsleServices();", self.world)
        self.assertIn('MoonfallCheckpointShrine.h', self.world)
        self.assertIn('MoonfallWorldNPC.h', self.world)
        for name in ("gothic_ravens_landing", "gothic_blackbell"):
            self.assertIn(name, island)
        for name in ("GothicRavenHarbormaster", "GothicBlackbellKeeper",
                     "GothicGraveWarden", "GothicDuskwatchSentinel",
                     "GothicThornwraithScholar"):
            self.assertIn(name, island)
        self.assertEqual(island.count("Shrine->ShrineId = StableId;"), 1)
        self.assertEqual(island.count("NPC->ResidentId = StableId;"), 1)
        self.assertIn("Shrine->CheckHeroAtSpawn(Hero);", island)
        self.assertIn("NPC->FactionId = FName(Spec.Faction);", island)
        self.assertIn("NPC->SettlementId = FName(TEXT(\"GothicIsle\"));", island)
        self.assertIn("NPC->SocialRole = FName(Spec.Role);", island)
        self.assertIn("NPC->RaceId = Spec.Race;", island)
        self.assertIn("NPC->CompletedStoryClue = FText::FromString(Spec.StoryClue);", island)
        for clue in ("harbor manifests name six cities",
                     "seventh name was erased",
                     "graves hold no bodies",
                     "wreckers copied a forged signal",
                     "roots shelter a star chart"):
            self.assertIn(clue, island)
        npc_h = read(SOURCE / "MoonfallWorldNPC.h")
        npc_cpp = read(SOURCE / "MoonfallWorldNPC.cpp")
        self.assertIn("FText CompletedStoryClue;", npc_h)
        self.assertIn('Hero->GetResidentQuestState(ResidentId) == 2', npc_cpp)
        self.assertIn('Reaction = CompletedStoryClue.IsEmpty()', npc_cpp)
        self.assertIn('LastSpokenLine = Reaction;', npc_cpp)
        self.assertEqual(island.count("TActorIterator<AMoonfallCheckpointShrine>"), 1)
        self.assertEqual(island.count("TActorIterator<AMoonfallWorldNPC>"), 1)
        self.assertIn("ExistingShrineIds.Contains(StableId)", island)
        self.assertIn("ExistingResidentIds.Contains(StableId)", island)
        self.assertIn("ExistingShrineIds.Add(StableId);", island)
        self.assertIn("ExistingResidentIds.Add(StableId);", island)
        self.assertLess(island.index("ExistingShrineIds.Contains(StableId)"),
                        island.index("SpawnActorDeferred<AMoonfallCheckpointShrine>"))
        self.assertLess(island.index("ExistingResidentIds.Contains(StableId)"),
                        island.index("SpawnActorDeferred<AMoonfallWorldNPC>"))
        self.assertEqual(island.count("ESpawnActorCollisionHandlingMethod::AlwaysSpawn"), 2)
        self.assertNotIn("PrimaryActorTick", island)
        self.assertNotIn("SetTimer(", island)
        self.assertNotIn("SaveGameToSlot", island)

    def test_isle_encounters_are_bounded_near_landmarks_and_not_instantly_respawned(self):
        combat = self.world.split("void AMoonfallExpandedWorld::UpdateGothicIsleEncounters(", 1)[1].split(
            "void AMoonfallExpandedWorld::BeginPlay()", 1)[0]
        for tag in ("GothicHarborReaver", "GothicBlackbellPenitent",
                    "GothicGraveSentinel", "GothicDuskwatchOutcast",
                    "GothicThornwraithStalker"):
            self.assertIn(tag, combat)
        self.assertIn("TMap<int32, TWeakObjectPtr<AMoonfallEnemy>> IslandEncounters;", self.world_h)
        self.assertIn("TSet<int32> ClearedIslandEncounters;", self.world_h)
        self.assertIn("DistanceSquared > FMath::Square(18000.f)", combat)
        self.assertIn("DistanceSquared > FMath::Square(11000.f)", combat)
        self.assertIn("ClearedIslandEncounters.Add(Index);", combat)
        self.assertIn("!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->Health <= 0.f", combat)
        # An invalid or pending-destroy actor must be recorded as cleared
        # before it leaves the live map, preventing same-visit respawn.
        stale = combat.split("if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed()", 1)[1].split("if (!bHeroReady", 1)[0]
        self.assertLess(stale.index("ClearedIslandEncounters.Add(Index);"),
                        stale.index("IslandEncounters.Remove(Index);"))
        self.assertIn("ClearedIslandEncounters.Remove(Index);", combat)
        self.assertIn("IslandEncounters.Contains(Index)", combat)
        self.assertIn("Enemy->Destroy();", combat)
        self.assertIn("Enemy->bBoss = Encounters[Index].bBoss;", combat)
        self.assertIn("GothicBlackbellWarden", combat)
        self.assertIn("FVector(12500.f, 12500.f, 90.f)", combat)
        self.assertIn("3, false, true}", combat)
        self.assertIn("Enemy->Tags.AddUnique(FName(Encounters[Index].Tag));", combat)
        self.assertNotIn('MoonfallDungeonBoss', combat)
        self.assertNotIn('z_void_reaper', combat)
        self.assertNotIn('nyra_eclipse_queen', combat)
        self.assertIn("Enemy->bElite = Encounters[Index].bElite;", combat)
        self.assertIn("ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding", combat)
        self.assertIn("const bool bHeroReady = IsValid(Hero) && FMath::IsFinite(Hero->Health)", combat)
        self.assertIn("&& Hero->Health > 0.f;", combat)
        self.assertIn("if (!bHeroReady || DistanceSquared > FMath::Square(11000.f)", combat)
        self.assertLess(combat.index("if (!bHeroReady ||"), combat.index("World->SpawnActorDeferred<AMoonfallEnemy>"))
        self.assertLess(combat.index("if (DistanceSquared > FMath::Square(18000.f))"),
                        combat.index("if (!bHeroReady ||"))
        self.assertIn("SiteColumn = FMath::FloorToInt(", combat)
        self.assertIn("SiteRow = FMath::FloorToInt(", combat)
        self.assertIn("AMoonfallOverworldTile::TileSize", combat)
        self.assertIn("const int32 SiteTileId = 6 * TilesPerRegion", combat)
        self.assertIn("ResidentTiles.Find(SiteTileId)", combat)
        self.assertIn("!IsValid(GroundTile->Get())", combat)
        self.assertIn("GroundTile->Get()->IsActorBeingDestroyed()", combat)
        self.assertLess(combat.index("ResidentTiles.Find(SiteTileId)"),
                        combat.index("World->SpawnActorDeferred<AMoonfallEnemy>"))
        self.assertLess(self.world.index("ResidentTiles.CreateIterator()"),
                        self.world.index("UpdateGothicIsleEncounters(PlayerPosition);"))
        self.assertIn("UpdateGothicIsleEncounters(PlayerPosition);", self.world)
        self.assertIn("IslandEncounters.Empty();", self.world)
        self.assertIn("TMap<int32, float> IslandSpawnRetryAt;", self.world_h)
        self.assertIn("IslandSpawnRetryAt.Find(Index)", combat)
        self.assertIn("if (Now < *RetryAt) continue;", combat)
        self.assertIn("IslandSpawnRetryAt.Add(Index, Now + 2.5f);", combat)
        self.assertIn("IslandSpawnRetryAt.Remove(Index);", combat)
        self.assertIn("IslandSpawnRetryAt.Empty();", self.world)
        self.assertLess(combat.index("if (Now < *RetryAt) continue;"),
                        combat.index("World->SpawnActorDeferred<AMoonfallEnemy>"))
        self.assertNotIn("SetTimer(", combat)
        self.assertNotIn("PrimaryActorTick", combat)

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
        streaming = self.world.split("void AMoonfallExpandedWorld::RefreshNearbyTiles()", 1)[1]
        cap_guard = "if (ResidentTiles.Num() >= MaxResidentTiles) return;"
        self.assertIn(cap_guard, streaming)
        # Capacity cannot short-circuit encounter cleanup or unloading.
        self.assertLess(streaming.index("It.RemoveCurrent();"),
                        streaming.index("UpdateGothicIsleEncounters(PlayerPosition);"))
        self.assertLess(streaming.index("UpdateGothicIsleEncounters(PlayerPosition);"),
                        streaming.index(cap_guard))
        self.assertIn("ResidentTiles.CreateIterator()", streaming)
        self.assertIn("It.RemoveCurrent();", streaming)
        self.assertIn("if (IsValid(Tile) && !Tile->IsActorBeingDestroyed()) Tile->Destroy();", streaming)
        self.assertIn("if (IsValid(Tile) && !Tile->IsActorBeingDestroyed()", streaming)
        self.assertLess(streaming.index("!Tile->IsActorBeingDestroyed()"),
                        streaming.index("It.RemoveCurrent();"))
        self.assertNotIn("TArray<int32> ExpiredIds;", streaming)
        self.assertLess(streaming.index("It.RemoveCurrent();"),
                        streaming.index(cap_guard))
        self.assertLess(streaming.index(cap_guard),
                        streaming.index("TArray<TPair<float, int32>> Candidates;"))
        self.assertLess(streaming.index(cap_guard), streaming.index("Candidates.Sort("))
        self.assertIn("const FVector Center = RegionCenter(Region);", streaming)
        self.assertIn("2.f * AMoonfallOverworldTile::TileSize", streaming)
        self.assertIn("const float CandidateReach = LoadRadius + MaxTileCenterOffset;", streaming)
        self.assertIn("FMath::Abs(PlayerPosition.X - Center.X) > CandidateReach", streaming)
        self.assertIn("FMath::Abs(PlayerPosition.Y - Center.Y) > CandidateReach", streaming)
        self.assertLess(streaming.index("const float CandidateReach"),
                        streaming.index("for (int32 Row = 0;"))
        # Grid-center bounds are conservative: any tile inside the circular
        # load radius must belong to a region that survives this axis filter.
        centers = [(-120000, 62000), (0, 62000), (120000, 62000),
                   (-120000, 182000), (0, 182000), (120000, 182000),
                   (0, 302000)]
        load_radius = 46000
        reach = load_radius + 2 * 20000
        for cx, cy in centers:
            for x in (-200000, -160000, -120000, -80000, -40000, 0,
                      40000, 80000, 120000, 160000, 200000):
                for y in (0, 40000, 80000, 120000, 160000, 200000,
                          240000, 280000, 320000, 360000):
                    nearest_square = min(
                        (x - (cx + dx)) ** 2 + (y - (cy + dy)) ** 2
                        for dx in (-40000, -20000, 0, 20000, 40000)
                        for dy in (-40000, -20000, 0, 20000, 40000))
                    if nearest_square <= load_radius ** 2:
                        self.assertLessEqual(abs(x - cx), reach)
                        self.assertLessEqual(abs(y - cy), reach)
        self.assertIn("SpawnActorDeferred<AMoonfallOverworldTile>(", self.world)
        self.assertIn("Tile->FinishSpawning(TileTransform)", self.world)
        self.assertIn("Tile->RegionIndex = Region;", self.world)
        self.assertIn("It.RemoveCurrent();", self.world)
        self.assertLess(streaming.index("ResidentTiles.CreateIterator()"),
                        streaming.index("TArray<TPair<float, int32>> Candidates;"))
        ending = self.world.split("void AMoonfallExpandedWorld::EndPlay(", 1)[1].split(
            "void AMoonfallExpandedWorld::UpdateSeaSwimming()", 1)[0]
        self.assertIn("for (const TPair<int32, TObjectPtr<AMoonfallOverworldTile>>& Entry : ResidentTiles)", ending)
        self.assertIn("AMoonfallOverworldTile* Tile = Entry.Value.Get();", ending)
        self.assertIn("if (IsValid(Tile) && !Tile->IsActorBeingDestroyed())", ending)
        self.assertIn("Tile->Destroy();", ending)
        self.assertIn("ResidentTiles.Empty();", ending)
        self.assertLess(ending.index("Tile->Destroy();"), ending.index("ResidentTiles.Empty();"))
        self.assertIn("GetWorldTimerManager().ClearTimer(StreamingTimer);", self.world)
        self.assertIn("GetWorldTimerManager().ClearTimer(SeaSwimmingTimer);", self.world)
        self.assertIn("if (GetWorld()) {", self.world)
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
        # The Nightferry crosses open water; no collision-enabled sea bridge.
        self.assertIn("no false walkable sea bridge", self.world)
        self.assertIn("SpawnActor<AMoonfallGothicFerry>", self.world)
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
