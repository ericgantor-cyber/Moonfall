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
        self.assertIn("Position.X < -1600.f || Position.X > 17600.f", pawn)
        self.assertIn("Position.Y < -3200.f || Position.Y > 3200.f", pawn)
        self.assertIn("FMath::Clamp(FMath::FloorToInt((Position.X + 1600.f) / 400.f), 0, 47)", pawn)
        self.assertIn("FMath::Clamp(FMath::FloorToInt((Position.Y + 3200.f) / 400.f), 0, 15)", pawn)


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
        self.assertIn("if (bWorldMapOpen)\n    {", pawn)
        self.assertIn("const float DodgeMovementDt = bDodgeActiveThisFrame", pawn)
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

    def test_full_screen_atlas_pauses_combat_and_resource_clocks(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        tick = pawn[pawn.index("void AMoonfallPawn::Tick(float DeltaSeconds)"): ]
        map_branch = tick[tick.index("if (bWorldMapOpen)"):tick.index("AttackCooldown = FMath::Max")]
        self.assertIn("WorldMapCursorCells.X = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapCursorCells.Y = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapPanCells.X = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapPanCells.Y = FMath::Clamp(", map_branch)
        self.assertIn("return;", map_branch)
        self.assertLess(map_branch.index("WorldMapPanCells.Y = FMath::Clamp("), map_branch.index("return;"))
        self.assertIn("UpdateWorldEnemies(Dt);", tick)
        self.assertIn("Mana = FMath::Min(180.f, Mana + 8.f * Dt);", tick)
        self.assertIn("combat paused", hud)
        self.assertIn("M / Esc close", hud)

    def test_combat_and_death_screen_state_are_mutually_exclusive(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertIn("bool bRespawnInProgress = false;", header)
        toggle = pawn[pawn.index("void AMoonfallPawn::ToggleWorldMap()"):pawn.index("void AMoonfallPawn::CloseWorldMap()")]
        self.assertIn("if (Health <= 0.f || bRespawnInProgress) return;", toggle)
        retry = pawn[pawn.index("void AMoonfallPawn::RespawnFromCheckpoint()"):pawn.index("void AMoonfallPawn::Parry()")]
        self.assertIn("bWorldMapOpen || bRespawnInProgress || !GetWorld()", retry)
        self.assertIn("bRespawnInProgress = true;", retry)
        self.assertLess(retry.index("bRespawnInProgress = true;"), retry.index("UGameplayStatics::OpenLevel"))
        self.assertNotIn("SaveProgress();", retry)
        self.assertIn("if (Health <= 0.f || ParryTimer > 0.f || AttackCooldown > 0.f)", pawn)
        self.assertIn("if (Health <= 0.f || ParryTimer > 0.f || BoltCooldown > 0.f", pawn)
        self.assertIn("if (Health <= 0.f || ParryTimer > 0.f || HealCooldown > 0.f", pawn)
        start = hud.index("if (Hero->GetHudHealth() <= 0.f)")
        end = hud.index("if (Hero->IsWorldMapOpen())", start)
        death_hud = hud[start:end]
        self.assertIn('DrawText(TEXT("RESPAWN")', death_hud)
        self.assertIn("return;", death_hud)

    def test_defeat_blocks_waypoint_save_and_camera_zoom(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        zoom = pawn[pawn.index("void AMoonfallPawn::Zoom(float Value)"):
                    pawn.index("void AMoonfallPawn::GetRespawnButtonBounds(", pawn.index("void AMoonfallPawn::Zoom(float Value)"))]
        clear = pawn[pawn.index("void AMoonfallPawn::ClearWaypoint()"):
                     pawn.index("void AMoonfallPawn::MoveForward(float Value)")]
        pad = pawn[pawn.index("void AMoonfallPawn::ControllerSetWaypoint()"):
                   pawn.index("void AMoonfallPawn::SetWaypoint()")]
        self.assertIn("if (Health <= 0.f || bRespawnInProgress) return;", zoom)
        self.assertLess(zoom.index("if (Health <= 0.f || bRespawnInProgress) return;"),
                        zoom.index("ZoomDistance ="))
        self.assertIn("if (Health <= 0.f || bRespawnInProgress || !bHasWaypoint) return;", clear)
        self.assertLess(clear.index("if (Health <= 0.f"), clear.index("SaveProgress();"))
        self.assertIn("if (Health <= 0.f || bRespawnInProgress) return;", pad)
        self.assertIn("RespawnFromCheckpoint();", pawn)

    def test_gamepad_trigger_cannot_activate_mouse_map_or_death_clicks(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        config = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn('ActionName="ControllerAttack",Key=Gamepad_RightTrigger', config)
        self.assertNotIn('ActionName="Attack",Key=Gamepad_RightTrigger', config)
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', config)
        self.assertIn("void ControllerAttack();", header)
        self.assertIn('Input->BindAction(TEXT("ControllerAttack"), IE_Pressed', cpp)
        start = cpp.index("void AMoonfallPawn::ControllerAttack()")
        end = cpp.index("void AMoonfallPawn::ControllerConfirm()", start)
        controller = cpp[start:end]
        self.assertIn("if (Health <= 0.f || bWorldMapOpen || bRespawnInProgress) return;", controller)
        self.assertIn("Attack();", controller)
        self.assertNotIn("ClickRespawnButton();", controller)
        self.assertNotIn("PlaceMapWaypoint();", controller)

    def test_death_closes_atlas_and_stops_combat_before_resource_tick(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        death = tick[:tick.index("    if (bWorldMapOpen)")]
        self.assertIn("if (Health <= 0.f && !bDeathInputEnabled)", death)
        self.assertIn("bWorldMapOpen = false;", death)
        self.assertIn("PC->SetInputMode(DeathInput);", death)
        self.assertIn("PC->bShowMouseCursor = true;", death)
        self.assertNotIn("ToggleWorldMap();", death)
        self.assertIn("if (Health <= 0.f)\n    {", death)
        self.assertIn("bCasting = false;", death)
        self.assertIn("CastTimer = 0.f;", death)
        self.assertIn("if (WarningMesh) WarningMesh->SetVisibility(false);", death)
        death_cleanup = death[death.index("if (Health <= 0.f)\n    {"):]
        self.assertIn("return;", death_cleanup)
        self.assertLess(tick.index("if (WarningMesh) WarningMesh->SetVisibility(false);"),
                        tick.index("Mana = FMath::Min(180.f, Mana + 8.f * Dt);"))
        self.assertIn("if (Health <= 0.f || bRespawnInProgress) return;", cpp)

    def test_controller_overhaul_combat_map_respawn_and_mouse_parity(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        inputs = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        for key in ('Gamepad_LeftX', 'Gamepad_LeftY', 'Gamepad_RightX',
                    'Gamepad_RightY', 'Gamepad_RightTrigger', 'Gamepad_LeftTrigger',
                    'Gamepad_FaceButton_Bottom', 'Gamepad_FaceButton_Right',
                    'Gamepad_FaceButton_Left', 'Gamepad_FaceButton_Top',
                    'Gamepad_Special_Right', 'Gamepad_DPad_Up', 'Gamepad_DPad_Down'):
            self.assertIn('Key=' + key, inputs)
        for original in ('Key=LeftMouseButton', 'Key=RightMouseButton',
                         'Key=MouseScrollUp', 'Key=MouseScrollDown', 'Key=W', 'Key=S'):
            self.assertIn(original, inputs)
        self.assertNotIn('ActionName="RespawnFromCheckpoint"', inputs)
        self.assertNotIn('ActionName="RetryFromCheckpoint"', inputs)
        for action in ('ControllerConfirm', 'ControllerBack', 'ControllerZoomIn',
                       'ControllerZoomOut', 'ControllerSetWaypoint'):
            self.assertIn('void ' + action + '();', header)
            self.assertIn('Input->BindAction(TEXT("' + action + '"), IE_Pressed', pawn)
        confirm = pawn[pawn.index("void AMoonfallPawn::ControllerConfirm()"):
                       pawn.index("void AMoonfallPawn::ControllerBack()")]
        self.assertIn("if (Health <= 0.f) { RespawnFromCheckpoint(); return; }", confirm)
        self.assertIn("if (bWorldMapOpen) { SetMapCenterWaypoint(); return; }", confirm)
        self.assertIn("Attack();", confirm)
        back = pawn[pawn.index("void AMoonfallPawn::ControllerBack()"):
                    pawn.index("void AMoonfallPawn::ControllerZoomIn()")]
        self.assertIn("if (bWorldMapOpen) { CloseWorldMap(); return; }", back)
        self.assertIn("Dodge();", back)
        center = pawn[pawn.index("void AMoonfallPawn::SetMapCenterWaypoint()"):
                      pawn.index("void AMoonfallPawn::ControllerConfirm()")]
        self.assertIn("WorldMapCursorCells.X", center)
        self.assertIn("WorldMapCursorCells.Y", center)
        self.assertIn("if (!IsMapPositionDiscovered(Candidate)) return;", center)
        self.assertIn("SaveProgress();", center)
        self.assertIn("Gamepad_LeftX", inputs)
        self.assertIn("DeadZone=0.24", inputs)
        self.assertIn("Click or press gamepad A to respawn", hud)
        self.assertIn("const FVector2D Reticle =", hud)
        self.assertIn("Pad: right stick select tile, A explored pin, B close", hud)

    def test_timed_parry_has_cost_cooldown_and_one_hit_consumption(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="Parry",Key=RightMouseButton', keys)
        self.assertIn("void Parry();", header)
        self.assertIn("float ParryTimer = 0.f, ParryCooldown = 0.f;", header)
        self.assertIn('Input->BindAction(TEXT("Parry"), IE_Pressed, this, &AMoonfallPawn::Parry);', pawn)
        start = pawn.index("void AMoonfallPawn::Parry()")
        body = pawn[start:pawn.index("\n}", start) + 2]
        for required in ("if (bWorldMapOpen || Health <= 0.f || DodgeTimer > 0.f",
                         "ParryCooldown > 0.f || AttackCooldown > 0.f || Stamina < 25.f",
                         "Stamina -= 25.f;", "ParryTimer = 0.22f;",
                         "ParryCooldown = 1.25f;"):
            self.assertIn(required, body)
        tick = pawn[pawn.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("bool bParryActiveThisFrame = ParryTimer > 0.f;", tick)
        self.assertIn("ParryCooldown = FMath::Max(0.f, ParryCooldown - Dt);", tick)
        self.assertIn("Enemies[ContactIndex].StaggerTimer = 0.8f;", tick)
        self.assertIn("CastCooldown = FMath::Max(CastCooldown, 2.f);", tick)
        self.assertEqual(tick.count("bParryActiveThisFrame = false;"), 2)
        self.assertLess(tick.index("if (bWorldMapOpen)"), tick.index("ParryCooldown ="))

    def test_respawn_button_responsive_geometry_and_hover(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        bounds_start = pawn.index("void AMoonfallPawn::GetRespawnButtonBounds(")
        bounds = pawn[bounds_start:pawn.index("\n}", bounds_start) + 2]
        self.assertIn("FMath::Min(360.f, Width - 32.f)", bounds)
        self.assertIn("FMath::Min(84.f, Height * 0.16f)", bounds)
        self.assertIn("OutMin = Center -", bounds)
        self.assertIn("OutMax = Center +", bounds)
        self.assertIn("PlayerOwner->GetMousePosition(MouseX, MouseY)", hud)
        self.assertIn("MouseX >= ButtonMin.X && MouseX <= ButtonMax.X", hud)
        self.assertIn("MouseY >= ButtonMin.Y && MouseY <= ButtonMax.Y", hud)
        self.assertIn("bHovered ? 5.f : 3.f", hud)
        self.assertIn("Hero->GetRespawnButtonBounds(Canvas->ClipX, Canvas->ClipY", hud)
        self.assertIn("constexpr int32 RuneSegments = 40;", hud)
        self.assertIn("Canvas->K2_DrawLine(RingA, RingB, 2.f", hud)
        self.assertIn("Canvas->K2_DrawLine(Inner, RingA, 2.f", hud)
        self.assertIn("FMath::Clamp(Canvas->ClipY * .105f, 24.f, 70.f)", hud)
        self.assertIn("Click or press gamepad A to respawn at your checkpoint", hud)
        self.assertNotIn('DrawText(TEXT("RETRY")', hud)

    def test_respawn_grace_hud_countdown_only_while_protected(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertIn("float GetRespawnProtectionSeconds() const", header)
        self.assertIn("FMath::Max(0.f, RespawnGraceTimer)", header)
        self.assertIn("Hero->GetRespawnProtectionSeconds()", hud)
        self.assertIn("Hero->GetHudHealth() > 0.f && ProtectionSeconds > 0.f", hud)
        self.assertIn('TEXT("PROTECTED %.1fs")', hud)
        self.assertLess(hud.index('TEXT("PROTECTED %.1fs")'),
                        hud.index("if (Hero->GetHudHealth() <= 0.f)"))
        self.assertIn("FMath::Max(12.f, OrbCenter.X - 55.f)", hud)

    def test_invalid_checkpoint_is_repaired_without_resetting_world_progress(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool bCheckpointNeedsRepair = false;", header)
        begin = pawn[pawn.index("void AMoonfallPawn::BeginPlay()"):
                     pawn.index("UBillboardComponent* AMoonfallPawn::AddWorldArtwork(")]
        self.assertIn("bCheckpointNeedsRepair = Progress->bHasCheckpoint && !bLoadedCheckpoint;", begin)
        self.assertIn("if (bCheckpointNeedsRepair)", begin)
        self.assertLess(begin.index("BuildWorld();"), begin.index("if (bCheckpointNeedsRepair)"))
        self.assertIn("SaveProgress();", begin[begin.index("if (bCheckpointNeedsRepair)"):])
        self.assertIn("DefeatedEnemyMask = Progress->DefeatedEnemyMask", begin)
        self.assertIn("ClaimedChamberMask = Progress->ClaimedChamberMask", begin)
        save = pawn[pawn.index("void AMoonfallPawn::SaveProgress()"):
                    pawn.index("bool AMoonfallPawn::GetWaypoint(")]
        self.assertIn("bool bWroteCheckpoint = false;", save)
        self.assertIn("bCheckpointNeedsRepair || IsSafeCheckpointLocation()", save)
        self.assertIn("bWroteCheckpoint = true;", save)
        self.assertIn("else if (bWroteCheckpoint)\n        bCheckpointNeedsRepair = false;", save)
        self.assertLess(save.index("SaveGameToSlot("),
                        save.index("bCheckpointNeedsRepair = false;"))

    def test_checkpoints_avoid_respawn_grace_or_recent_damage(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        safe = cpp[cpp.index("bool AMoonfallPawn::IsSafeCheckpointLocation() const"):
                   cpp.index("void AMoonfallPawn::SaveProgress()")]
        self.assertIn("RespawnGraceTimer > 0.f", safe)
        self.assertIn("EnemyDamageCooldown > 0.f", safe)
        self.assertNotIn("Health < GetHudMaxHealth() * 0.25f", safe)
        self.assertIn("Health <= 0.f", safe)
        self.assertIn("bRespawnInProgress || bCasting", safe)
        save = cpp[cpp.index("void AMoonfallPawn::SaveProgress()"):
                   cpp.index("bool AMoonfallPawn::GetWaypoint(")]
        self.assertIn("Progress->HeroXP = Progression.Experience;", save)
        self.assertIn("Progress->DefeatedEnemyMask = DefeatedEnemyMask;", save)
        self.assertIn("(!Progress->bHasCheckpoint || bCheckpointNeedsRepair || IsSafeCheckpointLocation())", save)
        self.assertIn("Progress->SavedHealth = Health;", save)
        self.assertIn("bWroteCheckpoint = true;", save)

    def test_progress_saves_do_not_move_checkpoint_into_live_combat(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool IsSafeCheckpointLocation() const;", header)
        safe = cpp[cpp.index("bool AMoonfallPawn::IsSafeCheckpointLocation() const"):
                   cpp.index("void AMoonfallPawn::SaveProgress()")]
        self.assertIn("Health <= 0.f || bRespawnInProgress || bCasting", safe)
        self.assertIn("FMath::Square(1500.f)", safe)
        self.assertIn("FMath::Square(750.f)", safe)
        self.assertIn("FVector::DistSquared2D", safe)
        save = cpp[cpp.index("void AMoonfallPawn::SaveProgress()"):
                   cpp.index("bool AMoonfallPawn::GetWaypoint(")]
        self.assertIn("Progress->DefeatedEnemyMask = DefeatedEnemyMask;", save)
        self.assertIn("Progress->ClaimedChamberMask = ClaimedChamberMask;", save)
        self.assertIn("(!Progress->bHasCheckpoint || bCheckpointNeedsRepair || IsSafeCheckpointLocation())", save)
        self.assertLess(save.index("Progress->DefeatedEnemyMask ="),
                        save.index("IsSafeCheckpointLocation()"))
        self.assertIn("Progress->SavedHeroPosition = HeroMesh->GetRelativeLocation();", save)
        self.assertIn("Progress->SavedHealth = Health;", save)
        self.assertNotIn("Progress->bHasCheckpoint = false;", save)

    def test_early_dodge_button_press_is_buffered_once_without_extra_stamina(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("float DodgeBufferTimer = 0.f;", header)
        self.assertIn("FVector BufferedDodgeDirection = FVector::ZeroVector;", header)
        dodge = cpp[cpp.index("void AMoonfallPawn::Dodge()"):
                    cpp.index("int32 AMoonfallPawn::GuardianBlessingCount()")]
        self.assertIn("else if (DodgeCooldown <= 0.18f)", dodge)
        self.assertIn("BufferedDodgeDirection = FVector(Forward, Right, 0.f).GetSafeNormal();", dodge)
        self.assertIn("DodgeBufferTimer = 0.2f;", dodge)
        self.assertEqual(dodge.count("Stamina -= 45.f;"), 1)
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("DodgeBufferTimer = FMath::Max(0.f, DodgeBufferTimer - Dt);", tick)
        self.assertIn("bBufferValidThisFrame && DodgeCooldown <= 0.f &&", tick)
        self.assertLess(tick.index("DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);"),
                        tick.index("DodgeDirection = BufferedDodgeDirection;"))
        self.assertLess(tick.index("DodgeDirection = BufferedDodgeDirection;"),
                        tick.index("const float DodgeTimeRemainingThisFrame = DodgeTimer;"))
        self.assertLess(tick.index("const bool bDodgeActiveThisFrame = DodgeTimeRemainingThisFrame > 0.f;"),
                        tick.index("const float DodgeMovementDt = bDodgeActiveThisFrame"))
        self.assertIn("DodgeTimer <= 0.f && ParryTimer <= 0.f && Stamina >= 45.f", tick)
        self.assertIn("DodgeDirection = BufferedDodgeDirection;", tick)
        self.assertLess(tick.index("DodgeDirection = BufferedDodgeDirection;"),
                        tick.index("Stamina -= 45.f;"))
        self.assertIn("DodgeBufferTimer = 0.f;", tick)
        toggle = cpp[cpp.index("void AMoonfallPawn::ToggleWorldMap()"):
                     cpp.index("void AMoonfallPawn::CloseWorldMap()")]
        self.assertIn("DodgeBufferTimer = 0.f;", toggle)
        self.assertLess(tick.index("if (bWorldMapOpen)"), tick.index("if (DodgeBufferTimer > 0.f)"))
        self.assertIn("DodgeBufferTimer = 0.f;", tick[:tick.index("if (bWorldMapOpen)")])

    def test_dodge_direction_remains_committed_after_input_release(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("FVector DodgeDirection = FVector::ZeroVector;", header)
        dodge = cpp[cpp.index("void AMoonfallPawn::Dodge()"):
                    cpp.index("int32 AMoonfallPawn::GuardianBlessingCount()")]
        self.assertIn("FVector2D(Forward, Right).SizeSquared() <= 0.01f", dodge)
        self.assertIn("DodgeDirection = FVector(Forward, Right, 0.f).GetSafeNormal();", dodge)
        self.assertNotIn("DodgeDirection = FVector(Forward, Right, 0.f).GetClampedToMaxSize(1.f);", dodge)
        self.assertLess(dodge.index("DodgeDirection ="), dodge.index("DodgeTimer = 0.32f;"))
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("const float DodgeTimeRemainingThisFrame = DodgeTimer;", tick)
        self.assertIn("const bool bDodgeActiveThisFrame = DodgeTimeRemainingThisFrame > 0.f;", tick)
        self.assertIn("const float DodgeMovementDt = bDodgeActiveThisFrame", tick)
        self.assertIn("FMath::Min(MovementDt, DodgeTimeRemainingThisFrame) : 0.f;", tick)
        self.assertIn("const float WalkMovementDt = MovementDt - DodgeMovementDt;", tick)
        self.assertIn("DodgeDirection * 1200.f * DodgeMovementDt", tick)
        self.assertIn("WalkDirection * 600.f * WalkMovementDt", tick)
        self.assertLess(tick.index("const float DodgeTimeRemainingThisFrame = DodgeTimer;"),
                        tick.index("DodgeTimer = FMath::Max(0.f, DodgeTimer - ReactionDt);"))

    def test_enemy_melee_requires_visible_windup_and_one_shot_recovery(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        for field in ("float AttackWindup = 0.f;", "float AttackCooldown = 0.f;",
                      "bool bAttackReadyThisFrame = false;"):
            self.assertIn(field, header)
        self.assertIn("float GetNearestEnemyWindupSeconds() const;", header)
        ai = cpp[cpp.index("void AMoonfallPawn::UpdateWorldEnemies(float Dt)"):
                 cpp.index("float AMoonfallPawn::GetNearestEnemyWindupSeconds()")]
        self.assertIn("Enemy.bAttackReadyThisFrame = false;", ai)
        self.assertIn("Enemy.AttackCooldown = FMath::Max(0.f, Enemy.AttackCooldown - MoveDt);", ai)
        self.assertIn("Enemy.AttackWindup = FMath::Max(0.f, Enemy.AttackWindup - MoveDt);", ai)
        self.assertIn("Enemy.bAttackReadyThisFrame = true;", ai)
        self.assertIn("Enemy.AttackCooldown = Enemy.bGuardian ? 2.f : 1.5f;", ai)
        self.assertIn("Enemy.AttackWindup = Enemy.bGuardian ?", ai)
        self.assertIn("Enemy.AttackWindup = 0.f;", ai)
        self.assertIn("HeroDistSq <= FMath::Square(240.f)", ai)
        self.assertIn("const bool bAggro", ai)
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertLess(tick.index("UpdateWorldEnemies(Dt);"),
                        tick.index("if (!Candidate.bAttackReadyThisFrame) continue;"))
        self.assertIn("if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;", tick)
        self.assertIn("else if (bParryActiveThisFrame)", tick)
        self.assertIn("else if (!bRespawnProtectedThisFrame)", tick)
        damage = cpp[cpp.index("void AMoonfallPawn::DamageWorldEnemy("):
                     cpp.index("TArray<FVector> AMoonfallPawn::GetChamberMapPositions()")]
        self.assertIn("Enemy.AttackWindup = 0.f;", damage)
        self.assertIn("Enemy.AttackCooldown = FMath::Max(Enemy.AttackCooldown, 0.5f);", damage)
        self.assertNotIn("GetNearestEnemyWindupSeconds()", hud)
        self.assertNotIn("DODGE / PARRY", hud)
        self.assertNotIn("INCOMING %.1fs", hud)
        self.assertNotIn("SpawnWave(", ai)

    def test_three_world_enemy_tactics_remain_leashed_and_wave_free(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        ai = cpp[cpp.index("void AMoonfallPawn::UpdateWorldEnemies(float Dt)"):
                 cpp.index("int32 AMoonfallPawn::NearestEnemyIndex(")]
        self.assertIn("const int32 Tactic = Enemy.bGuardian ? 0 : Enemy.Id % 3;", ai)
        self.assertIn("if (Tactic == 1)", ai)
        self.assertIn("Tangent(-MoveDirection.Y, MoveDirection.X, 0.f)", ai)
        self.assertIn("Tactic == 2 && HeroDistance < 620.f", ai)
        self.assertIn("ToCandidateHome.GetClampedToMaxSize2D(Leash)", ai)
        self.assertIn("const float MoveDt = FMath::Clamp(Dt, 0.f, 0.05f);", ai)
        self.assertNotIn("SpawnWave(", ai)

    def test_all_six_forged_regions_offer_bounded_and_distinct_perks(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        melee = cpp[cpp.index("void AMoonfallPawn::Attack()"):
                    cpp.index("void AMoonfallPawn::CastBolt()")]
        bolt = cpp[cpp.index("void AMoonfallPawn::CastBolt()"):
                   cpp.index("void AMoonfallPawn::CastHeal()")]
        enemy_damage = cpp[cpp.index("void AMoonfallPawn::DamageWorldEnemy("):
                           cpp.index("TArray<FVector> AMoonfallPawn::GetChamberMapPositions()")]
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("Stamina + 2.f * ForgedRegionTier(0)", melee)
        self.assertIn("4.f * ForgedRegionTier(3)", cpp)
        self.assertIn("0.08f * ForgedRegionTier(5)", bolt)
        self.assertIn("const float BoltManaCost = FMath::Max(22.f, 30.f - 2.f * ForgedRegionTier(1));", bolt)
        self.assertIn("Mana < BoltManaCost", bolt)
        self.assertIn("Mana -= BoltManaCost;", bolt)
        self.assertLess(bolt.index("if (EnemyIndex == INDEX_NONE && !bBossInRange) return;"),
                        bolt.index("Mana -= BoltManaCost;"))
        self.assertIn("FMath::Min(0.8f, 0.4f + 0.1f * ForgedRegionTier(2))", enemy_damage)
        self.assertIn("Enemy.bGuardian ? 0.8f :", enemy_damage)
        self.assertIn("ContactDamage - 0.75f * ForgedRegionTier(4)", tick)
        self.assertIn("BossBaseDamage - 0.75f * ForgedRegionTier(4)", tick)
        self.assertIn("FMath::Max(1.f,", tick)
        self.assertIn("bDodgeActiveThisFrame", tick)
        self.assertIn("bParryActiveThisFrame", tick)
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertNotIn("INCOMING %.1fs", hud)
        self.assertNotIn("DODGE / PARRY", hud)

    def test_forging_prefers_local_region_and_gear_has_distinct_traits(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        forge = cpp[cpp.index("void AMoonfallPawn::ForgeEquipment()"):
                    cpp.index("float AMoonfallPawn::BasicAttackDamage()")]
        self.assertIn("MoonfallWorldRules::RegionAt(HeroMesh->GetRelativeLocation().X)", forge)
        self.assertIn("(LocalRegion + Offset) % MoonfallWorldRules::RegionCount", forge)
        self.assertIn("if (CurrentTier >= 4) continue;", forge)
        self.assertIn("const int32 MaterialCost = CurrentTier == 0 ? 2 : 1;", forge)
        self.assertIn("!IsSafeCheckpointLocation()", forge)
        self.assertIn("SaveProgress();", forge)
        heal = cpp[cpp.index("float AMoonfallPawn::HealingSpellAmount() const"):
                   cpp.index("void AMoonfallPawn::Attack()")]
        self.assertIn("4.f * ForgedRegionTier(3)", heal)
        melee = cpp[cpp.index("void AMoonfallPawn::Attack()"):
                    cpp.index("void AMoonfallPawn::CastBolt()")]
        self.assertIn("Stamina + 2.f * ForgedRegionTier(0)", melee)
        self.assertIn("if (bComboFinisher && ForgedRegionTier(0) > 0)", melee)
        bolt = cpp[cpp.index("void AMoonfallPawn::CastBolt()"):
                   cpp.index("void AMoonfallPawn::CastHeal()")]
        self.assertIn("0.08f * ForgedRegionTier(5)", bolt)
        self.assertIn("FMath::Max(0.7f", bolt)
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertNotIn("INCOMING %.1fs", hud)

    def test_focus_and_melee_forge_inputs_do_not_spend_other_recipes(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        inputs = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        for declaration in ("void ForgeEquipment();", "void ForgeMeleeEquipment();",
                            "void ForgeArcaneEquipment();", "void ForgeEquipmentForType(int32 DesiredType);"):
            self.assertIn(declaration, header)
        for binding in ('ActionName="ForgeEquipment",Key=C',
                        'ActionName="ForgeEquipment",Key=Gamepad_RightShoulder',
                        'ActionName="ForgeMelee",Key=X',
                        'ActionName="ForgeArcane",Key=F',
                        'ActionName="ForgeMelee",Key=Gamepad_LeftThumbstick',
                        'ActionName="ForgeArcane",Key=Gamepad_RightThumbstick'):
            self.assertIn(binding, inputs)
        for name in ("ForgeEquipment", "ForgeMelee", "ForgeArcane"):
            self.assertIn('Input->BindAction(TEXT("' + name + '"), IE_Pressed', cpp)
        self.assertIn("ForgeEquipmentForType(-1);", cpp)
        self.assertIn("ForgeEquipmentForType(0);", cpp)
        self.assertIn("ForgeEquipmentForType(1);", cpp)
        forge = cpp[cpp.index("void AMoonfallPawn::ForgeEquipmentForType(int32 DesiredType)"):
                    cpp.index("float AMoonfallPawn::BasicAttackDamage()")]
        self.assertIn("if (DesiredType >= 0 && (Region % 2) != DesiredType) continue;", forge)
        self.assertLess(forge.index("if (DesiredType >= 0"),
                        forge.index("const int32 CurrentTier = ForgedRegionTier(Region);"))
        self.assertIn("!IsSafeCheckpointLocation()", forge)
        self.assertIn("const int32 MaterialCost = CurrentTier == 0 ? 2 : 1;", forge)
        self.assertEqual(forge.count("SaveProgress();"), 1)
        self.assertNotIn("INCOMING %.1fs",
            (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8"))

    def test_masterwork_rank_four_increases_every_regional_perk(self):
        # Rank IV must not be just an inventory label for the three bounded
        # cooldown, mana and stagger effects.
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        self.assertIn("FMath::Max(22.f, 30.f - 2.f * ForgedRegionTier(1))", cpp)
        self.assertIn("FMath::Min(0.8f, 0.4f + 0.1f * ForgedRegionTier(2))", cpp)
        self.assertIn("FMath::Max(0.7f,", cpp)
        self.assertIn("BoltCooldown - 0.08f * ForgedRegionTier(5)", cpp)
        self.assertEqual([max(22, 30 - 2 * t) for t in range(5)],
                         [30, 28, 26, 24, 22])
        self.assertEqual([round(min(0.8, 0.4 + 0.1 * t), 2) for t in range(5)],
                         [0.4, 0.5, 0.6, 0.7, 0.8])
        self.assertEqual([round(max(0.7, 1.1 - 0.08 * t), 2) for t in range(5)],
                         [1.1, 1.02, 0.94, 0.86, 0.78])

    def test_masterwork_uses_fifth_guardian_cache_with_bounded_rank_four(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        rules = (base / "Source/MoonfallPrototype/MoonfallWorldRules.h").read_text(encoding="utf-8")
        self.assertIn("ChambersPerRegion = 5", rules)
        scan = cpp[cpp.index("int32 AMoonfallPawn::ForgedRegionTier("):
                   cpp.index("void AMoonfallPawn::ForgeEquipment()")]
        self.assertIn("Rank >= 1 && Rank <= 4", scan)
        self.assertIn("return FMath::Min(12, Power);", scan)
        forge = cpp[cpp.index("void AMoonfallPawn::ForgeEquipmentForType(int32 DesiredType)"):
                    cpp.index("float AMoonfallPawn::BasicAttackDamage()")]
        self.assertIn("if (CurrentTier >= 4) continue;", forge)
        self.assertIn("const int32 MaterialCost = CurrentTier == 0 ? 2 : 1;", forge)
        self.assertIn("const int32 Id = Region * MoonfallWorldRules::ChambersPerRegion + Chamber;", forge)
        self.assertIn("if ((ClaimedChamberMask & (1 << Id)) == 0 ||", forge)
        self.assertIn("(DefeatedGuardianMask & (1 << Id)) == 0) continue;", forge)
        self.assertIn("MaterialsToSpend.Num() != MaterialCost", forge)
        self.assertIn("ConsumedMaterialNames.Contains(BossInventory[I])", forge)
        self.assertIn("Gear[Region], CurrentTier + 1", forge)
        self.assertEqual(forge.count("SaveProgress();"), 1)
        self.assertEqual(sum(2 if tier == 0 else 1 for tier in range(4)), 5)
        self.assertNotIn("INCOMING %.1fs",
            (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8"))

    def test_chamber_forging_ranks_are_unique_bounded_and_persistent(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        inputs = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("void ForgeEquipment();", header)
        self.assertIn("int32 ForgedRegionTier(int32 Region) const;", header)
        self.assertIn("int32 ForgedEquipmentPower(bool bArcane) const;", header)
        self.assertIn('ActionName="ForgeEquipment",Key=C', inputs)
        self.assertIn('ActionName="ForgeEquipment",Key=Gamepad_RightShoulder', inputs)
        self.assertIn('Input->BindAction(TEXT("ForgeEquipment"), IE_Pressed', cpp)
        scan = cpp[cpp.index("int32 AMoonfallPawn::ForgedRegionTier("):
                   cpp.index("void AMoonfallPawn::ForgeEquipment()")]
        self.assertIn("const FString RankText = Item.RightChop(Prefix.Len());", scan)
        self.assertIn("if (!RankText.IsNumeric()) continue;", scan)
        self.assertIn("Rank >= 1 && Rank <= 4", scan)
        self.assertIn("BestTier = FMath::Max(BestTier, Rank);", scan)
        self.assertIn("FMath::Min(12, Power)", scan)
        self.assertIn("Power += ForgedRegionTier(Region);", scan)
        forge = cpp[cpp.index("void AMoonfallPawn::ForgeEquipment()"):
                    cpp.index("float AMoonfallPawn::BasicAttackDamage()")]
        self.assertIn("!IsSafeCheckpointLocation()", forge)
        self.assertIn("if (CurrentTier >= 4) continue;", forge)
        self.assertIn("const int32 MaterialCost = CurrentTier == 0 ? 2 : 1;", forge)
        self.assertIn("MaterialsToSpend.Num() != MaterialCost", forge)
        self.assertIn("ClaimedChamberMask & (1 << Id)", forge)
        self.assertIn("DefeatedGuardianMask & (1 << Id)", forge)
        self.assertIn("BossInventory[I] != Material", forge)
        self.assertIn("MaterialsToSpend.Contains(I)", forge)
        self.assertIn("ConsumedMaterialNames.Add(Material);", forge)
        self.assertIn("ConsumedMaterialNames.Contains(BossInventory[I])", forge)
        self.assertIn("UpdatedInventory.Add(FString::Printf(", forge)
        self.assertIn("BossInventory = MoveTemp(UpdatedInventory);", forge)
        self.assertIn("SaveProgress();", forge)
        self.assertEqual(forge.count("SaveProgress();"), 1)
        self.assertIn("Forged Embersteel Blade", forge)
        self.assertIn("Forged Voidcrystal Focus", forge)
        self.assertIn("2.f * ForgedEquipmentPower(false)", cpp)
        self.assertIn("3.f * ForgedEquipmentPower(true)", cpp)
        self.assertNotIn("INCOMING %.1fs",
            (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8"))

    def test_chamber_materials_and_mastery_persist_without_new_save_fields(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("int32 ChamberMasteryCount() const;", header)
        mastery = cpp[cpp.index("int32 AMoonfallPawn::ChamberMasteryCount() const"):
                      cpp.index("float AMoonfallPawn::BasicAttackDamage() const")]
        self.assertIn("ClaimedChamberMask & (1 << Id)", mastery)
        self.assertIn("DefeatedGuardianMask & (1 << Id)", mastery)
        self.assertIn("0.75f * ChamberMasteryCount()", cpp)
        self.assertIn("1.25f * ChamberMasteryCount()", cpp)
        chest = cpp[cpp.index("void AMoonfallPawn::CollectNearbyChambers()"):
                    cpp.index("bool AMoonfallPawn::IsMapPositionDiscovered(")]
        self.assertIn("if ((DefeatedGuardianMask & (1 << Id)) == 0) continue;", chest)
        self.assertIn("BossInventory.Add(Reward);", chest)
        self.assertIn("if (BossInventory.Num() < 100)", chest)
        self.assertIn("if (bClaimedAny) SaveProgress();", chest)
        self.assertEqual(chest.count("SaveProgress();"), 1)

    def test_combat_damage_numbers_do_not_allocate_world_actors(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertIn("float GetDamageNumberValue() const", header)
        self.assertIn("float GetDamageNumberSeconds() const", header)
        self.assertIn("void AMoonfallPawn::ShowDamageNumber(float Damage, float RemainingHealth)", cpp)
        self.assertIn("FMath::Clamp(Damage, 0.f, FMath::Max(0.f, RemainingHealth))", cpp)
        self.assertIn("DamageNumberTimer = FMath::Max(0.f, DamageNumberTimer - Dt);", cpp)
        self.assertIn("ShowDamageNumber(Damage, Enemy.Health);", cpp)
        self.assertIn("ShowDamageNumber(Damage, BossHealth);", cpp)
        self.assertIn('TEXT("-%.0f")', hud)
        self.assertIn("Hero->GetDamageNumberSeconds() > 0.f", hud)
        self.assertNotIn("NewObject<", hud)

    def test_controller_atlas_selection_works_at_unzoomed_scale(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertIn("FVector2D WorldMapCursorCells = FVector2D(24.5f, 8.5f);", header)
        self.assertIn("GetWorldMapCursorCells() const", header)
        map_open = cpp[cpp.index("void AMoonfallPawn::ToggleWorldMap()"):
                       cpp.index("void AMoonfallPawn::CloseWorldMap()")]
        self.assertIn("WorldMapCursorCells = FVector2D(", map_open)
        self.assertIn("HeroMesh->GetRelativeLocation()", map_open)
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        map_branch = tick[tick.index("if (bWorldMapOpen)"):
                          tick.index("AttackCooldown = FMath::Max")]
        self.assertIn("WorldMapCursorCells.X = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapCursorCells.Y = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapPanCells.X = FMath::Clamp(", map_branch)
        self.assertIn("WorldMapPanCells.Y = FMath::Clamp(", map_branch)
        self.assertIn(".5f, 47.5f", map_branch)
        place = cpp[cpp.index("void AMoonfallPawn::SetMapCenterWaypoint()"):
                    cpp.index("void AMoonfallPawn::ControllerAttack()")]
        self.assertIn("FMath::FloorToInt(WorldMapCursorCells.X)", place)
        self.assertIn("FMath::FloorToInt(WorldMapCursorCells.Y)", place)
        self.assertIn("if (!IsMapPositionDiscovered(Candidate)) return;", place)
        self.assertIn("Origin + Hero->GetWorldMapCursorCells() * Cell", hud)

    def test_isometric_camera_tracks_smoothly_after_checkpoint_snap(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        begin = cpp[cpp.index("void AMoonfallPawn::BeginPlay()"):
                    cpp.index("UBillboardComponent* AMoonfallPawn::AddWorldArtwork(")]
        self.assertIn("CameraBoom->SetRelativeLocation(FVector(HeroPosition.X, HeroPosition.Y, 0.f));", begin)
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("CameraBoom->SetRelativeLocation(FMath::VInterpTo(", tick)
        self.assertIn("CameraBoom->GetRelativeLocation(), CameraTarget, FMath::Min(Dt, 0.05f), 9.f", tick)
        self.assertIn("CameraBoom->TargetArmLength = ZoomDistance;", cpp)

    def test_checkpoint_respawn_damage_grace_is_safe_and_paused_by_map(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        cpp = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        self.assertIn("float RespawnGraceTimer = 0.f;", header)
        begin = cpp[cpp.index("void AMoonfallPawn::BeginPlay()"):
                    cpp.index("UBillboardComponent* AMoonfallPawn::AddWorldArtwork(")]
        self.assertIn("RespawnGraceTimer = bLoadedCheckpoint ? 2.5f : 0.f;", begin)
        tick = cpp[cpp.index("void AMoonfallPawn::Tick(float DeltaSeconds)"):]
        self.assertIn("const bool bRespawnProtectedThisFrame = RespawnGraceTimer > 0.f;", tick)
        self.assertIn("RespawnGraceTimer = FMath::Max(0.f, RespawnGraceTimer - Dt);", tick)
        self.assertLess(tick.index("if (bWorldMapOpen)"),
                        tick.index("RespawnGraceTimer = FMath::Max(0.f"))
        self.assertIn("else if (!bRespawnProtectedThisFrame)", tick)
        self.assertIn("if (!bRespawnProtectedThisFrame)\n                    EnemyDamageCooldown = 1.2f;", tick)
        self.assertIn("else if (EnemyDamageCooldown <= 0.f && !bRespawnProtectedThisFrame)", tick)
        self.assertNotIn("SaveProgress();", tick[tick.index("const bool bRespawnProtectedThisFrame"):
                                                   tick.index("CounterWindow = FMath::Max")])

    def test_defeat_respawn_restores_last_living_checkpoint_without_new_save_mode(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        header = (base / "Source/MoonfallPrototype/MoonfallPawn.h").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        keys = (base / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertNotIn('ActionName="RespawnFromCheckpoint",Key=R', keys)
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', keys)
        self.assertIn("void RespawnFromCheckpoint();", header)
        self.assertIn("void ClickRespawnButton();", header)
        self.assertIn("GetRespawnButtonBounds(", header)
        self.assertIn("bDeathInputEnabled = false;", header)
        self.assertNotIn('Input->BindAction(TEXT("RespawnFromCheckpoint")', pawn)
        self.assertIn("if (Health <= 0.f) { ClickRespawnButton(); return; }", pawn)
        self.assertIn("if (Health <= 0.f && !bDeathInputEnabled)", pawn)
        self.assertIn("PC->bShowMouseCursor = true;", pawn)
        self.assertIn("PC->SetInputMode(DeathInput);", pawn)
        self.assertIn("GetRespawnButtonBounds(static_cast<float>(Width), static_cast<float>(Height), Min, Max);", pawn)
        self.assertIn("MouseX >= Min.X && MouseX <= Max.X && MouseY >= Min.Y && MouseY <= Max.Y", pawn)
        begin = pawn.index("void AMoonfallPawn::RespawnFromCheckpoint()")
        retry = pawn[begin:pawn.index("\n}", begin) + 2]
        self.assertIn("if (Health > 0.f || bWorldMapOpen || bRespawnInProgress || !GetWorld()) return;", retry)
        self.assertIn("UGameplayStatics::GetCurrentLevelName(this, true)", retry)
        self.assertIn("UGameplayStatics::OpenLevel(this, FName(*LevelName));", retry)
        self.assertNotIn("SaveProgress();", retry)
        self.assertIn("if (HeroMesh && Health > 0.f &&", pawn)
        self.assertIn('DrawText(TEXT("DEFEATED")', hud)
        self.assertIn('DrawText(TEXT("RESPAWN")', hud)
        self.assertIn("Click or press gamepad A to respawn at your checkpoint", hud)
        self.assertIn("Hero->GetRespawnButtonBounds(Canvas->ClipX, Canvas->ClipY", hud)
        self.assertIn("const FVector2D ButtonSize = ButtonMax - ButtonMin;", hud)
        self.assertIn("Canvas->K2_DrawBox(ButtonMin, ButtonSize", hud)

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
        self.assertIn("if (bWorldMapOpen)\n    {", pawn)
        self.assertIn("const float DodgeMovementDt = bDodgeActiveThisFrame", pawn)

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
