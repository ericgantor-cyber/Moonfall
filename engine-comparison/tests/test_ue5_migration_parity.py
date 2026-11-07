"""Safety and coverage gate for the planned Godot-to-Unreal migration.

Static verification only; does not imply an Unreal build, feature parity, or gameplay success.
"""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "engine-comparison/UE5_MIGRATION_PARITY.json"


class UnrealMigrationParityGate(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_source_projects_remain_separate(self):
        self.assertTrue((ROOT / self.data["unreal_project"]).is_file())
        self.assertTrue((ROOT / self.data["godot_reference"]).is_file())
        self.assertIn("Godot remains production", self.data["decision"])
        self.assertTrue(self.data["constraints"]["do_not_replace_godot_without_build"])

    def test_no_untested_feature_claimed_complete(self):
        features = self.data["features"]
        ids = [feature["id"] for feature in features]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertIn("windows_release", ids)
        for feature in features:
            with self.subTest(feature=feature["id"]):
                self.assertNotEqual(feature["status"], "verified")
                self.assertTrue(feature["gate"])
                for relative_path in feature["unreal"]:
                    self.assertTrue((ROOT / relative_path).is_file(), relative_path)

    def test_quest_tracker_uses_persistent_combat_and_loot_flags(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype"
        header = (base / "MoonfallPawn.h").read_text(encoding="utf-8")
        pawn = (base / "MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "MoonfallHud.cpp").read_text(encoding="utf-8")
        saves = (base / "MoonfallProgressSave.h").read_text(encoding="utf-8")
        for method in ("GetQuestDefeatedBeasts", "GetQuestClearedChambers"):
            self.assertIn(method + "() const;", header)
            self.assertIn("AMoonfallPawn::" + method + "() const", pawn)
            self.assertIn("Hero->" + method + "()", hud)
        self.assertIn("DefeatedEnemyMask & (1 << Id)", pawn)
        self.assertIn("ClaimedChamberMask & (1 << Id)", pawn)
        self.assertIn("DefeatedGuardianMask & (1 << Id)", pawn)
        for flag in ("DefeatedEnemyMask", "ClaimedChamberMask", "DefeatedGuardianMask"):
            self.assertIn(flag, saves)
            self.assertIn("Progress->" + flag + " = " + flag + ";", pawn)
        self.assertIn("WORLD QUESTS", hud)
        self.assertIn("Defeat Z", hud)
        self.assertIn("Hero->IsBossDefeated()", hud)

    def test_region_fog_does_not_reveal_unvisited_map_markers(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype"
        pawn = (base / "MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "MoonfallHud.cpp").read_text(encoding="utf-8")
        saves = (base / "MoonfallProgressSave.h").read_text(encoding="utf-8")
        self.assertIn("int32 DiscoveredRegionMask = 0;", header)
        self.assertIn("int32 DiscoveredRegionMask = 0;", saves)
        self.assertIn("Progress->DiscoveredRegionMask = DiscoveredRegionMask;", pawn)
        self.assertIn("DiscoveredRegionMask = Progress->DiscoveredRegionMask &", pawn)
        self.assertIn("DiscoveredRegionMask |= 1 << MoonfallWorldRules::RegionAt(", pawn)
        self.assertIn("const bool bNewRegion = (DiscoveredRegionMask & RegionBit) == 0;", pawn)
        self.assertIn("SaveProgress(); // Persist only when discoveries change", pawn)
        self.assertIn("bool AMoonfallPawn::IsMapPositionDiscovered(", pawn)
        self.assertIn("int32 AMoonfallPawn::GetDiscoveredRegionCount()", pawn)
        self.assertIn("Hero->IsMapPositionDiscovered(Chamber)", hud)
        self.assertIn("Hero->IsMapPositionDiscovered(Enemy)", hud)
        self.assertIn("Hero->IsMapPositionDiscovered(B)", hud)
        self.assertIn("DISCOVERED %d/6", hud)
        self.assertIn("TArray<uint8> DiscoveredMapCells;", header)
        self.assertIn("TArray<uint8> DiscoveredMapCells;", saves)
        self.assertIn("Progress->DiscoveredMapCells = DiscoveredMapCells;", pawn)
        self.assertIn("Progress->DiscoveredMapCells.Num() == 48 * 16", pawn)
        self.assertIn("DiscoveredMapCells.Init(0, 48 * 16)", pawn)
        self.assertIn("const bool bNewCells = RevealNearbyMapCells(Position);", pawn)
        self.assertIn("if (bNewRegion || bNewCells)", pawn)
        self.assertIn("if (Hero->IsMapPositionDiscovered(CellCenter))", hud)
        self.assertIn("X >= 0 && X < 48 && Y >= 0 && Y < 16", pawn)


    def test_waypoints_persist_without_revealing_fogged_quests(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        save = (base / "Source/MoonfallPrototype/MoonfallProgressSave.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        for action, key in (("SetWaypoint", "V"), ("ClearWaypoint", "B")):
            self.assertIn(f'ActionName="{action}",Key={key}', keys)
            self.assertIn(f'BindAction(TEXT("{action}")', pawn)
        for token in ("bool bHasWaypoint = false;", "FVector SavedWaypoint", "TArray<uint8> DiscoveredMapCells"):
            self.assertIn(token, save)
        self.assertIn("GetQuestDestination(FVector& OutDestination) const;", header)
        self.assertIn("GetWaypoint(FVector& OutWaypoint) const;", header)
        self.assertIn("Progress->SavedWaypoint = bHasWaypoint ? Waypoint : FVector::ZeroVector;", pawn)
        self.assertIn("Progress->bHasWaypoint = bHasWaypoint;", pawn)
        self.assertIn("CandidateWaypoint.X >= -1600.f", pawn)
        self.assertIn("CandidateWaypoint.Y <= 3200.f", pawn)
        self.assertIn("if (!IsMapPositionDiscovered(Candidate)) continue;", pawn)
        self.assertIn("!bWon && IsMapPositionDiscovered(GetBossMapPosition())", pawn)
        self.assertIn("if (Hero->GetQuestDestination(Destination))", hud)
        self.assertIn("if (Hero->GetWaypoint(MarkerPosition))", hud)
        self.assertIn("PIN: %.0f m", hud)
        self.assertIn("QUEST: %.0f m", hud)

    def test_full_map_click_never_attacks_or_reveals_fog(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="ToggleWorldMap",Key=M', keys)
        self.assertIn("ToggleWorldMap()", header)
        self.assertIn("PlaceMapWaypoint()", header)
        self.assertIn("if (bWorldMapOpen) { PlaceMapWaypoint(); return; }", pawn)
        self.assertIn("if (!IsMapPositionDiscovered(Candidate)) return;", pawn)
        self.assertIn("if (X < 0 || X >= 48 || Y < 0 || Y >= 16) return;", pawn)
        self.assertIn("FInputModeGameAndUI MapInput;", pawn)
        self.assertIn("MapInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);", pawn)
        self.assertIn("MapInput.SetHideCursorDuringCapture(false);", pawn)
        self.assertIn("PC->SetInputMode(MapInput);", pawn)
        self.assertIn("PC->SetInputMode(FInputModeGameOnly());", pawn)
        self.assertIn("PC->bShowMouseCursor = true;", pawn)
        self.assertIn("PC->bShowMouseCursor = false;", pawn)
        self.assertIn("bWorldMapOpen ? FVector::ZeroVector", pawn)
        self.assertIn("if (Hero->IsWorldMapOpen())", hud)
        self.assertIn("if (!Hero->IsMapPositionDiscovered(World)) continue;", hud)
        self.assertIn("Click explored terrain to set pin", hud)
        self.assertIn("if (!Hero->IsBossDefeated())", hud)

    def test_map_zoom_pan_shared_projection_and_clipping(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        for direction in ("MapPanHorizontal", "MapPanVertical"):
            self.assertIn('AxisName="' + direction + '"', keys)
            self.assertIn('BindAxis(TEXT("' + direction + '")', pawn)
        self.assertIn("float WorldMapZoom = 1.f;", header)
        self.assertIn("FVector2D WorldMapPanCells", header)
        self.assertIn("if (bWorldMapOpen)", pawn)
        self.assertIn("WorldMapZoom + Value * 0.2f, 1.f, 3.f", pawn)
        self.assertIn("24.f * (1.f - 1.f / WorldMapZoom)", pawn)
        self.assertIn("8.f * (1.f - 1.f / WorldMapZoom)", pawn)
        self.assertIn("GetWorldMapLayout(", header)
        self.assertIn("GetWorldMapLayout(", pawn)
        self.assertIn("Hero->GetWorldMapLayout(", hud)
        self.assertIn("MouseX < ClipMin.X", pawn)
        self.assertIn("MouseY >= ClipMax.Y", pawn)
        self.assertIn("VisibleMax.X <= VisibleMin.X", hud)
        self.assertIn("Screen.X >= ClipMin.X + 5.f", hud)
        self.assertIn("Wheel zoom", hud)
        self.assertIn("Arrows pan", hud)

    def test_escape_closes_atlas_and_navigation_state_resets(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="CloseWorldMap",Key=Escape', keys)
        self.assertIn("void CloseWorldMap();", header)
        self.assertIn('Input->BindAction(TEXT("CloseWorldMap"), IE_Pressed, this, &AMoonfallPawn::CloseWorldMap);', pawn)
        close_start = pawn.index("void AMoonfallPawn::CloseWorldMap()")
        close_body = pawn[close_start:pawn.index("\n}", close_start) + 2]
        self.assertIn("if (bWorldMapOpen) ToggleWorldMap();", close_body)
        toggle_start = pawn.index("void AMoonfallPawn::ToggleWorldMap()")
        toggle_body = pawn[toggle_start:pawn.index("\n}", toggle_start) + 2]
        self.assertIn("MapPanHorizontalInput = 0.f;", toggle_body)
        self.assertIn("MapPanVerticalInput = 0.f;", toggle_body)
        self.assertIn("Forward = 0.f;", toggle_body)
        self.assertIn("Right = 0.f;", toggle_body)

    def test_fog_reveal_inclusive_world_boundary(self):
        pawn = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        start = pawn.index("bool AMoonfallPawn::IsMapPositionDiscovered(")
        end = pawn.index("\n}", start)
        lookup = pawn[start:end]
        for expected in (
            "Position.X < -1600.f || Position.X > 17600.f",
            "Position.Y < -3200.f || Position.Y > 3200.f",
            "FMath::IsFinite(Position.X)",
            "FMath::IsFinite(Position.Y)",
            "FMath::Clamp(FMath::FloorToInt((Position.X + 1600.f) / 400.f), 0, 47)",
            "FMath::Clamp(FMath::FloorToInt((Position.Y + 3200.f) / 400.f), 0, 15)",
        ):
            self.assertIn(expected, lookup)
        self.assertIn("DiscoveredMapCells[Y * 48 + X] != 0", lookup)

    def test_open_map_blocks_combat_resource_inputs(self):
        pawn = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        for method in ("Dodge", "CastBolt", "CastHeal", "SetWaypoint"):
            with self.subTest(method=method):
                start = pawn.index("void AMoonfallPawn::" + method + "()")
                body = pawn[start:pawn.index("\n}", start) + 2]
                self.assertIn("if (bWorldMapOpen) return;", body)
        # Left click is map placement before any attack cooldown/damage logic.
        attack = pawn.split("void AMoonfallPawn::Attack()", 1)[1]
        self.assertLess(attack.index("if (bWorldMapOpen) { PlaceMapWaypoint(); return; }"),
                        attack.index("AttackCooldown ="))
        self.assertIn("bWorldMapOpen ? FVector::ZeroVector", pawn)

    def test_character_and_input_constraints(self):
        constraints = self.data["constraints"]
        self.assertEqual(constraints["preserve_original_art"],
                         ["Z_Original.webp", "Nyra_Original.png"])
        self.assertEqual(constraints["boss_roles"], ["Z", "Nyra"])
        self.assertTrue(constraints["click_only_attack"])
        self.assertTrue(constraints["continuous_world"])
        self.assertTrue(constraints["no_waves"])
        self.assertEqual(constraints["windows"], ["10", "11"])
        input_text = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', input_text)
        self.assertNotIn('ActionName="Attack",Key=SpaceBar', input_text)


if __name__ == "__main__":
    unittest.main()
