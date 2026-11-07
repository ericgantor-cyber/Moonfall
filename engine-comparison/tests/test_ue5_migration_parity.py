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

    def test_full_screen_atlas_pauses_combat_and_resource_clocks(self):
        base = ROOT / "engine-comparison/unreal/MoonfallPrototype"
        pawn = (base / "Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        hud = (base / "Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        tick = pawn[pawn.index("void AMoonfallPawn::Tick(float DeltaSeconds)"): ]
        map_branch = tick[tick.index("if (bWorldMapOpen)"):tick.index("AttackCooldown = FMath::Max")]
        self.assertIn("WorldMapPanCells.X +=", map_branch)
        self.assertIn("WorldMapPanCells.Y +=", map_branch)
        self.assertIn("return;", map_branch)
        self.assertLess(map_branch.index("WorldMapPanCells.Y +="), map_branch.index("return;"))
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
        self.assertIn("Click to respawn at your checkpoint", hud)
        self.assertNotIn('DrawText(TEXT("RETRY")', hud)

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
        self.assertIn("if (HeroMesh && Health > 0.f)", pawn)
        self.assertIn('DrawText(TEXT("DEFEATED")', hud)
        self.assertIn('DrawText(TEXT("RESPAWN")', hud)
        self.assertIn("Click to respawn at your checkpoint", hud)
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
