#!/usr/bin/env python3
"""Fast source-contract checks; not a substitute for compiling Unreal."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class MoonfallIntegrationChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero_cpp = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.enemy_h = (ROOT / "MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.enemy_cpp = (ROOT / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_unreal_startup_level_generator_is_safe_and_parseable(self):
        import ast
        import json
        generator = (ROOT.parents[1] / "Build" / "Create-StartupLevel.py").read_text(encoding="utf-8")
        ast.parse(generator)
        project = json.loads((ROOT.parents[1] / "MoonfallUnreal.uproject").read_text(encoding="utf-8"))
        self.assertTrue(any(p.get("Name") == "PythonScriptPlugin" and p.get("Enabled") for p in project.get("Plugins", [])))
        self.assertTrue(any(p.get("Name") == "EditorScriptingUtilities" and p.get("Enabled") for p in project.get("Plugins", [])))
        self.assertIn('MAP = "/Game/Maps/MoonfallStart"', generator)
        self.assertIn("EditorAssetLibrary.does_asset_exist(MAP)", generator)
        self.assertIn("def create_new_startup_level():", generator)
        self.assertIn("if not map_exists:", generator)
        self.assertIn("if map_exists:", generator)
        self.assertIn("map_file.stat().st_size <= 0", generator)
        self.assertIn("create_new_startup_level()", generator)
        self.assertIn("if map_file.exists():", generator)
        self.assertIn("levels.new_level(MAP, is_partitioned_world=False)", generator)
        self.assertIn("unreal.PlayerStart", generator)
        self.assertIn("unreal.NavMeshBoundsVolume", generator)
        self.assertIn("levels.save_current_level()", generator)
        self.assertIn("updated_map_config(original_config)", generator)
        self.assertIn("Path(unreal.Paths.project_dir()).resolve()", generator)
        self.assertIn('(project_root / "MoonfallUnreal.uproject").is_file()', generator)
        self.assertNotIn("get_project_file_path()", generator)
        self.assertIn("backup_path.write_text(original_config", generator)
        self.assertIn('backup_path.read_text(encoding="utf-8") != original_config', generator)
        self.assertIn('if backup_path.exists():', generator)
        self.assertIn("config_path.write_text(new_config", generator)

    def test_unreal_startup_map_config_preserves_other_settings(self):
        import ast
        import re
        source = (ROOT.parents[1] / "Build" / "Create-StartupLevel.py").read_text(encoding="utf-8")
        module = ast.parse(source)
        constants = {}
        for item in module.body:
            if isinstance(item, ast.Assign) and len(item.targets) == 1:
                target = item.targets[0]
                if isinstance(target, ast.Name) and target.id in ("MAP", "MAP_SECTION", "MAP_KEYS"):
                    constants[target.id] = ast.literal_eval(item.value)
        fn = next(item for item in module.body
                  if isinstance(item, ast.FunctionDef) and item.name == "updated_map_config")
        isolated = ast.Module(body=[fn], type_ignores=[])
        namespace = {"re": re, **constants}
        exec(compile(ast.fix_missing_locations(isolated), "<startup-config>", "exec"), namespace)
        original = (
            "[/Script/EngineSettings.GameMapsSettings]\n"
            "GlobalDefaultGameMode=/Script/MoonfallUnreal.MoonfallUnrealGameMode\n"
            "GameDefaultMap=/Engine/Maps/Entry\n"
            "EditorStartupMap=/Engine/Maps/Entry\n"
            "\n[/Script/Engine.RendererSettings]\n"
            "r.DefaultFeature.AntiAliasing=2\n"
        )
        revised = namespace["updated_map_config"](original)
        self.assertIn("GameDefaultMap=/Game/Maps/MoonfallStart", revised)
        self.assertIn("EditorStartupMap=/Game/Maps/MoonfallStart", revised)
        self.assertIn("r.DefaultFeature.AntiAliasing=2", revised)
        self.assertEqual(namespace["updated_map_config"](revised), revised)
        with self.assertRaises(ValueError):
            namespace["updated_map_config"](original.replace(
                "GameDefaultMap=/Engine/Maps/Entry\n", ""))
        with self.assertRaises(ValueError):
            namespace["updated_map_config"](original + original)


    def test_manual_windows_release_smoke_tests_before_installer(self):
        workflow = (ROOT.parents[2] / ".github" / "workflows" /
                    "unreal-windows-build.yml").read_text(encoding="utf-8")
        self.assertIn("Smoke-test packaged Unreal game launch", workflow)
        self.assertIn("Test-PackagedGame.ps1", workflow)
        self.assertIn("-ObservationSeconds 8", workflow)
        self.assertLess(workflow.index("Smoke-test packaged Unreal game launch"),
                        workflow.index("Upload packaged Windows build"))
        self.assertLess(workflow.index("Smoke-test packaged Unreal game launch"),
                        workflow.index("Build Windows installer with NSIS"))
        self.assertIn("Graphics and gameplay are not verified.", workflow)

    def test_manual_windows_release_uses_startup_map_preflight(self):
        workflow = (ROOT.parents[2] / ".github" / "workflows" /
                    "unreal-windows-build.yml").read_text(encoding="utf-8")
        self.assertIn("Build-Windows.ps1 -UnrealRoot $env:UE54_ROOT -PreflightOnly", workflow)
        self.assertIn("UnrealEditor-Cmd.exe", workflow)
        self.assertIn("-ExecutePythonScript=$script", workflow)
        self.assertIn("Create-StartupLevel.py", workflow)
        self.assertIn("MoonfallStart.umap", workflow)
        self.assertLess(workflow.index("Compile Unreal Editor Win64 Development"),
                        workflow.index("Generate and validate Moonfall startup level"))
        self.assertLess(workflow.index("Generate and validate Moonfall startup level"),
                        workflow.index("Package Windows game using AutomationTool"))
        self.assertLess(workflow.index("Generate and validate Moonfall startup level"),
                        workflow.index("Build-Windows.ps1 -UnrealRoot $env:UE54_ROOT -PreflightOnly"))
        self.assertIn("if (-not (Test-Path -LiteralPath $map -PathType Leaf))", workflow)
        self.assertIn('& $editor $project "-ExecutePythonScript=$script" -unattended -nop4', workflow)
        self.assertNotIn("if (-not (Test-Path -LiteralPath $map -PathType Leaf)) {", workflow.split("& $editor $project", 1)[0])
        self.assertLess(workflow.index("& $editor $project"), workflow.index("if (-not (Test-Path -LiteralPath $map -PathType Leaf))"))
        self.assertIn('"-map=/Game/Maps/MoonfallStart"', workflow)
        self.assertNotIn("Engine\\Content\\Maps\\Entry.umap", workflow)

    def test_windows_package_requires_project_startup_level(self):
        script = (ROOT.parents[1] / "Build" / "Build-Windows.ps1").read_text(encoding="utf-8")
        self.assertIn("MoonfallStart.umap", script)
        self.assertIn("GameDefaultMap", script)
        self.assertIn("if (-not $CompileOnly -and -not $CompilePreflightOnly) {", script)
        self.assertIn("if ($CompilePreflightOnly) {", script)
        self.assertIn("Playable startup level missing:", script)
        self.assertIn("foreach ($mapKey in @('GameDefaultMap', 'EditorStartupMap'))", script)
        self.assertIn("GlobalDefaultGameMode", script)
        self.assertIn("Get-Item -LiteralPath $startupMap", script)
        self.assertIn('"-map=/Game/Maps/MoonfallStart"', script)
        self.assertIn("BuildCookRun", script)
        self.assertIn("$matchingEntries.Count -ne 1", script)
        self.assertIn("$mapSectionMatches.Count -ne 1", script)
        self.assertIn("if ($PreflightOnly)", script)
        self.assertEqual(script.count("if (-not $CompileOnly -and -not $CompilePreflightOnly) {"), 2)
        self.assertNotIn("if (-not $CompileOnly -and -not $PreflightOnly)", script)
        self.assertIn("Get-Command $command", script)
        self.assertIn("makensis.exe", script)
        self.assertIn("dotnet --list-sdks", script)
        self.assertLess(script.index("if (-not $CompileOnly -and -not $CompilePreflightOnly) {", script.index("# Packaging preflight")),
                        script.index("if ($PreflightOnly)"))
        self.assertLess(script.index("dotnet --list-sdks"), script.index("if ($PreflightOnly)"))

    def test_progression_declared_and_defined(self):
        for name in ("AwardExperience", "AwardEquipmentDrop", "RewardPerfectDodge"):
            self.assertIn(name + "(", self.hero_h)
            self.assertIn("AMoonfallHeroPawn::" + name + "(", self.hero_cpp)

    def test_boss_health_and_attack_warning_contract(self):
        for name in ("BossMaxHealth", "BossWarningEndTime", "BossPhase"):
            self.assertIn(name, self.enemy_h)
            self.assertIn("Boss->" + name, self.hud)
        self.assertIn("void AMoonfallEnemy::BeginPlay()", self.enemy_cpp)
        self.assertIn("Hero->SpawnGroundEquipmentDrop", self.enemy_cpp)

    def test_death_closes_inventory_overlay(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("if (Health <= 0.f) {", damage)
        self.assertIn("EndSprint();", damage)
        self.assertIn("EndGuard();", damage)
        self.assertIn("bInventoryOpen = false;", damage)
        self.assertLess(damage.index("EndSprint();"), damage.index("EndGuard();"))
        self.assertLess(damage.index("if (Health <= 0.f) {"), damage.index("bInventoryOpen = false;"))

    def test_death_closes_options_and_inventory_overlays(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        defeat = damage.split("if (Health <= 0.f) {", 1)[1].split("if (!bReduceMotion)", 1)[0]
        self.assertIn("bInventoryOpen = false;", defeat)
        self.assertIn("bOptionsOpen = false;", defeat)
        self.assertLess(defeat.index("EndGuard();"), defeat.index("bOptionsOpen = false;"))

    def test_keyboard_controller_inventory_overlay_navigation_and_equip(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        for binding in ('ActionName="Inventory",Key=I', 'ActionName="Inventory",Key=Gamepad_Special_Left',
                        'ActionName="InventoryConfirm",Key=Enter',
                        'ActionName="InventoryConfirm",Key=Gamepad_FaceButton_Top'):
            self.assertIn(binding, inputs)
        self.assertIn("bool bInventoryOpen = false;", self.hero_h)
        self.assertIn("int32 InventorySelection = 0;", self.hero_h)
        self.assertIn("void ToggleInventory();", self.hero_h)
        self.assertIn("void EquipSelectedInventoryItem();", self.hero_h)
        self.assertIn('BindAction(TEXT("Inventory"), IE_Pressed', self.hero_cpp)
        self.assertIn('BindAction(TEXT("InventoryConfirm"), IE_Pressed', self.hero_cpp)
        toggle = self.hero_cpp.split("void AMoonfallHeroPawn::ToggleInventory()", 1)[1].split(
            "void AMoonfallHeroPawn::AdjustOption", 1)[0]
        self.assertIn("bInventoryOpen = !bInventoryOpen;", toggle)
        self.assertIn("EquipInventoryItem(InventorySelection);", toggle)
        self.assertIn("if (bInventoryOpen) { bInventoryOpen = false; return; }", toggle)
        self.assertIn("InventorySelection = FMath::Max(0, InventorySelection - 1);", toggle)
        self.assertIn("InventorySelection = FMath::Min(FMath::Max(0, InventoryItemCodes.Num() - 1), InventorySelection + 1);", toggle)
        self.assertIn("if (bInventoryOpen) { EquipSelectedInventoryItem(); return; }", toggle)
        self.assertIn("if (Health <= 0.f || bOptionsOpen || bInventoryOpen || !FMath::IsFinite(Value)) return;", self.hero_cpp)
        self.assertIn("bOptionsOpen || bInventoryOpen || !GetWorld()", self.hero_cpp)
        self.assertIn("if (Hero->bInventoryOpen)", self.hud)
        self.assertIn("const int32 PerPage = 8;", self.hud)
        self.assertIn("Hero->InventoryItemCodes[Index]", self.hud)
        self.assertIn("Hero->EquippedWeaponCode", self.hud)
        self.assertIn("Hero->EquippedArmorCode", self.hud)
        self.assertIn("Hero->EquippedAccessoryCode", self.hud)
        self.assertIn("I / Select / Esc: close", self.hud)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_diablo_style_ground_loot_requires_explicit_pickup(self):
        import re
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        ground_h = (ROOT / "MoonfallGroundLoot.h").read_text(encoding="utf-8")
        ground_cpp = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        self.assertIn("Hero->SpawnGroundEquipmentDrop(bElite, bBoss, GetActorLocation());", self.enemy_cpp)
        self.assertNotIn("Hero->AwardEquipmentDrop(", self.enemy_cpp)
        for action in ('ActionName="PickupLoot",Key=F', 'ActionName="PickupLoot",Key=Gamepad_FaceButton_Left'):
            self.assertIn(action, inputs)
        self.assertIn('BindAction(TEXT("PickupLoot"), IE_Pressed', self.hero_cpp)
        self.assertIn("void PickupNearestGroundLoot();", self.hero_h)
        self.assertIn("void SpawnGroundEquipmentDrop(", self.hero_h)
        self.assertIn("bool CollectGroundEquipmentCode(int32 ItemCode);", self.hero_h)
        spawn = self.hero_cpp.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertIn("RollEquipmentCode(bEliteEnemy, bBossEnemy)", spawn)
        self.assertIn("const int32 DropCount = bBossEnemy ? 3 : (bEliteEnemy ? 2 : 1);", spawn)
        self.assertIn("for (int32 Drop = 0; Drop < DropCount; ++Drop)", spawn)
        self.assertIn("Spread * FMath::Cos(Angle)", spawn)
        self.assertIn("Spread * FMath::Sin(Angle)", spawn)
        self.assertIn("SpawnActor<AMoonfallGroundLoot>", spawn)
        self.assertIn("Loot->InitializeLoot(ItemCode);", spawn)
        pickup = self.hero_cpp.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("bOptionsOpen || bInventoryOpen", pickup)
        self.assertIn("FMath::Square(250.f)", pickup)
        self.assertIn("TActorIterator<AMoonfallGroundLoot>", pickup)
        self.assertIn("Loot->TryCollect()", pickup)
        collect = self.hero_cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("ItemCode != Affix * 100000 + SlotIndex * 10000", collect)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", collect)
        self.assertNotIn("EquippedCode =", collect)
        self.assertNotIn("Salvage =", collect)
        self.assertNotIn("RemoveAt(", collect)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", ground_cpp)
        self.assertIn("CreateDefaultSubobject<UTextRenderComponent>", ground_cpp)
        self.assertIn("CreateDefaultSubobject<UStaticMeshComponent>", ground_cpp)
        self.assertIn("SetTextRenderColor(QualityColor);", ground_cpp)
        self.assertIn("if (bCollected || ItemCode <= 0) return false;", ground_cpp)
        self.assertIn("Hero->CollectGroundEquipmentCode(ItemCode)", ground_cpp)
        self.assertLess(ground_cpp.index("Hero->CollectGroundEquipmentCode(ItemCode)"), ground_cpp.index("Destroy();"))
        self.assertIn("ItemCode = 0;", ground_h)
        self.assertNotIn("SetLifeSpan(", ground_cpp)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_nearby_batch_loot_is_range_and_cover_safe(self):
        pickup = self.hero_cpp.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("constexpr int32 MaxLootScanned = 128;", pickup)
        self.assertIn("constexpr int32 MaxLootPerPress = 6;", pickup)
        self.assertIn("It && Scanned < MaxLootScanned;", pickup)
        self.assertIn("FMath::Square(250.f)", pickup)
        self.assertIn("FMath::Abs(LootPosition.Z - HeroPosition.Z) > 150.f", pickup)
        self.assertIn("SCENE_QUERY_STAT(MoonfallGroundLootPickupSight)", pickup)
        self.assertIn("LootSightParams.AddIgnoredActor(Candidate);", pickup)
        self.assertIn("GetWorld()->LineTraceSingleByChannel(LootSightHit,", pickup)
        self.assertIn("NearbyLoot.Insert(Candidate, InsertAt);", pickup)
        self.assertIn("NearbyLoot.RemoveAt(MaxLootPerPress);", pickup)
        self.assertIn("for (AMoonfallGroundLoot* Loot : NearbyLoot)", pickup)
        self.assertIn("if (IsValid(Loot)) Loot->TryCollect();", pickup)
        self.assertNotIn("EquipInventoryItem(", pickup)
        self.assertNotIn("SalvageInventoryItem(", pickup)
        self.assertNotIn("PrimaryActorTick", pickup)

    def test_inventory_accepts_new_loot_without_rejection_or_auto_cleanup(self):
        loot = self.hero_cpp.split("void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("InventoryItemCodes.Add(ItemCode);", loot)
        self.assertNotIn("InventoryItemCodes.Num() >= 30", loot)
        self.assertNotIn("DiscardIndex", loot)
        self.assertNotIn("DiscardedCode", loot)
        self.assertNotIn("InventoryItemCodes.RemoveAt(", loot)
        self.assertIn("InventorySelection = FMath::Clamp(InventorySelection, 0, InventoryItemCodes.Num() - 1);", loot)

    def test_explicit_equip_best_loadout_is_validated_and_never_auto_equips_loot(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="EquipBest",Key=B', inputs)
        self.assertIn('ActionName="EquipBest",Key=Gamepad_LeftTrigger', inputs)
        self.assertIn('BindAction(TEXT("EquipBest"), IE_Pressed', self.hero_cpp)
        self.assertIn('int32 EquipBestInventoryUpgrades();', self.hero_h)
        self.assertIn('void OnBestLoadoutEquipped(int32 UpgradesEquipped);', self.hero_h)
        body = self.hero_cpp.split("int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("if (!bInventoryOpen || bOptionsOpen || Health <= 0.f) return 0;", body)
        self.assertIn("const int32 EquippedCodes[3]", body)
        self.assertIn("BestScores[3]", body)
        self.assertIn("InventoryItemCodes.Num()", body)
        self.assertIn("Tier * 100 + Rarity * 10 + (Affix > 0 ? 1 : 0)", body)
        self.assertIn("Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier", body)
        self.assertIn("if (Score > BestScores[Slot])", body)
        self.assertIn("EquipInventoryItem(BestIndices[Slot])", body)
        self.assertIn("OnBestLoadoutEquipped(UpgradesEquipped);", body)
        self.assertNotIn("SalvageInventoryItem(", body)
        self.assertNotIn("RemoveAt(", body)
        pickup = self.hero_cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertNotIn("EquipBestInventoryUpgrades", pickup)

    def test_inventory_salvage_is_validated_and_protects_equipped_gear(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="InventorySalvage",Key=Delete', inputs)
        self.assertIn('ActionName="InventorySalvage",Key=Gamepad_FaceButton_Left', inputs)
        self.assertIn("bool SalvageInventoryItem(int32 InventoryIndex);", self.hero_h)
        self.assertIn("void SalvageSelectedInventoryItem();", self.hero_h)
        self.assertIn('BindAction(TEXT("InventorySalvage"), IE_Pressed', self.hero_cpp)
        salvage = self.hero_cpp.split("bool AMoonfallHeroPawn::SalvageInventoryItem(int32 InventoryIndex)", 1)[1].split(
            "void AMoonfallHeroPawn::InventoryConfirm()", 1)[0]
        self.assertIn("InventoryItemCodes.IsValidIndex(InventoryIndex)", salvage)
        self.assertIn("Code == EquippedWeaponCode || Code == EquippedArmorCode", salvage)
        self.assertIn("Code == EquippedAccessoryCode", salvage)
        self.assertIn("if (bEquippedCode)", salvage)
        self.assertIn("for (const int32 Candidate : InventoryItemCodes)", salvage)
        self.assertIn("if (Copies <= 1) return false;", salvage)
        self.assertIn("Rarity < 0 || Rarity > 3 || Tier < 1 || Tier > 25", salvage)
        self.assertIn("(Rarity == 0 && Affix != 0)", salvage)
        self.assertIn("const int32 SalvageReward = 1 + Rarity * 2 + Tier / 5;", salvage)
        self.assertIn("Salvage = FMath::Clamp(Salvage + SalvageReward, 0, 9999);", salvage)
        self.assertIn("InventoryItemCodes.RemoveAt(InventoryIndex);", salvage)
        self.assertLess(salvage.index("Code == EquippedWeaponCode"), salvage.index("Salvage = FMath::Clamp("))
        self.assertLess(salvage.index("Salvage = FMath::Clamp("), salvage.index("InventoryItemCodes.RemoveAt(InventoryIndex);"))
        self.assertIn("Hero->Salvage", self.hud)
        self.assertIn("Delete / X: salvage", self.hud)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_inventory_equipping_validates_item_codes_and_recalculates_armor(self):
        self.assertIn("bool EquipInventoryItem(int32 InventoryIndex);", self.hero_h)
        for field in ("EquippedWeaponCode", "EquippedArmorCode", "EquippedAccessoryCode"):
            self.assertIn("int32 " + field + " = 0;", self.hero_h)
        equip = self.hero_cpp.split("bool AMoonfallHeroPawn::EquipInventoryItem(int32 InventoryIndex)", 1)[1]
        self.assertIn("InventoryItemCodes.IsValidIndex(InventoryIndex)", equip)
        self.assertIn("const int32 Code = InventoryItemCodes[InventoryIndex];", equip)
        self.assertIn("if (Affix < 0 || Affix > 3", equip)
        self.assertIn("SlotIndex < 0 || SlotIndex > 2", equip)
        self.assertIn("Rarity < 0 || Rarity > 3", equip)
        self.assertIn("Tier < 1 || Tier > 25", equip)
        self.assertIn("(Rarity == 0 && Affix != 0)", equip)
        self.assertIn("Code != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier", equip)
        self.assertIn("*EquippedTier = Tier;", equip)
        self.assertIn("*EquippedAffix = Affix;", equip)
        self.assertIn("*EquippedCode = Code;", equip)
        self.assertIn("MaxHealth = FMath::Max(1.f, MaxHealth + HealthDelta);", equip)
        self.assertIn("Health = FMath::Clamp(Health + HealthDelta, 1.f, MaxHealth);", equip)
        self.assertIn("OnHealthChanged(Health, MaxHealth);", equip)
        self.assertLess(equip.index("Code != Affix * 100000"), equip.index("*EquippedTier = Tier;"))
        loot = self.hero_cpp.split("void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("*EquippedCode = ItemCode;", loot)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", loot)

    def test_random_item_affixes_apply_bounded_slot_specific_bonuses(self):
        for field in ("LastLootAffix", "WeaponAffix", "ArmorAffix", "AccessoryAffix"):
            self.assertIn("int32 " + field + " = 0;", self.hero_h)
        loot = self.hero_cpp.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        self.assertIn("LastLootAffix = LastLootRarity == 0 ? 0", loot)
        self.assertIn("FMath::RandRange(1, 3)", loot)
        self.assertIn("LastLootAffix * 100000 + LastLootSlot * 10000", loot)
        self.assertIn("int32* EquippedAffix =", self.hero_cpp)
        self.assertIn("*EquippedAffix = LastLootAffix;", self.hero_cpp)
        self.assertLess(self.hero_cpp.index("if (NewTier > *Slot) {"), self.hero_cpp.index("*EquippedAffix = LastLootAffix;"))
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("(WeaponAffix == 1 ? 0.06f : 0.f)", cast)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("FMath::Min(0.34f", damage)
        self.assertIn("(ArmorAffix == 2 ? 0.06f : 0.f)", damage)
        self.assertIn("(AccessoryAffix == 3 ? 0.08f : 0.f)", self.hero_cpp)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", self.hero_cpp)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_randomized_loot_rarity_is_bounded_with_boss_guarantees(self):
        loot = self.hero_cpp.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        self.assertIn("const int32 LootRoll = FMath::RandRange(0, 99);", loot)
        self.assertIn("LastLootSlot = FMath::RandRange(0, 2);", loot)
        self.assertIn("const int32 RarityBias = FMath::Clamp(Level / 5, 0, 12);", loot)
        self.assertIn("LastLootRarity = bBossEnemy ? 3", loot)
        self.assertIn("(bEliteEnemy ? (LootRoll < 15 ? 3 : 2)", loot)
        self.assertIn("LootRoll < 3 + RarityBias / 3 ? 3", loot)
        self.assertIn("LootRoll < 18 + RarityBias ? 2", loot)
        self.assertIn("LootRoll < 53 + RarityBias ? 1 : 0", loot)
        self.assertIn("FMath::Clamp(Level / 5", loot)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", self.hero_cpp)
        self.assertIn("LastLootAffix * 100000 + LastLootSlot * 10000", loot)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_equipment_stats_affect_combat(self):
        for name in ("WeaponTier", "ArmorTier", "AccessoryTier"):
            self.assertIn(name, self.hero_h)
            self.assertIn(name, self.hero_cpp)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", self.hero_cpp)

    def test_damage_hud_uses_current_max_health(self):
        start = self.hero_cpp.index("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {")
        end = self.hero_cpp.index("void AMoonfallHeroPawn::", start + 5)
        damage_code = self.hero_cpp[start:end]
        self.assertIn("OnHealthChanged(Health, MaxHealth)", damage_code)
        self.assertNotIn("OnHealthChanged(Health, 100.f)", damage_code)

    def test_opening_menus_releases_held_sprint(self):
        inventory = self.hero_cpp.split("void AMoonfallHeroPawn::ToggleInventory()", 1)[1].split(
            "void AMoonfallHeroPawn::EquipSelectedInventoryItem()", 1)[0]
        self.assertIn("if (!bInventoryOpen) {\n  StopJumping();\n  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();\n  EndSprint();\n  EndGuard();", inventory)
        options = self.hero_cpp.split("void AMoonfallHeroPawn::OpenOptions()", 1)[1].split(
            "void AMoonfallHeroPawn::OptionsUp()", 1)[0]
        self.assertIn("if (!bOptionsOpen) {\n  StopJumping();\n  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();\n  EndSprint();\n  EndGuard();", options)

    def test_options_cannot_reopen_while_hero_is_defeated(self):
        options = self.hero_cpp.split("void AMoonfallHeroPawn::OpenOptions()", 1)[1].split(
            "void AMoonfallHeroPawn::OptionsUp()", 1)[0]
        self.assertIn("if (Health <= 0.f || !FMath::IsFinite(Health)) return;", options)
        self.assertLess(options.index("if (Health <= 0.f || !FMath::IsFinite(Health)) return;"),
                        options.index("if (bInventoryOpen)"))
        self.assertLess(options.index("if (Health <= 0.f || !FMath::IsFinite(Health)) return;"),
                        options.index("bOptionsOpen = !bOptionsOpen;"))
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("bInventoryOpen = false;", damage)
        self.assertIn("bOptionsOpen = false;", damage)

    def test_batch_menus_cancel_enemy_windups_and_followups(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        menu = tick.split("// Pause hostile behavior during both gameplay menus", 1)[1].split(
            "// A shatter follow-up expires", 1)[0]
        self.assertIn("Hero->bOptionsOpen || Hero->bInventoryOpen", menu)
        for action in ("WindupEndsAt = -1.f;", "BossWarningEndTime = -1.f;",
                       "bComboSecondHitPending = false;", "bComboSecondHitActive = false;",
                       "LockedAttackDirection = FVector::ZeroVector;"):
            self.assertIn(action, menu)
        self.assertIn("NextAttackAt = FMath::Max(", menu)
        self.assertIn("Now + 0.35f", menu)
        self.assertLess(menu.index("bInventoryOpen"), menu.index("return;"))
        self.assertLess(tick.index("// Pause hostile behavior during both gameplay menus"),
                        tick.index("if (WindupEndsAt > 0.f && Now >= WindupEndsAt)"))
        self.assertEqual(tick.count("AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));"), 1)

    def test_batch_guard_validates_timers_before_stance(self):
        begin = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        for field in ("GuardNow", "DamageImmunityUntil", "GuardBrokenUntil", "NextParryAt"):
            self.assertIn(f"!FMath::IsFinite({field})", begin)
            self.assertLess(begin.index(f"!FMath::IsFinite({field})"), begin.index("EndSprint();"))
        end = self.hero_cpp.split("void AMoonfallHeroPawn::EndGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginSprint()", 1)[0]
        self.assertIn("FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt", end)
        self.assertIn("GetWorld()->GetTimeSeconds() + 0.4f", end)

    def test_batch_perfect_parry_requires_finite_windows(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        parry = damage.split("const bool bPerfectParry =", 1)[1].split(
            "const float RequiredGuardCost =", 1)[0]
        self.assertIn("FMath::IsFinite(ParryWindowEndsAt)", parry)
        self.assertIn("FMath::IsFinite(NextParryAt)", parry)
        self.assertIn("Now <= ParryWindowEndsAt && Now <= NextParryAt", parry)

    def test_batch_enemy_posture_uses_finite_frame_delta(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.4f) : 0.f", tick)
        self.assertLess(tick.index("FMath::IsFinite(DeltaSeconds) ? FMath::Clamp("),
                        tick.index("OnPostureChanged(Posture, MaxPosture)"))

    def test_ai_batch_clamps_chase_and_dash_simulation_steps(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("if (!FMath::IsFinite(Now)) return;", tick)
        self.assertIn("const float SafeAIDelta = FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f;", tick)
        self.assertIn("FMath::Min(960.f * SafeAIDelta, Remaining)", tick)
        self.assertIn("FMath::IsFinite(ChaseSpeed) ? ChaseSpeed : 0.f", tick)
        self.assertIn("const float SafeChaseSpeed = FMath::Clamp(", tick)
        self.assertIn("ChaseSpeed : 0.f, 0.f, 2000.f);", tick)
        self.assertIn("SafeChaseSpeed * ChaseMultiplier * SafeAIDelta, true, &Hit)", tick)
        self.assertLess(tick.index("const float SafeAIDelta"), tick.index("AddActorWorldOffset("))

    def test_ai_batch_invalid_spell_damage_cannot_consume_shatter(self):
        spell = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::BeginFinisherDefeat(", 1)[0]
        self.assertIn("!FMath::IsFinite(EffectiveDamage) || EffectiveDamage <= 0.f", spell)
        self.assertLess(spell.index("!FMath::IsFinite(EffectiveDamage)"),
                        spell.index("if (bShatterFinisher) bPostureFinisherAvailable = false;"))
        self.assertLess(spell.index("!FMath::IsFinite(EffectiveDamage)"),
                        spell.index("Health = FMath::Max(0.0f, Health -"))

    def test_ai_batch_guard_recovery_and_vitals_are_safe(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : GetWorld()->GetTimeSeconds()", tick)
        guard = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("Health <= 0.f || !FMath::IsFinite(Health)", guard)
        self.assertLess(guard.index("!FMath::IsFinite(Health)"), guard.index("EndSprint();"))

    def test_enemy_spawn_scaling_rejects_invalid_blueprint_vitals(self):
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay()", 1)[1].split(
            "void AMoonfallEnemy::ApplyPostureDamage", 1)[0]
        self.assertIn("FMath::IsFinite(Health) && Health > 0.f ? Health : 1.f", begin)
        self.assertIn("FMath::IsFinite(AttackDamage) && AttackDamage > 0.f ? AttackDamage : 1.f", begin)
        self.assertIn("FMath::Clamp(BaseHealth, 1.f, 100000.f) * HPScale", begin)
        self.assertIn("FMath::Clamp(BaseDamage, 1.f, 10000.f) * DamageScale", begin)
        self.assertLess(begin.index("const float BaseHealth"), begin.index("MaxHealthAtSpawn ="))

    def test_enemy_posture_break_rejects_invalid_cooldown_timers(self):
        posture = self.enemy_cpp.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        for field in ("Now", "PostureBrokenUntil", "NextPostureBreakAllowedAt"):
            self.assertIn(f"!FMath::IsFinite({field})", posture)
            self.assertLess(posture.index(f"!FMath::IsFinite({field})"), posture.index("Posture = FMath::Clamp("))
        self.assertIn("if (Now < PostureBrokenUntil || Now < NextPostureBreakAllowedAt) return;", posture)

    def test_enemy_ai_rejects_invalid_hero_and_posture_recovery(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("!FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f", tick)
        self.assertLess(tick.index("!FMath::IsFinite(Hero->Health)"), tick.index("FVector Difference ="))
        self.assertIn("FMath::IsFinite(Posture) && Posture > 0.f", tick)
        self.assertIn("FMath::IsFinite(PostureRecoveryStartsAt) && Now >= PostureRecoveryStartsAt", tick)
        self.assertLess(tick.index("FMath::IsFinite(PostureRecoveryStartsAt)"), tick.index("OnPostureChanged(Posture, MaxPosture)"))

    def test_boss_batch_rejects_corrupted_target_positions(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("HeroPosition.ContainsNaN() || EnemyPosition.ContainsNaN()", tick)
        self.assertIn("if (!FMath::IsFinite(Distance)) return;", tick)
        self.assertLess(tick.index("HeroPosition.ContainsNaN()"), tick.index("const bool bInCombatArea"))

    def test_boss_batch_recovery_preserves_staggers_and_bounds_cooldowns(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("FMath::IsFinite(AttackCooldown) ? FMath::Clamp(AttackCooldown, 0.65f, 10.f) : 1.f", tick)
        self.assertIn("NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, AttackRecoveryAt);", tick)
        self.assertLess(tick.index("const float RecoveryCooldown"), tick.index("const FVector ImpactDirection"))

    def test_boss_batch_validates_attack_damage_before_dodge_or_parry(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("const float IncomingAttackDamage = AttackDamage * FMath::Min(2.3f, CurrentAttackMultiplier);", tick)
        self.assertIn("!FMath::IsFinite(IncomingAttackDamage) || IncomingAttackDamage <= 0.f", tick)
        self.assertLess(tick.index("!FMath::IsFinite(IncomingAttackDamage)"),
                        tick.index("const bool bPerfectDodge = Hero->TryRewardPerfectDodge();"))
        self.assertIn("IncomingAttackDamage, GetActorLocation());", tick)

    def test_boss_batch_invalid_nav_cache_resets_before_chasing(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("!FMath::IsFinite(NextPathRefreshAt) || Now >= NextPathRefreshAt", tick)
        self.assertIn("if (CachedWaypoint.ContainsNaN()) {", tick)
        self.assertIn("CachedWaypoint = FVector::ZeroVector;", tick)
        self.assertLess(tick.index("if (CachedWaypoint.ContainsNaN())"),
                        tick.index("FVector ToWaypoint = CachedWaypoint - GetActorLocation();"))

    def test_unreal_mousewheel_zoom_clean_backport(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="ZoomIn",Key=MouseScrollUp', inputs)
        self.assertIn('ActionName="ZoomOut",Key=MouseScrollDown', inputs)
        self.assertIn('BindAction(TEXT("ZoomIn"), IE_Pressed', self.hero_cpp)
        self.assertIn('BindAction(TEXT("ZoomOut"), IE_Pressed', self.hero_cpp)
        self.assertIn('float CameraDistance = 450.f;', self.hero_h)
        for name, operator in (("ZoomCameraIn", "-"), ("ZoomCameraOut", "+")):
            action = self.hero_cpp.split(f"void AMoonfallHeroPawn::{name}()", 1)[1].split("\n}", 1)[0]
            self.assertIn("bOptionsOpen || bInventoryOpen", action)
            self.assertIn("FMath::IsFinite(CameraDistance) ? CameraDistance : 450.f", action)
            self.assertIn(f"{operator} 65.f, 250.f, 850.f", action)
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("FMath::IsFinite(CameraBoom->TargetArmLength)", tick)
        self.assertIn("FMath::FInterpTo(CurrentCameraDistance, TargetCameraDistance, SafeCameraDelta, 10.f)", tick)
        self.assertIn("bReduceMotion ? 0.f : ImpactCameraOffset", tick)

    def test_guard_clears_held_sprint_before_entering_stance(self):
        guard = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("EndSprint();", guard)
        self.assertLess(guard.index("EndSprint();"), guard.index("bGuarding = true;"))
        self.assertIn("|| Stamina < GuardStaminaCost) return;", guard)
        self.assertLess(guard.index("|| Stamina < GuardStaminaCost) return;"), guard.index("EndSprint();"))

    def test_menu_regen_restores_hp_mana_and_stamina(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("Mana + FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f)", tick)
        self.assertIn("(bOptionsOpen || bInventoryOpen) && Health > 0.f", tick)
        self.assertIn("FMath::IsFinite(MenuHealthRegenPerSecond)", tick)
        self.assertIn("FMath::Clamp(MenuHealthRegenPerSecond, 0.f, 20.f)", tick)
        self.assertIn("OnHealthChanged(Health, MaxHealth)", tick)
        self.assertIn("if (Health > 0.f && !bSprinting && !bGuarding && GetWorld()", tick)
        self.assertNotIn("!bOptionsOpen && !bInventoryOpen\\n     && !bSprinting", tick)
        self.assertIn("float MenuHealthRegenPerSecond = 2.f;", self.hero_h)
        self.assertIn("float MenuRecoveryMultiplier = 1.5f;", self.hero_h)
        self.assertIn("FMath::Clamp(MenuRecoveryMultiplier, 1.f, 2.f)", tick)
        self.assertIn("Mana + FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f) * MenuRecoveryBoost", tick)

    def test_menu_health_regen_waits_after_real_damage(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() >= MenuHealthRegenStartsAt", tick)
        self.assertIn("MenuHealthRegenStartsAt = Now + (FMath::IsFinite(MenuHealthRegenDelaySeconds)", damage)
        self.assertLess(damage.index("if (DamageTaken > 0.f) {"),
                        damage.index("MenuHealthRegenStartsAt = Now +"))
        self.assertIn("float MenuHealthRegenDelaySeconds = 3.f;", self.hero_h)
        self.assertIn("float MenuHealthRegenStartsAt = 0.f;", self.hero_h)

    def test_combat_mana_pacing_and_perfect_parry_reward(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() < CombatRecoveryEndsAt", tick)
        self.assertIn("FMath::Clamp(CombatManaRegenMultiplier, 0.f, 1.f)", tick)
        self.assertIn("FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f) * MenuRecoveryBoost * CombatManaFactor", tick)
        self.assertIn("CombatRecoveryEndsAt = FMath::Max(", cast)
        self.assertIn("CombatRecoveryEndsAt = FMath::Max(", damage)
        self.assertIn("Mana + FMath::Clamp(PerfectParryManaReward, 0.f, 15.f)", damage)
        self.assertIn("float CombatRecoverySeconds = 5.f;", self.hero_h)
        self.assertIn("float CombatManaRegenMultiplier = 0.5f;", self.hero_h)
        self.assertIn("float PerfectParryManaReward = 5.f;", self.hero_h)

    def test_menu_hp_healing_requires_out_of_combat_recovery(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("const bool bInCombatRecoveryWindow = GetWorld()", tick)
        health_gate = tick.split("// Recovery in either menu is bounded and never revives a defeated hero.", 1)[1].split(
            "const float BeforeRegen = Health;", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() >= MenuHealthRegenStartsAt", health_gate)
        self.assertIn("&& !bInCombatRecoveryWindow) {", health_gate)
        self.assertLess(health_gate.index("GetWorld()->GetTimeSeconds() >= MenuHealthRegenStartsAt"),
                        health_gate.index("&& !bInCombatRecoveryWindow) {"))

    def test_regen_uses_safe_delta_and_combat_fallback(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f", tick)
        self.assertIn("const float StaminaDelta = SafeRecoveryDelta;", tick)
        self.assertIn("* SafeRecoveryDelta);", tick)
        self.assertIn("FMath::Clamp(AccessoryTier * 0.012f, 0.f, 0.25f)", tick)
        self.assertIn("FMath::Clamp(CombatManaRegenMultiplier, 0.f, 1.f) : 0.5f", tick)
        self.assertIn("Mana >= 0.f && Mana < MaxMana", tick)

    def test_perfect_parry_extends_combat_recovery(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        parry = damage.split("if (bPerfectParry) {", 1)[1].split("bLastIncomingHitParried", 1)[0]
        self.assertIn("CombatRecoveryEndsAt = FMath::Max(", damage)
        self.assertLess(damage.index("CombatRecoveryEndsAt = FMath::Max("), damage.index("if (bPerfectParry) {"))
        self.assertIn("FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f", damage)
        self.assertIn("PerfectParryManaReward", parry)

    def test_combat_recovery_timers_handle_nonfinite_tuning(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("FMath::IsFinite(CombatRecoverySeconds)", cast)
        self.assertEqual(damage.count("FMath::IsFinite(CombatRecoverySeconds)"), 2)
        self.assertIn("FMath::IsFinite(MenuHealthRegenDelaySeconds)", damage)
        self.assertIn("FMath::Clamp(MenuHealthRegenDelaySeconds, 0.f, 10.f) : 3.f", damage)
        self.assertIn("FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f", damage)

    def test_finisher_kills_refresh_combat_recovery(self):
        finisher = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        self.assertIn("if (Health <= 0.f || !GetWorld()) return 0;", finisher)
        self.assertIn("CombatRecoveryEndsAt = FMath::Max(", finisher)
        self.assertIn("FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f", finisher)
        self.assertLess(finisher.index("if (Health <= 0.f || !GetWorld()) return 0;"),
                        finisher.index("CombatRecoveryEndsAt = FMath::Max("))

    def test_normal_blocks_also_refresh_combat_recovery(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        guard = damage.split("if (bGuarding && bFrontAttack && !bOptionsOpen", 1)[1].split(
            "if (bPerfectParry) {", 1)[0]
        self.assertIn("CombatRecoveryEndsAt = FMath::Max(", guard)
        self.assertIn("Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina)", guard)
        self.assertLess(guard.index("CombatRecoveryEndsAt = FMath::Max("), guard.index("Stamina = FMath::Clamp"))

    def test_valid_hostile_cast_sets_combat_without_free_combo(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        self.assertLess(cast.index("Enemy->TakeSpellDamage(SpellDamage);"),
                        cast.index("CombatRecoveryEndsAt = FMath::Max("))
        self.assertLess(cast.index("CombatRecoveryEndsAt = FMath::Max("),
                        cast.index("if (ActualSpellDamage > 0.f) {"))
        self.assertLess(cast.index("if (ActualSpellDamage > 0.f) {"),
                        cast.index("HitComboStage = bComboAlive"))
        self.assertIn("FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f", cast)

    def test_combat_recovery_windows_never_shorten(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        finisher = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        for body, count in ((cast, 1), (damage, 2), (finisher, 1)):
            self.assertEqual(body.count("CombatRecoveryEndsAt = FMath::Max("), count)
            self.assertEqual(body.count("FMath::IsFinite(CombatRecoveryEndsAt) ? CombatRecoveryEndsAt : 0.f"), count)

    def test_regeneration_rates_are_bounded_even_with_extreme_finite_tuning(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("FMath::IsFinite(ManaRegenPerSecond) && ManaRegenPerSecond > 0.f", tick)
        self.assertIn("FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f)", tick)
        self.assertIn("FMath::IsFinite(StaminaRegenPerSecond) && StaminaRegenPerSecond > 0.f", tick)
        self.assertIn("FMath::Clamp(StaminaRegenPerSecond, 0.f, 250.f)", tick)
        self.assertIn("const float SafeRecoveryDelta = FMath::IsFinite(DeltaSeconds)", tick)

    def test_gameplay_movement_camera_and_jump_obey_menu_and_defeat_locks(self):
        controls = self.hero_cpp.split("void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[1].split(
            "bool AMoonfallHeroPawn::TryCast(", 1)[0]
        self.assertIn('BindAction(TEXT("Jump"), IE_Pressed, this, &AMoonfallHeroPawn::TryJump);', controls)
        self.assertIn('BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);', controls)
        self.assertIn("void TryJump();", self.hero_h)
        jump = controls.split("void AMoonfallHeroPawn::TryJump()", 1)[1].split(
            "void AMoonfallHeroPawn::MoveForward", 1)[0]
        self.assertIn("if (Health <= 0.f || bOptionsOpen || bInventoryOpen) return;", jump)
        self.assertLess(jump.index("if (Health <= 0.f || bOptionsOpen || bInventoryOpen) return;"),
                        jump.index("Jump();"))
        for axis in ("MoveForward", "MoveRight"):
            body = controls.split(f"void AMoonfallHeroPawn::{axis}(float Value)", 1)[1].split(
                "}", 1)[0]
            self.assertIn("Health <= 0.f || bOptionsOpen || bInventoryOpen || !FMath::IsFinite(Value)", body)
        for axis, call in (("Turn", "AddControllerYawInput"), ("LookUp", "AddControllerPitchInput")):
            self.assertIn(f"void AMoonfallHeroPawn::{axis}(float Value) {{ if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && FMath::IsFinite(Value)) {call}(Value); }}", controls)

    def test_menu_entry_and_defeat_cancel_lingering_jumps_and_velocity(self):
        inventory = self.hero_cpp.split("void AMoonfallHeroPawn::ToggleInventory()", 1)[1].split(
            "void AMoonfallHeroPawn::EquipSelectedInventoryItem()", 1)[0]
        options = self.hero_cpp.split("void AMoonfallHeroPawn::OpenOptions()", 1)[1].split(
            "void AMoonfallHeroPawn::OptionsUp()", 1)[0]
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        defeat = damage.split("if (Health <= 0.f) {", 1)[1].split("if (!bReduceMotion)", 1)[0]
        for body in (inventory, options, defeat):
            self.assertIn("StopJumping();", body)
            self.assertIn("GetCharacterMovement()->StopMovementImmediately();", body)
            self.assertLess(body.index("StopJumping();"), body.index("EndSprint();"))

    def test_perfect_dodge_rewards_reject_nonfinite_resources_and_tuning(self):
        reward = self.hero_cpp.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge()", 1)[1].split(
            "bool AMoonfallHeroPawn::ApplyMoonfallDamageFrom(", 1)[0]
        self.assertIn("FMath::IsFinite(MaxMana) && MaxMana > 0.f && FMath::IsFinite(Mana)", reward)
        self.assertIn("Mana = FMath::Clamp(Mana + 8.f, 0.f, MaxMana);", reward)
        self.assertIn("FMath::IsFinite(MaxStamina) && MaxStamina > 0.f && FMath::IsFinite(Stamina)", reward)
        self.assertIn("FMath::IsFinite(PerfectDodgeStaminaReward) && PerfectDodgeStaminaReward > 0.f", reward)
        self.assertIn("FMath::Clamp(PerfectDodgeStaminaReward, 0.f, 25.f)", reward)
        self.assertLess(reward.index("LastPerfectDodgeRewardAt = Now;"),
                        reward.index("Mana = FMath::Clamp(Mana + 8.f"))

    def test_perfect_dodge_rewards_require_live_valid_window(self):
        reward = self.hero_cpp.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge()", 1)[1].split(
            "bool AMoonfallHeroPawn::ApplyMoonfallDamageFrom(", 1)[0]
        self.assertIn("Health <= 0.f || bOptionsOpen || bInventoryOpen", reward)
        for field in ("Now", "PerfectDodgeWindowUntil", "LastPerfectDodgeRewardAt"):
            self.assertIn(f"!FMath::IsFinite({field})", reward)
        self.assertIn("Now > PerfectDodgeWindowUntil || Now - LastPerfectDodgeRewardAt < 1.f", reward)
        self.assertLess(reward.index("bOptionsOpen || bInventoryOpen"), reward.index("LastPerfectDodgeRewardAt = Now;"))
        self.assertLess(reward.index("!FMath::IsFinite(PerfectDodgeWindowUntil)"),
                        reward.index("PerfectDodgeWindowUntil = -1.f;"))

    def test_dodge_rejects_invalid_timers_and_preserves_stamina_delay(self):
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        for value in ("Now", "NextDodgeAt", "DamageImmunityUntil"):
            self.assertIn(f"!FMath::IsFinite({value})", dodge)
            self.assertLess(dodge.index(f"!FMath::IsFinite({value})"),
                            dodge.index("Stamina = FMath::Clamp(Stamina - DodgeStaminaCost"))
        self.assertIn("Now < NextDodgeAt || Now < DamageImmunityUntil", dodge)
        self.assertIn("StaminaRegenStartsAt = FMath::Max(", dodge)
        self.assertIn("FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now", dodge)
        self.assertIn("Now + 0.8f);", dodge)
        self.assertLess(dodge.index("StaminaRegenStartsAt = FMath::Max("),
                        dodge.index("LaunchCharacter("))

    def test_guard_impact_preserves_stamina_delay_and_resets_stale_counter(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        guard = damage.split("if (bGuarding && bFrontAttack && !bOptionsOpen", 1)[1].split(
            "OnGuardImpact(", 1)[0]
        self.assertIn("StaminaRegenStartsAt = FMath::Max(", guard)
        self.assertIn("FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now", guard)
        self.assertLess(guard.index("StaminaRegenStartsAt = FMath::Max("),
                        guard.index("if (bPerfectParry) {"))
        parry = guard.split("if (bPerfectParry) {", 1)[1]
        self.assertIn("ParryCounterUntil = -1.f;", parry)
        self.assertIn("if (FMath::IsFinite(ParryCounterWindowSeconds) && ParryCounterWindowSeconds > 0.f)", parry)
        self.assertLess(parry.index("ParryCounterUntil = -1.f;"),
                        parry.index("ParryCounterUntil = Now + FMath::Clamp(ParryCounterWindowSeconds"))

    def test_combo_clears_in_inventory_and_rejects_invalid_target_or_timers(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        combo = tick.split("if (HitComboStage > 0 && GetWorld()) {", 1)[1].split(
            "// Bounded camera impulse", 1)[0]
        self.assertIn("bOptionsOpen || bInventoryOpen", combo)
        for check in ("!FMath::IsFinite(ComboTarget->Health)", "!FMath::IsFinite(ComboNow)",
                      "!FMath::IsFinite(LastSuccessfulHitAt)"):
            self.assertIn(check, combo)
        self.assertIn("HitComboStage = 0;", combo)
        self.assertIn("LastHitComboTarget.Reset();", combo)

    def test_camera_impulse_rejects_invalid_delta_and_clamps_zoom(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("if (!FMath::IsFinite(ImpactCameraOffset)) ImpactCameraOffset = 0.f;", tick)
        self.assertIn("const float SafeCameraDelta = FMath::IsFinite(DeltaSeconds)", tick)
        self.assertIn("FMath::Clamp(DeltaSeconds, 0.f, 0.1f)", tick)
        self.assertIn("FMath::Clamp(ImpactCameraOffset, 0.f, 24.f)", tick)
        self.assertIn("FMath::FInterpTo(", tick)
        self.assertLess(tick.index("if (!FMath::IsFinite(ImpactCameraOffset))"),
                        tick.index("CameraBoom->TargetArmLength"))

    def test_incoming_damage_rejects_invalid_hero_vitals_and_clock(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        for guard in ("Health <= 0.f", "!FMath::IsFinite(Health)", "!FMath::IsFinite(MaxHealth)",
                      "MaxHealth <= 0.f", "!FMath::IsFinite(Amount)", "!FMath::IsFinite(Now)",
                      "!FMath::IsFinite(DamageImmunityUntil)"):
            self.assertIn(guard, damage)
            self.assertLess(damage.index(guard), damage.index("const float PreviousHealth = Health;"))
        self.assertIn("GetWorld()->GetTimeSeconds() < DamageImmunityUntil", damage)
        self.assertLess(damage.index("const float PreviousHealth = Health;"),
                        damage.index("const float ArmoredDamage = Amount *"))

    def test_sprint_input_is_rejected_in_menus_or_after_defeat(self):
        begin = self.hero_cpp.split("void AMoonfallHeroPawn::BeginSprint()", 1)[1].split(
            "void AMoonfallHeroPawn::EndSprint()", 1)[0]
        self.assertIn("if (bOptionsOpen || bInventoryOpen || Health <= 0.f) return;", begin)
        self.assertLess(begin.index("if (bOptionsOpen || bInventoryOpen || Health <= 0.f) return;"),
                        begin.index("bSprintHeld = true;"))

    def test_sprint_drain_is_bounded_and_recovery_is_monotonic(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        sprint = tick.split("if (bCanSprint && Stamina > 0.f) {", 1)[1].split(
            "const bool bSprinting =", 1)[0]
        self.assertIn("FMath::IsFinite(SprintStaminaDrainPerSecond) && SprintStaminaDrainPerSecond > 0.f", tick)
        self.assertIn("FMath::Clamp(SprintStaminaDrainPerSecond, 0.f, 250.f) * StaminaDelta", sprint)
        self.assertIn("const float SprintRecoveryAt = GetWorld()->GetTimeSeconds() + 0.8f;", sprint)
        self.assertIn("FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : SprintRecoveryAt", sprint)
        self.assertIn("StaminaRegenStartsAt = FMath::Max(", sprint)
        self.assertLess(sprint.index("Stamina = FMath::Max("), sprint.index("StaminaRegenStartsAt = FMath::Max("))

    def test_stamina_sprint_has_keyboard_controller_and_exhaustion_gates(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="Sprint",Key=LeftShift', inputs)
        self.assertIn('ActionName="Sprint",Key=Gamepad_LeftThumbstick', inputs)
        self.assertIn('BindAction(TEXT("Sprint"), IE_Pressed', self.hero_cpp)
        self.assertIn('BindAction(TEXT("Sprint"), IE_Released', self.hero_cpp)
        self.assertIn("void AMoonfallHeroPawn::BeginSprint()", self.hero_cpp)
        self.assertIn("const bool bValidStamina = FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina)", self.hero_cpp)
        self.assertIn("FMath::IsFinite(SprintRestartStamina)", self.hero_cpp)
        self.assertIn("bValidStamina ? FMath::Clamp(SprintRestartStamina, 0.f, MaxStamina) : 0.f", self.hero_cpp)
        self.assertIn("!bValidStamina || Stamina < Required || Stamina <= 0.f", self.hero_cpp)
        self.assertIn("float SprintRestartStamina = 30.f;", self.hero_h)
        self.assertIn("void AMoonfallHeroPawn::EndSprint()", self.hero_cpp)
        self.assertIn("float SprintStaminaDrainPerSecond = 18.f;", self.hero_h)
        self.assertIn("float SprintSpeedMultiplier = 1.45f;", self.hero_h)
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("bSprintHeld && Stamina <= 0.f", tick)
        self.assertIn("Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina)", tick)
        self.assertIn("!FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina)", tick)
        self.assertIn("!bSprintExhausted && !bGuarding && !bOptionsOpen && !bInventoryOpen && Health > 0.f", tick)
        self.assertIn("GetLastMovementInputVector().SizeSquared2D() > 0.01f", tick)
        self.assertIn("GetTimeSeconds() >= DamageImmunityUntil", tick)
        self.assertIn("if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", self.hero_cpp)
        self.assertIn("GetVelocity().SizeSquared2D()", tick)
        self.assertIn("Stamina = FMath::Max(0.f, Stamina - FMath::Clamp(SprintStaminaDrainPerSecond, 0.f, 250.f)", tick)
        self.assertIn("bSprintExhausted = true;", tick)
        self.assertIn("!bSprinting && !bGuarding && GetWorld()", tick)
        self.assertIn("FMath::IsFinite(StaminaRegenPerSecond) && StaminaRegenPerSecond > 0.f", tick)
        self.assertIn("Stamina + FMath::Clamp(StaminaRegenPerSecond, 0.f, 250.f) * StaminaDelta * MenuRecoveryBoost", tick)
        self.assertIn("FMath::IsFinite(SprintSpeedMultiplier)", tick)
        self.assertIn("float SprintAccelerationRate = 9.f;", self.hero_h)
        self.assertIn("NormalWalkSpeed * FMath::Clamp(SprintSpeedMultiplier", tick)
        self.assertIn("FMath::FInterpTo(", tick)
        self.assertIn("FMath::Clamp(SprintAccelerationRate, 1.f, 20.f)", tick)
        self.assertIn("GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", tick)
        self.assertLess(tick.index("if (bSprinting) {"), tick.index("} else if (bGuarding && !bOptionsOpen && Health > 0.f) {"))
        self.assertIn("float GuardMoveSpeedMultiplier = 0.65f;", self.hero_h)
        self.assertIn("FMath::IsFinite(GuardMoveSpeedMultiplier)", tick)
        self.assertIn("NormalWalkSpeed * GuardMultiplier", tick)
        self.assertIn("GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", tick)
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        self.assertIn("bSprintExhausted = true;", dodge)

    def test_stamina_cost_and_regeneration_are_bounded(self):
        self.assertIn("float Stamina = 100.f;", self.hero_h)
        self.assertIn("float MaxStamina = 100.f;", self.hero_h)
        self.assertIn("float DodgeStaminaCost = 25.f;", self.hero_h)
        self.assertIn("float StaminaRegenPerSecond = 24.f;", self.hero_h)
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        self.assertIn("Stamina < DodgeStaminaCost", dodge)
        self.assertIn("Stamina = FMath::Clamp(Stamina - DodgeStaminaCost", dodge)
        self.assertIn("StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now, Now + 0.8f);", dodge)
        self.assertLess(dodge.index("Stamina < DodgeStaminaCost"), dodge.index("LaunchCharacter("))
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("GetTimeSeconds() >= StaminaRegenStartsAt", tick)
        self.assertIn("FMath::Clamp(Stamina + FMath::Clamp(StaminaRegenPerSecond, 0.f, 250.f) * StaminaDelta * MenuRecoveryBoost, 0.f, MaxStamina)", tick)
        self.assertIn("Health > 0.f", tick)

    def test_dodge_cooldown_is_bounded_and_never_overlaps_immunity(self):
        self.assertIn('ClampMin="0.24", ClampMax="5.0"', self.hero_h)
        self.assertIn("float DodgeCooldown = 1.3f;", self.hero_h)
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        self.assertIn("Now < NextDodgeAt || Now < DamageImmunityUntil", dodge)
        self.assertIn("!FMath::IsFinite(DodgeCooldown) || DodgeCooldown <= 0.f", dodge)
        self.assertIn("NextDodgeAt = Now + FMath::Clamp(DodgeCooldown, 0.24f, 5.f);", dodge)
        self.assertLess(dodge.index("!FMath::IsFinite(DodgeCooldown)"),
                        dodge.index("Stamina = FMath::Clamp(Stamina - DodgeStaminaCost"))
        self.assertLess(dodge.index("NextDodgeAt = Now + FMath::Clamp(DodgeCooldown"),
                        dodge.index("DamageImmunityUntil = Now + 0.24f"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_dodge_prefers_live_movement_direction_and_bounds_launch_speed(self):
        self.assertIn("float DodgeLaunchSpeed = 650.f;", self.hero_h)
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        self.assertIn("!FMath::IsFinite(DodgeLaunchSpeed)", dodge)
        self.assertIn("GetPendingMovementInputVector().GetSafeNormal2D()", dodge)
        self.assertIn("GetLastMovementInputVector().GetSafeNormal2D()", dodge)
        self.assertIn("GetActorForwardVector().GetSafeNormal2D()", dodge)
        self.assertIn("!PendingDirection.IsNearlyZero() ? PendingDirection", dodge)
        self.assertIn("FMath::Clamp(DodgeLaunchSpeed, 250.f, 1200.f)", dodge)
        self.assertLess(dodge.index("!FMath::IsFinite(DodgeLaunchSpeed)"),
                        dodge.index("Stamina = FMath::Clamp(Stamina - DodgeStaminaCost"))
        self.assertLess(dodge.index("GetPendingMovementInputVector()"),
                        dodge.index("LaunchCharacter("))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_perfect_dodge_restores_bounded_stamina_once(self):
        self.assertIn("float PerfectDodgeStaminaReward = 10.f;", self.hero_h)
        self.assertIn("float DodgeStaminaCost = 25.f;", self.hero_h)
        self.assertEqual(self.hero_h.count("float DodgeStaminaCost = 25.f;"), 1)
        reward = self.hero_cpp.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge() {", 1)[1].split(
            "bool AMoonfallHeroPawn::ApplyMoonfallDamageFrom", 1)[0]
        self.assertIn("Now - LastPerfectDodgeRewardAt < 1.f", reward)
        self.assertIn("PerfectDodgeWindowUntil = -1.f;", reward)
        self.assertIn("Stamina = FMath::Clamp(Stamina + FMath::Clamp(PerfectDodgeStaminaReward, 0.f, 25.f), 0.f, MaxStamina);", reward)
        self.assertLess(reward.index("PerfectDodgeWindowUntil = -1.f;"),
                        reward.index("Stamina = FMath::Clamp"))

    def test_orb_displays_stamina_without_replacing_existing_resources(self):
        self.assertIn("Hero->Stamina / FMath::Max(1.f, Hero->MaxStamina)", self.hud)
        self.assertIn("DrawArc(Radius * 1.13f, DisplayStamina", self.hud)
        self.assertIn("Hero->Stamina < Hero->DodgeStaminaCost", self.hud)
        self.assertIn('TEXT("LOW STAMINA")', self.hud)
        self.assertIn('TEXT("SPRINT RECOVERING")', self.hud)
        self.assertIn("Hero->SprintRestartStamina", self.hud)
        self.assertIn("DisplayStamina = FMath::FInterpTo(DisplayStamina, ActualStamina", self.hud)
        self.assertIn("DrawArc(Radius * 0.85f, ManaFraction", self.hud)
        self.assertIn("DrawArc(Radius * 0.97f, XpFraction", self.hud)
        self.assertIn("const float HealthFraction = DisplayHealth;", self.hud)

    def test_dodge_immunity_and_perfect_window_order(self):
        start = self.hero_cpp.index("void AMoonfallHeroPawn::Dodge()")
        end = self.hero_cpp.index("void AMoonfallHeroPawn::RewardPerfectDodge()", start)
        dodge_code = self.hero_cpp[start:end]
        self.assertIn("DamageImmunityUntil = Now + 0.24f", dodge_code)
        self.assertIn("PerfectDodgeWindowUntil = Now + 0.14f", dodge_code)
        self.assertIn("Now < NextDodgeAt", dodge_code)

    def test_invalid_damage_rejected_before_health_changes(self):
        player = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        enemy = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        for body in (player, enemy):
            self.assertIn("FMath::IsFinite(Amount)", body)
            self.assertIn("Amount <= 0.f", body)
            self.assertLess(body.index("FMath::IsFinite(Amount)"), body.index("Health = FMath::Max"))
        self.assertIn("const float ActualDamage = HealthBeforeHit - Health", enemy)

    def test_boss_is_configured_before_begin_play(self):
        arena = (ROOT / "MoonfallArena.cpp").read_text(encoding="utf-8")
        spawn = arena.index("BeginDeferredActorSpawnFromClass")
        boss = arena.index("Enemy->bBoss = true", spawn)
        health = arena.index("Enemy->Health *= 4.f", boss)
        finish = arena.index("FinishSpawningActor(Enemy, SpawnTransform)", health)
        self.assertLess(spawn, boss)
        self.assertLess(boss, health)
        self.assertLess(health, finish)
        begin_play = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay()", 1)[1].split("\n}", 1)[0]
        self.assertIn("MaxHealthAtSpawn = FMath::Max(1.f, Health)", begin_play)
        self.assertIn("BossMaxHealth = bBoss ? MaxHealthAtSpawn : 0.f", begin_play)

    def test_boss_resists_repeated_hit_stagger(self):
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        self.assertIn("if (!bBoss) HitReactionEndsAt = FMath::Max(HitReactionEndsAt, LastHitTime + 0.22f)", damage)
        self.assertIn("BossPhaseMessageUntil = Now + 1.5f", damage)
        self.assertIn("WindupEndsAt = -1.f", damage)
        self.assertIn("bComboSecondHitPending = false", damage)

    def test_healing_is_capped_and_updates_hud(self):
        heal = self.hero_cpp.split("if (School == 3) {", 2)[-1]
        self.assertIn("Health = FMath::Min(MaxHealth, Health +", heal)
        self.assertIn("OnHealthChanged(Health, MaxHealth)", heal)

    def test_spell_cast_cost_and_cooldown_contract(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("Mana < Costs[School] * Multiplier", cast)
        self.assertIn("Time - LastCastTime[Hand] < Cooldowns[School]", cast)
        self.assertIn("Mana -= Costs[School] * Multiplier", cast)
        self.assertIn("LastCastTime[Hand] = Time", cast)
        self.assertLess(cast.index("Mana < Costs[School] * Multiplier"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_xp_and_loot_only_on_enemy_death(self):
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        death = damage.index("if (Health <= 0.0f) {")
        self.assertGreater(damage.index("Hero->AwardExperience", death), death)
        self.assertGreater(damage.index("Hero->SpawnGroundEquipmentDrop", death), death)

    def test_boss_phase_thresholds_are_ordered(self):
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        self.assertIn("HealthRatio <= 0.33f ? 3", damage)
        self.assertIn("HealthRatio <= 0.66f ? 2 : 1", damage)
        self.assertIn("if (NewPhase > BossPhase)", damage)

    def test_boss_phase_stops_pending_combo(self):
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, Now + 1.5f)", damage)
        self.assertIn("bComboSecondHitPending = false", damage)
        self.assertIn("bComboSecondHitActive = false", damage)

    def test_enemy_ai_uses_distance_based_tick_intervals(self):
        self.assertIn("bInCombatArea ? 0.1f : 0.4f", self.enemy_cpp)
        self.assertIn("SetActorTickInterval(DesiredTickInterval)", self.enemy_cpp)

    def test_every_enemy_melee_can_reward_a_precise_dodge(self):
        impact = self.enemy_cpp.split("if (!bBlocked) {", 1)[1].split(
            "const bool bParried =", 1)[0]
        self.assertIn("Hero->TryRewardPerfectDodge();", impact)
        self.assertNotIn("if (bBoss) Hero->TryRewardPerfectDodge();", impact)
        reward = self.hero_cpp.split(
            "bool AMoonfallHeroPawn::TryRewardPerfectDodge() {", 1)[1].split(
            "\\n}", 1)[0]
        self.assertIn("Now > PerfectDodgeWindowUntil", reward)
        self.assertIn("Now - LastPerfectDodgeRewardAt < 1.f", reward)
        self.assertIn("PerfectDodgeWindowUntil = -1.f", reward)

    def test_perfect_dodge_pressures_only_attacker_once(self):
        self.assertIn("void RewardPerfectDodge();", self.hero_h)
        self.assertIn("bool TryRewardPerfectDodge();", self.hero_h)
        self.assertIn("TryRewardPerfectDodge();", self.hero_cpp.split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[1].split(
            "bool AMoonfallHeroPawn::TryRewardPerfectDodge()", 1)[0])
        reward = self.hero_cpp.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge() {", 1)[1].split(
            "bool AMoonfallHeroPawn::ApplyMoonfallDamageFrom", 1)[0]
        self.assertIn("Now - LastPerfectDodgeRewardAt < 1.f", reward)
        self.assertIn("return false;", reward)
        self.assertIn("return true;", reward)
        self.assertLess(reward.index("PerfectDodgeWindowUntil = -1.f;"), reward.index("return true;"))
        self.assertIn("float PerfectDodgePostureDamage = 18.f;", self.enemy_h)
        self.assertIn("float PerfectDodgeRecoverySeconds = 0.5f;", self.enemy_h)
        self.assertIn("void OnPerfectDodged(float PosturePressure);", self.enemy_h)
        strike = self.enemy_cpp.split("if (!bBlocked) {", 1)[1].split(
            "const bool bParried =", 1)[0]
        self.assertIn("const bool bPerfectDodge = Hero->TryRewardPerfectDodge();", strike)
        self.assertIn("if (bPerfectDodge) {", strike)
        self.assertIn("FMath::Clamp(PerfectDodgePostureDamage, 0.f, 100.f)", strike)
        self.assertIn("ApplyPostureDamage(DodgePressure);", strike)
        self.assertIn("OnPerfectDodged(DodgePressure);", strike)
        self.assertIn("FMath::IsFinite(PerfectDodgeRecoverySeconds)", strike)
        self.assertIn("FMath::Clamp(PerfectDodgeRecoverySeconds, 0.f, 2.f)", strike)
        self.assertIn("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + RecoverySeconds);", strike)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, HitReactionEndsAt + 0.35f);", strike)
        self.assertIn("bComboSecondHitPending = false;", strike)
        self.assertIn("bComboSecondHitActive = false;", strike)
        self.assertIn("BossWarningEndTime = -1.f;", strike)
        self.assertLess(strike.index("const bool bPerfectDodge = Hero->TryRewardPerfectDodge();"),
                        strike.index("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + RecoverySeconds);"))
        self.assertLess(strike.index("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + RecoverySeconds);"),
                        strike.index("OnPerfectDodged(DodgePressure);"))
        self.assertIn("} else if (!bPerfectDodge) {", self.enemy_cpp)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_boss_posture_adapts_with_bounded_growth_and_elite_finisher_ignores_brace(self):
        self.assertIn("float BossPostureGrowthPerBreak = 0.12f;", self.enemy_h)
        self.assertIn("float BossPostureGrowthCap = 1.75f;", self.enemy_h)
        self.assertIn("float BasePostureAtSpawn = 100.f;", self.enemy_h)
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay()", 1)[1].split(
            "void AMoonfallEnemy::ApplyPostureDamage", 1)[0]
        self.assertIn("BasePostureAtSpawn = MaxPosture;", begin)
        posture = self.enemy_cpp.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        self.assertIn("PostureBreakCount = FMath::Min(PostureBreakCount + 1, 100000);", posture)
        self.assertIn("if (bBoss && FMath::IsFinite(BasePostureAtSpawn) && BasePostureAtSpawn > 0.f)", posture)
        self.assertIn("FMath::Clamp(BossPostureGrowthPerBreak, 0.f, 0.5f)", posture)
        self.assertIn("FMath::Clamp(BossPostureGrowthCap, 1.f, 3.f)", posture)
        self.assertIn("FMath::Min(PostureBreakCount, 100)", posture)
        self.assertIn("MaxPosture = FMath::Min(10000.f, BasePostureAtSpawn", posture)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("bElite && Now < HitReactionEndsAt && !bShatterFinisher", damage)
        self.assertIn("const float EffectiveDamage = BracedDamage", damage)
        self.assertIn("bShatterFinisher ? FinisherMultiplier : (bPostureExposed ? ExposureMultiplier : 1.f)", damage)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_posture_shatter_finisher_boss_recovery_and_compact_hud(self):
        self.assertIn("float PostureFinisherDamageMultiplier = 1.8f;", self.enemy_h)
        self.assertIn("bool bPostureFinisherAvailable = false;", self.enemy_h)
        self.assertIn("float BossPostureBreakRecoverySeconds = 2.5f;", self.enemy_h)
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay()", 1)[1].split(
            "void AMoonfallEnemy::ApplyPostureDamage", 1)[0]
        self.assertIn("const float PostureMultiplier = bBoss ? 1.8f : (bElite ? 1.25f : 1.f);", begin)
        self.assertIn("Posture = 0.f;", begin)
        posture = self.enemy_cpp.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        self.assertIn("Now < NextPostureBreakAllowedAt", posture)
        self.assertIn("bPostureFinisherAvailable = true;", posture)
        self.assertIn("NextPostureBreakAllowedAt = PostureBrokenUntil", posture)
        self.assertIn("OnPostureBroken(VulnerabilitySeconds);", posture)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("bPostureExposed && bPostureFinisherAvailable", damage)
        self.assertIn("FMath::Clamp(PostureFinisherDamageMultiplier, 1.f, 4.f)", damage)
        self.assertIn("if (bShatterFinisher) bPostureFinisherAvailable = false;", damage)
        self.assertIn("bShatterFinisher ? FinisherMultiplier : (bPostureExposed ? ExposureMultiplier : 1.f)", damage)
        self.assertLess(damage.index("if (bShatterFinisher) bPostureFinisherAvailable = false;"),
                        damage.index("Health = FMath::Max(0.0f, Health -"))
        self.assertIn('TEXT("POSTURE SHATTERED")', self.hud)
        self.assertIn('TEXT("SHATTERED")', self.hud)
        self.assertIn("Boss->Posture / FMath::Max(1.f, Boss->MaxPosture)", self.hud)
        self.assertIn("PostureTarget->Posture /", self.hud)
        self.assertIn("PostureDistanceSq = FMath::Square(1200.f)", self.hud)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_inventory_menu_cancels_pending_finisher_visuals(self):
        update = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience", 1)[0]
        self.assertIn("if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bMissingFinisherVisuals) {", update)
        self.assertIn("WaitingEnemy->BeginFinisherDefeat(Pending.Style);", update)
        self.assertIn("PendingFinishers.Reset();", update)
        self.assertIn("if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && Age >= Duration", update)
        self.assertIn("if (Health <= 0.f || bOptionsOpen || bInventoryOpen || Age < 0.f || Age >= Duration) {", update)
        self.assertLess(update.index("PendingFinishers.Reset();"),
                        update.index("if (Health > 0.f && !bOptionsOpen && !bInventoryOpen"))

    def test_finisher_lifecycle_handles_menu_kills_and_missing_visuals(self):
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        update = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("if (bOptionsOpen || bInventoryOpen || !FinisherSlashA || !FinisherSlashB || !FinisherFlash) {", register)
        self.assertIn("if (IsValid(DefeatedEnemy)) DefeatedEnemy->BeginFinisherDefeat(ChainStep);", register)
        self.assertLess(register.index("FinisherManaRestore"), register.index("if (bOptionsOpen || bInventoryOpen || !FinisherSlashA"))
        self.assertLess(register.index("if (bOptionsOpen || bInventoryOpen || !FinisherSlashA"),
                        register.index("PendingFinishers.Num() > 0"))
        self.assertIn("const bool bMissingFinisherVisuals = !FinisherSlashA || !FinisherSlashB || !FinisherFlash;", update)
        self.assertIn("if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bMissingFinisherVisuals) {", update)
        self.assertLess(update.index("WaitingEnemy->BeginFinisherDefeat(Pending.Style);"),
                        update.index("PendingFinishers.Reset();"))
        self.assertLess(update.index("PendingFinishers.Reset();"),
                        update.index("if (bMissingFinisherVisuals) return;"))
        self.assertLess(update.index("if (bMissingFinisherVisuals) return;"),
                        update.index("FinisherSlashA->SetHiddenInGame(true);"))

    def test_finisher_cleanup_when_queue_overflows_or_is_cancelled(self):
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertIn("SupersededEnemy->BeginFinisherDefeat(PendingFinishers.Last().Style);", register)
        self.assertLess(register.index("SupersededEnemy->BeginFinisherDefeat("),
                        register.index("PendingFinishers[PendingFinishers.Num() - 1] = Pending;"))
        update = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("for (const FPendingFinisher& Pending : PendingFinishers)", update)
        self.assertIn("WaitingEnemy->BeginFinisherDefeat(Pending.Style);", update)
        self.assertLess(update.index("WaitingEnemy->BeginFinisherDefeat(Pending.Style);"),
                        update.index("PendingFinishers.Reset();"))
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::BeginFinisherDefeat(", 1)[0]
        self.assertLess(damage.index("SetLifeSpan(9.5f);"), damage.index("Hero->RegisterFinisherKill("))
        self.assertEqual(damage.count("SetLifeSpan(9.5f);"), 1)
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginFinisherDefeat(", 1)[1].split(
            "void AMoonfallEnemy::TickDefeatAnimation(", 1)[0]
        self.assertIn("SetLifeSpan(1.25f);", begin)

    def test_queued_finishers_start_enemy_death_reaction_on_playback(self):
        self.assertIn("TWeakObjectPtr<AMoonfallEnemy> Enemy;", self.hero_h)
        self.assertIn("void BeginFinisherDefeat(int32 Style);", self.enemy_h)
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertIn("Pending.Enemy = DefeatedEnemy;", register)
        self.assertIn("StartFinisherPresentation(ChainStep, bShatterKill, bBossKill, DefeatedLocation, DefeatedEnemy);", register)
        presentation = self.hero_cpp.split("void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[1].split(
            "void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[0]
        self.assertIn("if (IsValid(DefeatedEnemy)) DefeatedEnemy->BeginFinisherDefeat(ChainStep);", presentation)
        self.assertLess(presentation.index("BeginFinisherDefeat(ChainStep);"), presentation.index("NativeFinisherStartedAt = Now;"))
        update = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("Pending.Enemy.Get()", update)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TickDefeatAnimation(", 1)[0]
        self.assertIn("DefeatAnimationStartedAt = -100.f;", damage)
        self.assertIn("SetLifeSpan(9.5f);", damage)
        self.assertIn("Hero->RegisterFinisherKill(bLastHitWasShatter, bBoss, GetActorLocation(), this)", damage)
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginFinisherDefeat(int32 Style)", 1)[1].split(
            "void AMoonfallEnemy::TickDefeatAnimation(", 1)[0]
        self.assertIn("DefeatAnimationStyle = FMath::Clamp(Style, 1, 5);", begin)
        self.assertIn("DefeatAnimationStartedAt = GetWorld()->GetTimeSeconds();", begin)
        self.assertIn("SetLifeSpan(1.25f);", begin)
        self.assertIn("DefeatAnimationStartedAt < 0.f", self.enemy_cpp)
        self.assertNotIn("SHATTER KO", self.hud)

    def test_finishers_have_no_shockwave_or_area_posture_pulse(self):
        self.assertNotIn("FinisherShockwave", self.hero_h)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertNotIn("TActorIterator<AMoonfallEnemy>", register)
        self.assertNotIn("NearbyEnemy->ApplyPostureDamage", register)
        self.assertIn("FinisherMomentumUntil = Now +", register)
        self.assertNotIn("SHATTER KO", self.hud)

    def test_finisher_momentum_rewards_combat_resources_and_respects_damage(self):
        for definition in [
            "float FinisherStaminaRestore = 8.f;",
            "float FinisherManaRestore = 6.f;",
            "float FinisherMomentumDamageMultiplier = 1.15f;",
            "float FinisherMomentumDurationSeconds = 6.f;",
            "float FinisherMomentumUntil = -1.f;",
        ]:
            self.assertIn(definition, self.hero_h)
        combo = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertIn("const float KillRewardScale = bBossKill ? 2.f : (bShatterKill ? 1.5f : 1.f);", combo)
        self.assertIn("FMath::Clamp(FinisherStaminaRestore, 0.f, 25.f)", combo)
        self.assertIn("FMath::Clamp(FinisherManaRestore, 0.f, 25.f)", combo)
        self.assertIn("Stamina = FMath::Clamp(", combo)
        self.assertIn("Mana = FMath::Clamp(", combo)
        self.assertIn("FinisherChainCount >= 3", combo)
        self.assertIn("FinisherMomentumUntil = Now + FMath::Clamp(FinisherMomentumDurationSeconds, 1.f, 12.f);", combo)
        self.assertNotIn("Health = ", combo)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("Time <= FinisherMomentumUntil", cast)
        self.assertIn("FMath::Clamp(FinisherMomentumDamageMultiplier, 1.f, 1.5f)", cast)
        self.assertIn("* CounterMultiplier * MomentumBonus * HitComboMultiplier;", cast)
        self.assertIn("if (School == 3 && Health >= MaxHealth) return false;", cast)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("if (DamageTaken > 0.f) {", damage)
        self.assertIn("FinisherMomentumUntil = -1.f;", damage)
        self.assertLess(damage.index("const float DamageTaken = PreviousHealth - Health;"),
                        damage.index("FinisherMomentumUntil = -1.f;"))

    def test_long_finisher_chain_rotates_styles_and_resets_on_gap(self):
        self.assertIn("int32 FinisherSequenceCount = 0;", self.hero_h)
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertIn("Now >= LastFinisherKillAt && Now - LastFinisherKillAt <= Window", register)
        self.assertIn("FMath::Min(FinisherSequenceCount + 1, 1000000) : 1;", register)
        self.assertIn("FinisherChainCount = FMath::Min(FinisherSequenceCount, 5);", register)
        self.assertIn("const int32 ChainStep = (FinisherSequenceCount - 1) % 5 + 1;", register)
        self.assertIn("Pending.Target = DefeatedLocation.ContainsNaN()", register)
        self.assertIn("PendingFinishers.Num() < MaxPendingFinishers", register)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_finisher_queue_preserves_rapid_kill_choreography(self):
        self.assertIn("struct FPendingFinisher", self.hero_h)
        self.assertIn("static constexpr int32 MaxPendingFinishers = 8;", self.hero_h)
        self.assertIn("TArray<FPendingFinisher> PendingFinishers;", self.hero_h)
        register = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[0]
        self.assertIn("PendingFinishers.Num() > 0 || (Now >= NativeFinisherStartedAt", register)
        self.assertIn("Now - NativeFinisherStartedAt < NativeDuration", register)
        self.assertIn("PendingFinishers.Num() < MaxPendingFinishers", register)
        self.assertIn("PendingFinishers.Add(Pending);", register)
        self.assertIn("PendingFinishers[PendingFinishers.Num() - 1] = Pending;", register)
        self.assertIn("Pending.Target = DefeatedLocation.ContainsNaN()", register)
        self.assertIn("Pending.Style = ChainStep;", register)
        self.assertIn("return ChainStep;", register)
        presentation = self.hero_cpp.split("void AMoonfallHeroPawn::StartFinisherPresentation(", 1)[1].split(
            "void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[0]
        self.assertIn("NativeFinisherStartedAt = Now;", presentation)
        self.assertIn("OnFinisherChain(ChainStep, bShatterKill, bBossKill);", presentation)
        self.assertIn("AnimInstance->Montage_Play(Montage, 1.f);", presentation)
        update = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("const FPendingFinisher Pending = PendingFinishers[0];", update)
        self.assertIn("PendingFinishers.RemoveAt(0);", update)
        self.assertIn("StartFinisherPresentation(Pending.Style, Pending.bShatter, Pending.bBoss, Pending.Target, Pending.Enemy.Get());", update)
        self.assertIn("PendingFinishers.Reset();", update)
        self.assertLess(update.index("PendingFinishers.RemoveAt(0);"),
                        update.index("StartFinisherPresentation(Pending.Style"))
        self.assertNotIn("SHATTER KO", self.hud)

    def test_finisher_direction_is_locked_to_killed_target(self):
        self.assertIn("FVector NativeFinisherOrigin = FVector::ZeroVector;", self.hero_h)
        self.assertIn("FVector NativeFinisherForward = FVector::ForwardVector;", self.hero_h)
        self.assertIn("int32 NativeFinisherStyle = 1;", self.hero_h)
        self.assertIn("bool bNativeFinisherBoss = false;", self.hero_h)
        combo = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[0]
        self.assertIn("NativeFinisherStyle = ChainStep;", combo)
        self.assertIn("NativeFinisherForward = (NativeFinisherTarget - NativeFinisherOrigin).GetSafeNormal2D();", combo)
        native = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("const FVector Origin = NativeFinisherOrigin;", native)
        self.assertIn("const FVector Forward = NativeFinisherForward;", native)
        self.assertIn("FMath::Clamp(NativeFinisherStyle, 1, 5)", native)
        self.assertNotIn("(NativeFinisherTarget - Origin).GetSafeNormal2D()", native)
        self.assertNotIn("FMath::Clamp(FinisherChainCount, 1, 5)", native)

    def test_builtin_five_style_finisher_visuals_require_no_animation_assets(self):
        hero_ctor = self.hero_cpp.split("AMoonfallHeroPawn::AMoonfallHeroPawn()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginPlay()", 1)[0]
        self.assertIn('CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FinisherSlashA"))', hero_ctor)
        self.assertIn('CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FinisherSlashB"))', hero_ctor)
        self.assertIn('CreateDefaultSubobject<UPointLightComponent>(TEXT("FinisherFlash"))', hero_ctor)
        self.assertIn('/Engine/BasicShapes/Cube.Cube', hero_ctor)
        self.assertIn("ECollisionEnabled::NoCollision", hero_ctor)
        self.assertIn("UpdateNativeFinisher(DeltaSeconds);", self.hero_cpp)
        native = self.hero_cpp.split("void AMoonfallHeroPawn::UpdateNativeFinisher(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("const int32 Style = FMath::Clamp(NativeFinisherStyle, 1, 5);", native)
        self.assertIn("const FVector Origin = NativeFinisherOrigin;", native)
        self.assertIn("const FVector Forward = NativeFinisherForward;", native)
        self.assertIn("bNativeFinisherBoss ? 3.2f", native)
        self.assertIn("bNativeFinisherBoss ? 4800.f", native)
        self.assertIn("FinisherSlashA->SetWorldLocationAndRotation", native)
        self.assertIn("FinisherSlashB->SetWorldLocationAndRotation", native)
        self.assertIn("FinisherFlash->SetIntensity", native)
        self.assertIn("bReduceMotion ? 0.22f : 0.78f", native)
        self.assertIn("FinisherSlashA->SetHiddenInGame(bReduceMotion);", native)
        self.assertIn("OnFinisherChain(ChainStep, bShatterKill, bBossKill);", self.hero_cpp)
        self.assertIn("void TickDefeatAnimation(float DeltaSeconds);", self.enemy_h)
        defeat = self.enemy_cpp.split("void AMoonfallEnemy::TickDefeatAnimation(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[0]
        self.assertIn("DefeatAnimationStyle == 4 ? 900.f", defeat)
        self.assertIn("Body->SetRelativeRotation", defeat)
        self.assertIn("Body->SetRelativeScale3D", defeat)
        self.assertIn("SetActorLocation(DefeatAnimationOrigin + Offset, false);", defeat)
        self.assertIn("if (Health <= 0.f) {", self.enemy_cpp)
        self.assertIn("TickDefeatAnimation(DeltaSeconds);", self.enemy_cpp)
        self.assertNotIn("SetActorHiddenInGame(true);", self.enemy_cpp)
        self.assertNotIn("SHATTER KO", self.hud)
        self.assertNotIn("KO -%.0f", self.hud)

    def test_chainable_finisher_montages_and_no_ko_text(self):
        self.assertIn("TArray<TObjectPtr<UAnimMontage>> FinisherMontages;", self.hero_h)
        self.assertIn("float FinisherChainWindowSeconds = 4.f;", self.hero_h)
        self.assertIn("int32 FinisherChainCount = 0;", self.hero_h)
        self.assertIn("int32 RegisterFinisherKill(bool bShatterKill, bool bBossKill, FVector DefeatedLocation, AMoonfallEnemy* DefeatedEnemy);", self.hero_h)
        self.assertIn("void OnFinisherChain(int32 ChainStep, bool bShatterKill, bool bBossKill);", self.hero_h)
        combo = self.hero_cpp.split("int32 AMoonfallHeroPawn::RegisterFinisherKill(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertIn("FMath::Clamp(FinisherChainWindowSeconds, 0.5f, 10.f)", combo)
        self.assertIn("FMath::Min(FinisherSequenceCount + 1, 1000000) : 1", combo)
        self.assertIn("FinisherChainCount = FMath::Min(FinisherSequenceCount, 5);", combo)
        self.assertIn("const int32 ChainStep = (FinisherSequenceCount - 1) % 5 + 1;", combo)
        self.assertIn("NativeFinisherStartedAt = Now;", combo)
        self.assertIn("NativeFinisherTarget = DefeatedLocation.ContainsNaN()", combo)
        self.assertIn("NativeFinisherOrigin = GetActorLocation();", combo)
        self.assertIn("NativeFinisherForward = (NativeFinisherTarget - NativeFinisherOrigin).GetSafeNormal2D();", combo)
        self.assertIn("NativeFinisherStyle = ChainStep;", combo)
        self.assertIn("bNativeFinisherBoss = bBossKill;", combo)
        self.assertIn("!bReduceMotion && GetMesh() && FinisherMontages.Num() > 0", combo)
        self.assertIn("AnimInstance->Montage_Play(Montage, 1.f);", combo)
        self.assertIn("OnFinisherChain(ChainStep, bShatterKill, bBossKill);", combo)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("Hero->RegisterFinisherKill(bLastHitWasShatter, bBoss, GetActorLocation(), this)", damage)
        self.assertNotIn("SHATTER KO", self.hud)
        self.assertNotIn("KO -%.0f", self.hud)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_killing_hit_keeps_damage_text_and_defeat_rewards_one_shot(self):
        self.assertIn("bool bKillingBlow = false;", self.enemy_h)
        self.assertIn("void OnEnemyDefeated(bool bKillingBlowWasShatter);", self.enemy_h)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("if (!FMath::IsFinite(Amount) || Amount <= 0.f || Health <= 0.f) return;", damage)
        self.assertIn("Entry.bKillingBlow = Health <= 0.f;", damage)
        self.assertIn("OnEnemyDefeated(bLastHitWasShatter);", damage)
        self.assertIn("DefeatAnimationStartedAt = -100.f;", damage)
        self.assertIn("SetLifeSpan(9.5f);", damage)
        self.assertLess(damage.index("Entry.bKillingBlow = Health <= 0.f;"), damage.index("if (Health <= 0.0f) {"))
        self.assertLess(damage.index("SetActorEnableCollision(false);"), damage.index("OnEnemyDefeated(bLastHitWasShatter);"))
        self.assertLess(damage.index("OnEnemyDefeated(bLastHitWasShatter);"), damage.index("Hero->AwardExperience"))
        self.assertLess(damage.index("SetLifeSpan(9.5f);"), damage.index("Hero->RegisterFinisherKill("))
        self.assertNotIn('TEXT("SHATTER KO -%.0f")', self.hud)
        self.assertNotIn('TEXT("KO -%.0f")', self.hud)
        self.assertIn('FString::Printf(TEXT("-%.0f"), Hit.Amount)', self.hud)
        self.assertIn("Hero->RegisterFinisherKill(bLastHitWasShatter, bBoss, GetActorLocation(), this)", damage)
        self.assertIn("VisibleDamageNumbers < 32", self.hud)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_rapid_hits_keep_independent_bounded_damage_labels(self):
        self.assertIn("static constexpr int32 DamageHistorySize = 4;", self.enemy_h)
        self.assertIn("FRecentDamageHit DamageHistory[DamageHistorySize];", self.enemy_h)
        self.assertIn("int32 NextDamageHistorySlot = 0;", self.enemy_h)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("if (ActualDamage > 0.f && FMath::IsFinite(ActualDamage) && GetWorld())", damage)
        self.assertIn("FRecentDamageHit& Entry = DamageHistory[NextDamageHistorySlot];", damage)
        self.assertIn("Entry.Amount = ActualDamage;", damage)
        self.assertIn("Entry.Time = LastHitTime;", damage)
        self.assertIn("Entry.bShatter = bLastHitWasShatter;", damage)
        self.assertIn("NextDamageHistorySlot = (NextDamageHistorySlot + 1) % DamageHistorySize;", damage)
        self.assertIn("Slot < AMoonfallEnemy::DamageHistorySize && VisibleDamageNumbers < 32", self.hud)
        self.assertIn("const AMoonfallEnemy::FRecentDamageHit& Hit = Enemy->DamageHistory[Slot];", self.hud)
        self.assertIn("const float Age = GetWorld()->GetTimeSeconds() - Hit.Time;", self.hud)
        self.assertIn("!FMath::IsFinite(Hit.Amount) || Hit.Amount <= 0.f", self.hud)
        self.assertIn("Screen.X + HorizontalOffset", self.hud)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_shatter_finisher_has_distinct_floating_damage_feedback(self):
        self.assertIn("bool bLastHitWasShatter = false;", self.enemy_h)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("bLastHitWasShatter = bShatterFinisher && ActualDamage > 0.f;", damage)
        self.assertLess(damage.index("const float ActualDamage ="),
                        damage.index("bLastHitWasShatter = bShatterFinisher"))
        self.assertIn("const bool bShatter = Hit.bShatter;", self.hud)
        self.assertIn('FString::Printf(TEXT("SHATTER -%.0f"), Hit.Amount)', self.hud)
        self.assertIn('FString::Printf(TEXT("-%.0f"), Hit.Amount)', self.hud)
        self.assertIn("(bShatter ? 1.55f : 1.3f) * Hero->CombatTextScale", self.hud)
        self.assertIn("VisibleDamageNumbers < 32", self.hud)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_shatter_finisher_expires_and_phase_transition_preserves_stagger(self):
        self.assertIn("float PosturePressureRecoveryDelaySeconds = 3.f;", self.enemy_h)
        self.assertIn("int32 PostureBreakCount = 0;", self.enemy_h)
        self.assertIn("void OnShatterFinisher(float DamageDealt);", self.enemy_h)
        posture = self.enemy_cpp.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        self.assertIn("PostureBreakCount = FMath::Min(PostureBreakCount + 1, 100000);", posture)
        self.assertIn("FMath::Clamp(PosturePressureRecoveryDelaySeconds, 0.f, 5.f)", posture)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[0]
        self.assertIn("if (bShatterFinisher && ActualDamage > 0.f)", damage)
        self.assertIn("OnShatterFinisher(ActualDamage);", damage)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, Now + 1.5f);", damage)
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("if (bPostureFinisherAvailable && Now >= PostureBrokenUntil)", tick)
        self.assertIn("bPostureFinisherAvailable = false;", tick)
        self.assertLess(tick.index("if (bPostureFinisherAvailable && Now >= PostureBrokenUntil)"),
                        tick.index("const bool Staggered = Now < HitReactionEndsAt;"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_enemy_posture_break_pressure_and_vulnerability(self):
        self.assertIn("float MaxPosture = 100.f;", self.enemy_h)
        self.assertIn("float ParryPostureDamage = 40.f;", self.enemy_h)
        self.assertIn("float SpellPostureScale = 0.3f;", self.enemy_h)
        self.assertIn("float BrokenPostureDamageMultiplier = 1.35f;", self.enemy_h)
        self.assertIn("void OnPostureBroken(float VulnerabilitySeconds);", self.enemy_h)
        self.assertIn("void OnPostureChanged(float CurrentPosture, float MaximumPosture);", self.enemy_h)
        posture = self.enemy_cpp.split("void AMoonfallEnemy::ApplyPostureDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        self.assertIn("!FMath::IsFinite(Amount) || Amount <= 0.f", posture)
        self.assertIn("if (Now < PostureBrokenUntil || Now < NextPostureBreakAllowedAt) return;", posture)
        self.assertIn("Posture = FMath::Clamp(Posture + Amount, 0.f, MaxPosture);", posture)
        self.assertIn("PostureRecoveryStartsAt = Now + (FMath::IsFinite(PosturePressureRecoveryDelaySeconds)", posture)
        self.assertIn("const float VulnerabilitySeconds = bBoss ? 1.5f : (bElite ? 2.f : 2.5f);", posture)
        self.assertIn("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, PostureBrokenUntil);", posture)
        self.assertIn("WindupEndsAt = -1.f;", posture)
        self.assertIn("bComboSecondHitPending = false;", posture)
        self.assertIn("OnPostureBroken(VulnerabilitySeconds);", posture)
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(float Amount)", 1)[1].split(
            "void AMoonfallEnemy::Tick(", 1)[0]
        self.assertIn("Now < PostureBrokenUntil", damage)
        self.assertIn("bPostureExposed ? ExposureMultiplier : 1.f", damage)
        self.assertIn("ApplyPostureDamage(ActualDamage * FMath::Clamp(SpellPostureScale, 0.f, 2.f))", damage)
        self.assertIn("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, LastHitTime + 0.22f);", damage)
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("Now >= PostureRecoveryStartsAt", tick)
        self.assertIn("OnPostureChanged(Posture, MaxPosture);", tick)
        parry = self.enemy_cpp.split("if (bParried) {", 1)[1].split("} else {", 1)[0]
        self.assertIn("ApplyPostureDamage(FMath::Max(0.f, ParryPostureDamage));", parry)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_perfect_parry_reaction_feedback_is_one_shot(self):
        self.assertIn("void OnPerfectParried(float StaggerSeconds);", self.enemy_h)
        self.assertIn("const float ParryStaggerSeconds = bBoss ? 0.45f : 0.75f;", self.enemy_cpp)
        self.assertIn("OnPerfectParried(ParryStaggerSeconds);", self.enemy_cpp)
        parry_branch = self.enemy_cpp.split("if (bParried) {", 1)[1].split("} else {", 1)[0]
        self.assertIn("OnPerfectParried(ParryStaggerSeconds);", parry_branch)
        self.assertIn("#if ENABLE_DRAW_DEBUG", parry_branch)
        self.assertIn("FColor::Cyan", parry_branch)

    def test_perfect_parry_staggers_attacker_and_breaks_combo(self):
        self.assertIn("bool ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation)", self.hero_h)
        self.assertIn("bLastIncomingHitParried = false;", self.hero_cpp)
        self.assertIn("bLastIncomingHitParried = bHasIncomingDamageSource;", self.hero_cpp)
        self.assertIn("return bLastIncomingHitParried;", self.hero_cpp)
        self.assertIn("const bool bParried = Hero->ApplyMoonfallDamageFrom(", self.enemy_cpp)
        self.assertIn("HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + ParryStaggerSeconds);", self.enemy_cpp)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, HitReactionEndsAt + 0.35f);", self.enemy_cpp)
        self.assertIn("bComboSecondHitPending = false;", self.enemy_cpp)
        self.assertIn("if (bParried)", self.enemy_cpp)

    def test_guard_only_catches_front_facing_enemy_strikes(self):
        self.assertIn("ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation)", self.hero_h)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", self.enemy_cpp)
        self.assertIn("GetActorLocation());", self.enemy_cpp)
        self.assertIn("const FVector GuardForward = GetActorForwardVector().GetSafeNormal2D();", self.hero_cpp)
        self.assertIn("const FVector ToSource = (IncomingDamageSource - GetActorLocation()).GetSafeNormal2D();", self.hero_cpp)
        self.assertIn("FVector::DotProduct(GuardForward, ToSource) >= 0.258819f", self.hero_cpp)
        self.assertIn("bGuarding && bFrontAttack && !bOptionsOpen", self.hero_cpp)
        self.assertIn("bHasIncomingDamageSource = false;", self.hero_cpp)

    def test_directional_guard_matches_character_facing_not_camera(self):
        self.assertIn("bOrientRotationToMovement = true", self.hero_cpp)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {", 1)[1]
        damage = damage.split("\\n}", 1)[0]
        self.assertIn("GetActorForwardVector().GetSafeNormal2D()", damage)
        self.assertNotIn("GetControlRotation().Vector().GetSafeNormal2D()", damage)
        self.assertIn("FVector::DotProduct(GuardForward, ToSource) >= 0.258819f", damage)

    def test_parry_timing_is_bounded_and_environment_cannot_trigger_counter(self):
        self.assertIn("float ParryCooldownSeconds = 0.8f;", self.hero_h)
        self.assertIn("float ParryWindowSeconds = 0.18f;", self.hero_h)
        guard = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("FMath::IsFinite(ParryWindowSeconds)", guard)
        self.assertIn("FMath::IsFinite(ParryCooldownSeconds)", guard)
        self.assertIn("FMath::Clamp(ParryWindowSeconds, 0.05f, 0.3f)", guard)
        self.assertIn("FMath::Clamp(ParryCooldownSeconds, 0.3f, 2.f)", guard)
        self.assertIn("ParryWindowEndsAt = -1.f;", guard)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("const bool bPerfectParry = bGuarding && bHasIncomingDamageSource && bFrontAttack", damage)
        self.assertIn("&& Now <= ParryWindowEndsAt && Now <= NextParryAt;", damage)
        self.assertIn("const bool bFrontAttack = !bHasIncomingDamageSource", damage)
        self.assertLess(damage.index("const bool bFrontAttack ="), damage.index("const bool bPerfectParry ="))
        self.assertIn("const float RequiredGuardCost = bPerfectParry ? 0.f : HitGuardCost;", damage)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_guard_and_parry_share_stamina_without_spending_spell_mana(self):
        self.assertIn("float GuardStaminaCost = 8.f;", self.hero_h)
        self.assertIn("float PerfectParryStaminaReward = 12.f;", self.hero_h)
        guard = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("!FMath::IsFinite(GuardStaminaCost)", guard)
        self.assertIn("Stamina < GuardStaminaCost", guard)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("Stamina >= RequiredGuardCost", damage)
        self.assertIn("const float RequiredGuardCost = bPerfectParry ? 0.f : HitGuardCost;", damage)
        self.assertIn("const bool bPerfectParry = bGuarding && bHasIncomingDamageSource && bFrontAttack", damage)
        self.assertIn("&& Now <= ParryWindowEndsAt && Now <= NextParryAt;", damage)
        self.assertIn("float GuardImpactStaminaScale = 0.15f;", self.hero_h)
        self.assertIn("FMath::IsFinite(GuardImpactStaminaScale)", damage)
        self.assertIn("ArmoredDamage * FMath::Min(GuardImpactStaminaScale, 1.f)", damage)
        self.assertIn("const float HitGuardCost =", damage)
        self.assertLess(damage.index("const float HitGuardCost ="), damage.index("const float RequiredGuardCost ="))
        self.assertLess(damage.index("Stamina >= RequiredGuardCost"), damage.index("Stamina = FMath::Clamp(Stamina - RequiredGuardCost"))
        self.assertLess(damage.index("const bool bPerfectParry"), damage.index("Stamina >= RequiredGuardCost"))
        self.assertIn("Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina)", damage)
        self.assertIn("StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now, Now + 0.8f);", damage)
        self.assertIn("FMath::IsFinite(PerfectParryStaminaReward)", damage)
        self.assertIn("Stamina + PerfectParryStaminaReward", damage)
        self.assertIn("if (Stamina < GuardStaminaCost) BreakGuard();", damage)
        self.assertIn("Insufficient stamina breaks guard", damage)
        self.assertNotIn("Mana -=", damage)
        self.assertNotIn("Mana = FMath::Min", damage)

    def test_blueprint_walk_speed_is_baseline_for_guard_and_sprint(self):
        begin_play = self.hero_cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[0]
        self.assertIn("Super::BeginPlay();", begin_play)
        self.assertIn("const float AuthoredWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;", begin_play)
        self.assertIn("FMath::IsFinite(AuthoredWalkSpeed) && AuthoredWalkSpeed > 0.f", begin_play)
        self.assertIn("NormalWalkSpeed = FMath::IsFinite(AuthoredWalkSpeed)", begin_play)
        self.assertIn("GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", begin_play)
        self.assertLess(begin_play.index("Super::BeginPlay();"), begin_play.index("AuthoredWalkSpeed"))
        self.assertNotIn("NormalWalkSpeed = 600.f;", begin_play)

    def test_guard_move_penalty_applies_and_cancels_immediately(self):
        start = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        end = self.hero_cpp.split("void AMoonfallHeroPawn::EndGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginSprint()", 1)[0]
        self.assertIn("FMath::Clamp(GuardMoveSpeedMultiplier, 0.2f, 1.f)", start)
        self.assertIn("GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed * GuardMultiplier;", start)
        self.assertIn("if (bWasGuarding && GetCharacterMovement())", end)
        self.assertIn("GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", end)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_perfect_parry_avoids_impact_stamina_cost(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("const float RequiredGuardCost = bPerfectParry ? 0.f : HitGuardCost;", damage)
        self.assertIn("Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina);", damage)
        self.assertIn("if (bPerfectParry) {", damage)
        self.assertIn("Stamina + PerfectParryStaminaReward", damage)
        self.assertIn("const float HitGuardCost =", damage)
        self.assertIn("if (Stamina < GuardStaminaCost) BreakGuard();", damage)
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("Stamina - GuardDrain * StaminaDelta", tick)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_guard_fatigue_drains_stamina_and_breaks_safely(self):
        self.assertIn("float GuardStaminaDrainPerSecond = 6.f;", self.hero_h)
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("bGuarding && !bOptionsOpen && Health > 0.f && GetWorld()", tick)
        self.assertIn("FMath::IsFinite(GuardStaminaDrainPerSecond)", tick)
        self.assertIn("!FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina)", tick)
        self.assertIn("|| MaxStamina <= 0.f || !FMath::IsFinite(GuardStaminaCost)", tick)
        self.assertIn("GuardStaminaCost < 0.f)) {", tick)
        self.assertLess(tick.index("GuardStaminaCost < 0.f)) {"), tick.index("Stamina - GuardDrain * StaminaDelta"))
        self.assertIn("BreakGuard();", tick)
        self.assertIn("FMath::Clamp(GuardStaminaDrainPerSecond, 0.f, 30.f)", tick)
        self.assertIn("Stamina - GuardDrain * StaminaDelta", tick)
        self.assertIn("StaminaRegenStartsAt = FMath::Max(", tick)
        self.assertIn("GuardStaminaCost > 0.f && Stamina < GuardStaminaCost", tick)
        self.assertIn("BreakGuard();", tick)
        self.assertLess(tick.index("Stamina - GuardDrain * StaminaDelta"),
                        tick.rindex("BreakGuard();"))
        self.assertLess(tick.index("BreakGuard();"), tick.index("if (GetCharacterMovement()) {"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_guard_stance_stops_stamina_regen_and_release_delays_it(self):
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("!bSprinting && !bGuarding && GetWorld()", tick)
        end_guard = self.hero_cpp.split("void AMoonfallHeroPawn::EndGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginSprint()", 1)[0]
        self.assertIn("const bool bWasGuarding = bGuarding;", end_guard)
        self.assertIn("if (bWasGuarding && GetWorld())", end_guard)
        self.assertIn("FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : GetWorld()->GetTimeSeconds(), GetWorld()->GetTimeSeconds() + 0.4f)", end_guard)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_twelve_hit_posture_milestones_only_affect_combo_target_once(self):
        self.assertIn("TWeakObjectPtr<AMoonfallEnemy> LastHitComboTarget;", self.hero_h)
        self.assertIn("float HitComboFourthHitPosture = 12.f;", self.hero_h)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("LastHitComboTarget.IsValid() && LastHitComboTarget.Get() == Enemy", cast)
        self.assertIn("const bool bComboAlive = bSameTarget &&", cast)
        self.assertIn("LastHitComboTarget = Enemy;", cast)
        self.assertIn("HitComboStage > IncomingStage", cast)
        self.assertIn("HitComboStage == 4 || HitComboStage == 8 || HitComboStage == 12", cast)
        self.assertIn("if (bNewPostureMilestone && Enemy->Health > 0.f", cast)
        self.assertIn("FMath::Clamp(HitComboFourthHitPosture, 0.f, 30.f)", cast)
        self.assertIn("Enemy->ApplyPostureDamage(", cast)
        self.assertLess(cast.index("if (ActualSpellDamage > 0.f) {"),
                        cast.index("HitComboStage == 4 || HitComboStage == 8 || HitComboStage == 12"))
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("LastHitComboTarget.Reset();", damage)
        self.assertNotIn("FinisherShockwave", self.hero_h)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_optional_twelve_slot_combo_animation_montages(self):
        self.assertIn("TArray<TObjectPtr<UAnimMontage>> ComboMontages;", self.hero_h)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("ComboMontages.IsValidIndex(HitComboStage - 1)", cast)
        self.assertIn("ComboMontages[HitComboStage - 1].Get()", cast)
        self.assertIn("Anim->Montage_Play(Montage, 1.f);", cast)
        self.assertIn("Enemy->Health > 0.f && !bReduceMotion", cast)
        self.assertLess(cast.index("const float HealthBeforeSpell = Enemy->Health;"),
                        cast.index("Enemy->TakeSpellDamage(SpellDamage);"))
        self.assertLess(cast.index("Enemy->TakeSpellDamage(SpellDamage);"),
                        cast.index("if (Enemy->Health > 0.f && !bReduceMotion"))
        self.assertLess(cast.index("if (ActualSpellDamage > 0.f) {"),
                        cast.index("Anim->Montage_Play(Montage, 1.f);"))
        self.assertLess(cast.index("Anim->Montage_Play(Montage, 1.f);"),
                        cast.index("OnSuccessfulHitCombo(HitComboStage, Hand == 0);"))

    def test_combo_milestone_reaction_cues_are_bounded(self):
        self.assertIn("void RegisterComboMilestone(int32 Stage);", self.enemy_h)
        self.assertIn("void OnComboMilestone(int32 Stage);", self.enemy_h)
        reaction = self.enemy_cpp.split("void AMoonfallEnemy::RegisterComboMilestone(int32 Stage)", 1)[1].split(
            "void AMoonfallEnemy::TakeSpellDamage", 1)[0]
        self.assertIn("Stage != 4 && Stage != 8 && Stage != 12", reaction)
        self.assertIn("Health <= 0.f", reaction)
        self.assertIn("if (!bBoss && Now >= PostureBrokenUntil)", reaction)
        self.assertIn("Stage == 12 ? 0.24f : (Stage == 8 ? 0.18f : 0.12f)", reaction)
        self.assertIn("FMath::IsFinite(HitReactionEndsAt)", reaction)
        self.assertIn("Now + FlinchSeconds", reaction)
        self.assertNotIn("Now + 0.5f", reaction)
        self.assertIn("OnComboMilestone(Stage);", reaction)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("if (bNewPostureMilestone && Enemy->Health > 0.f)", cast)
        self.assertIn("Enemy->RegisterComboMilestone(HitComboStage);", cast)

    def test_fourth_hit_has_full_damage_bonus_and_expiring_combo_clears(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("HitComboMultiplier = 1.f + ComboStepBonus * FMath::Clamp(EffectiveIncomingStage, 0, 3);", cast)
        self.assertNotIn("FMath::Max(0, IncomingStage - 1)", cast)
        self.assertIn("IncomingStage = bComboAlive ? FMath::Clamp(HitComboStage, 0, 12) : 0;", cast)
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("ComboNow - LastSuccessfulHitAt > ComboWindow", tick)
        self.assertIn("!IsValid(ComboTarget)", tick)
        self.assertIn("ComboTarget->Health <= 0.f", tick)
        self.assertIn("Health <= 0.f || bOptionsOpen", tick)
        self.assertIn("HitComboStage = 0;", tick)
        self.assertIn("LastHitComboTarget.Reset();", tick)
        self.assertIn("LastSuccessfulHitAt = -100.f;", tick)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_ordinary_enemy_attack_coordination_is_bounded_and_keeps_bosses_independent(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn('#include "EngineUtils.h"', self.enemy_cpp)
        self.assertIn("if (!bBoss && !bElite) {", tick)
        self.assertIn("ScannedActors < 64 && ConsideredAllies < 32", tick)
        self.assertIn("++ScannedActors;", tick)
        self.assertLess(tick.index("++ScannedActors;"), tick.index("if (!IsValid(Other)"))
        self.assertIn("ConsideredAllies < 32", tick)
        self.assertIn("Other->WindupEndsAt > Now", tick)
        self.assertIn("FVector::DistSquared2D(", tick)
        self.assertIn("FMath::Square(550.f)", tick)
        self.assertIn("if (++NearbyWindingAllies >= 2) break;", tick)
        self.assertIn("NextAttackAt = Now + SlotDelay;", tick)
        self.assertIn("0.18f + 0.04f * static_cast<float>(GetUniqueID() % 4u)", tick)
        self.assertLess(tick.index("if (!bBoss && !bElite) {"),
                        tick.index("const int32 PatternCount ="))
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_wounded_elite_retreat_is_brief_cooldown_gated_and_collision_safe(self):
        self.assertIn("float TacticalRetreatUntil = -1.f;", self.enemy_h)
        self.assertIn("float NextTacticalRetreatAt = 0.f;", self.enemy_h)
        self.assertIn("bElite && !bBoss && ActualDamage > 0.f && Health > 0.f", self.enemy_cpp)
        self.assertIn("Health / MaxHealthAtSpawn <= 0.3f", self.enemy_cpp)
        self.assertIn("Now >= NextTacticalRetreatAt", self.enemy_cpp)
        self.assertIn("TacticalRetreatUntil = Now + 2.f;", self.enemy_cpp)
        self.assertIn("NextTacticalRetreatAt = Now + 8.f;", self.enemy_cpp)
        self.assertIn("TacticalReengageUntil = TacticalRetreatUntil + 1.5f;", self.enemy_cpp)
        self.assertIn("bRetreatFlankLeft = !bRetreatFlankLeft;", self.enemy_cpp)
        self.assertIn("Now >= TacticalRetreatUntil && Now < TacticalReengageUntil", self.enemy_cpp)
        self.assertIn("FlankSign = bRetreatFlankLeft ? -1.f : 1.f;", self.enemy_cpp)
        self.assertIn("const float ChaseMultiplier = bTacticalReengage ? 1.2f : 1.f;", self.enemy_cpp)
        self.assertIn("SafeChaseSpeed * ChaseMultiplier * SafeAIDelta", self.enemy_cpp)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, TacticalRetreatUntil + 0.25f);", self.enemy_cpp)
        self.assertIn("if (bElite && !bBoss && Now < TacticalRetreatUntil", self.enemy_cpp)
        self.assertIn("Now >= HitReactionEndsAt && Now >= PostureBrokenUntil", self.enemy_cpp)
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertLess(tick.index("if (bElite && !bBoss && Now < TacticalRetreatUntil"),
                        tick.index("&& Distance < 2200.f && !bDivineRangedCastReady)"))
        self.assertIn("const float RetreatStep = 200.f * SafeAIDelta;", self.enemy_cpp)
        self.assertIn("AddActorWorldOffset(RetreatDirection * RetreatStep, true, &RetreatHit);", self.enemy_cpp)
        self.assertEqual(self.enemy_cpp.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_elites_and_late_bosses_strafe_only_during_attack_recovery(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("&& (bElite || (bBoss && BossPhase >= 2))", tick)
        self.assertIn("WindupEndsAt < 0.f && Now < NextAttackAt", tick)
        self.assertIn("Now >= HitReactionEndsAt && Now >= PostureBrokenUntil", tick)
        self.assertIn("Distance > AttackRange * 0.45f && Distance > 1.f", tick)
        self.assertIn("const float StrafeSpeed = bBoss ? 95.f : 70.f;", tick)
        self.assertIn("StrafeSpeed * SafeAIDelta;", tick)
        self.assertIn("AddActorWorldOffset(StrafeStep, true, &StrafeHit);", tick)
        self.assertLess(tick.index("AddActorWorldOffset(StrafeStep, true, &StrafeHit);"),
                        tick.index("if ((bBoss || bCanSeeHero) && WindupEndsAt < 0.f && Now >= NextAttackAt) {"))
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_wall_obstacle_escape_is_swept_time_bounded_and_does_not_add_nav_solves(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("float NextObstacleSidestepAt = 0.f;", self.enemy_h)
        self.assertIn("int32 ObstacleSidestepCount = 0;", self.enemy_h)
        self.assertIn("if (Hit.bBlockingHit && Now >= NextObstacleSidestepAt", tick)
        self.assertIn("SafeAIDelta > 0.f && !Direction.IsNearlyZero()", tick)
        self.assertIn("NextObstacleSidestepAt = Now + 0.6f;", tick)
        self.assertIn("ObstacleSidestepCount++", tick)
        self.assertIn("120.f * SafeAIDelta", tick)
        self.assertIn("AddActorWorldOffset(EscapeStep, true, &EscapeHit);", tick)
        self.assertIn("if (Hit.bBlockingHit && Now >= NextBlockedRepathAt)", tick)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(tick.count("Nav->ProjectPointToNavigation("), 1)

    def test_lost_sight_investigation_repaths_once_and_stops_at_last_seen_location(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("const bool bWasVisible = bCanSeeHero;", tick)
        self.assertIn("if (bCanSeeHero != bWasVisible)", tick)
        self.assertIn("CachedWaypoint = FVector::ZeroVector;", tick)
        self.assertIn("NextPathRefreshAt = 0.f;", tick)
        self.assertIn("FVector::DistSquared2D(EnemyPosition, LastKnownHeroPosition) <= FMath::Square(85.f)", tick)
        self.assertIn("LastSawHeroAt = -100.f;", tick)
        self.assertLess(tick.index("LastSawHeroAt = -100.f;"), tick.index("if ((Distance > AttackRange || (!bBoss && !bCanSeeHero))"))
        self.assertEqual(tick.count("Nav->ProjectPointToNavigation("), 1)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_divine_callbacks_observe_cleared_active_state(self):
        cpp = self.enemy_cpp
        helper = cpp.split("void AMoonfallEnemy::CancelDivineSignature() {", 1)[1].split("\\n}", 1)[0]
        self.assertIn("if (!bDivineSignatureActive) return;", helper)
        self.assertIn("const FName InterruptedMove = ActiveDivineMoveId;", helper)
        self.assertLess(helper.index("bDivineSignatureActive = false;"),
                        helper.index("OnDivineAttackInterrupted(InterruptedMove);"))
        self.assertLess(helper.index("ActiveDivineMoveId = NAME_None;"),
                        helper.index("OnDivineAttackInterrupted(InterruptedMove);"))
        self.assertEqual(cpp.count("CancelDivineSignature();"), 6)
        impact = cpp.split("const FName ResolvedMove = ActiveDivineMoveId;", 1)[1]
        self.assertLess(impact.index("bDivineSignatureActive = false;"),
                        impact.index("OnDivineAttackResolved(ResolvedMove, ResolvedCenter, bWithinDivineMarker);"))
        self.assertLess(impact.index("ActiveDivineMoveId = NAME_None;"),
                        impact.index("OnDivineAttackResolved(ResolvedMove, ResolvedCenter, bWithinDivineMarker);"))

    def test_divine_death_cancels_pending_cast_before_defeat_event(self):
        damage = self.enemy_cpp.split("void AMoonfallEnemy::TakeSpellDamage(", 1)[1]
        start = damage.index("if (Health <= 0.0f) {")
        end = damage.index("OnEnemyDefeated(", start)
        death = damage[start:end]
        self.assertIn("CancelDivineSignature();", death)
        self.assertIn("CancelDivineSignature();", death)
        self.assertIn("bDivineSignatureActive = false;", self.enemy_cpp)
        self.assertIn("ActiveDivineMoveId = NAME_None;", self.enemy_cpp)
        self.assertIn("WindupEndsAt = -1.f;", death)
        self.assertIn("BossWarningEndTime = -1.f;", death)
        self.assertIn("bComboSecondHitPending = false;", death)
        self.assertIn("bComboSecondHitActive = false;", death)
        self.assertLess(death.index("CancelDivineSignature();"), death.index("SetActorEnableCollision(false)"))

    def test_divine_telegraph_event_observes_committed_cast_clock(self):
        enemy = self.enemy_cpp
        start = enemy.index("// Use the number of *resolved* signatures")
        end = enemy.index("OnDivineAttackTelegraphed(", start) + len("OnDivineAttackTelegraphed(") + 120
        cast = enemy[start:end]
        self.assertIn("WindupEndsAt = Now + CurrentWindup;", cast)
        self.assertIn("if (bBoss) BossWarningEndTime = WindupEndsAt;", cast)
        self.assertIn("if (bDivineSignatureActive) {", cast)
        self.assertIn("OnDivineAttackTelegraphed(ActiveDivineMoveId, DivineSignatureCenter, DivineSignatureRadius, CurrentWindup);", cast)
        self.assertLess(cast.index("WindupEndsAt = Now + CurrentWindup;"),
                        cast.index("OnDivineAttackTelegraphed("))
        self.assertLess(cast.index("BossWarningEndTime = WindupEndsAt;"),
                        cast.index("OnDivineAttackTelegraphed("))
        self.assertLess(cast.index("LockedAttackDirection = Difference.GetSafeNormal2D();"),
                        cast.index("OnDivineAttackTelegraphed("))

    def test_divine_animation_callbacks_and_interruption_cleanup(self):
        self.assertIn("void OnDivineAttackTelegraphed(FName MoveId, FVector TargetCenter, float Radius, float WarningSeconds);", self.enemy_h)
        self.assertIn("void OnDivineAttackResolved(FName MoveId, FVector TargetCenter, bool bHeroInsideMarker);", self.enemy_h)
        self.assertIn("void OnDivineAttackInterrupted(FName MoveId);", self.enemy_h)
        self.assertIn("FName ActiveDivineMoveId = NAME_None;", self.enemy_h)
        self.assertIn('TEXT("VoidCollapse")', self.enemy_cpp)
        self.assertIn('TEXT("VoidJudgment")', self.enemy_cpp)
        self.assertIn('TEXT("EclipseMark")', self.enemy_cpp)
        self.assertIn('TEXT("EclipsePulse")', self.enemy_cpp)
        self.assertIn("OnDivineAttackTelegraphed(ActiveDivineMoveId, DivineSignatureCenter, DivineSignatureRadius, CurrentWindup);", self.enemy_cpp)
        self.assertIn("OnDivineAttackResolved(ResolvedMove, ResolvedCenter, bWithinDivineMarker);", self.enemy_cpp)
        self.assertIn("OnDivineAttackInterrupted(InterruptedMove);", self.enemy_cpp)
        self.assertIn("bDivineSignatureActive = false;", self.enemy_cpp)
        self.assertIn("ActiveDivineMoveId = NAME_None;", self.enemy_cpp)
        self.assertLess(self.enemy_cpp.index("OnDivineAttackResolved("), self.enemy_cpp.index("if (bWithinDivineMarker || bWithinBasicAttack)"))

    def test_divine_ranged_cast_requires_throttled_clear_line_of_sight(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("float NextDivineCastSightAt = 0.f;", self.enemy_h)
        self.assertIn("bool bDivineCastLineClear = false;", self.enemy_h)
        self.assertIn("const bool bDivineRangedCastCandidate = bDivineSignatureDue", tick)
        self.assertIn("if (bDivineRangedCastCandidate && Now >= NextDivineCastSightAt)", tick)
        self.assertIn("NextDivineCastSightAt = Now + 0.5f;", tick)
        self.assertIn("SCENE_QUERY_STAT(MoonfallDivineCastSight)", tick)
        self.assertIn("DivineSightParams.AddIgnoredActor(this);", tick)
        self.assertIn("DivineSightHit.GetActor() == Hero;", tick)
        self.assertIn("const bool bDivineRangedCastReady = bDivineRangedCastCandidate && bDivineCastLineClear;", tick)
        self.assertLess(tick.index("bDivineRangedCastReady = bDivineRangedCastCandidate && bDivineCastLineClear;"),
                        tick.index("&& Distance < 2200.f && !bDivineRangedCastReady)"))
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(tick.count("Nav->ProjectPointToNavigation("), 1)

    def test_divine_phase_transitions_open_with_readable_signature(self):
        enemy = self.enemy_cpp
        self.assertIn("void OnDivinePhaseEntered(FName BossId, int32 NewPhase, float RecoverySeconds);", self.enemy_h)
        self.assertIn('const bool bDivinePhase = DivineBossId == FName(TEXT("z_void_reaper"))', enemy)
        self.assertIn('|| DivineBossId == FName(TEXT("nyra_eclipse_queen"));', enemy)
        self.assertIn("const int32 NewDivineInterval = BossPhase >= 3 ? 2 : (BossPhase == 2 ? 3 : 4);", enemy)
        self.assertIn("AttackSequence = bDivinePhase ? NewDivineInterval - 1 : 0;", enemy)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, Now + 1.5f);", enemy)
        self.assertIn("if (bDivinePhase) OnDivinePhaseEntered(DivineBossId, BossPhase, 1.5f);", enemy)
        phase = enemy.split("if (NewPhase > BossPhase) {", 1)[1].split("OnBossPhaseChanged(BossPhase);", 1)[0]
        self.assertLess(phase.index("CancelDivineSignature();"), phase.index("AttackSequence = bDivinePhase"))
        self.assertLess(phase.index("NextAttackAt = FMath::Max("), phase.index("AttackSequence = bDivinePhase"))
        self.assertIn("DivineSignatureCastCount = 0;", phase)
        self.assertIn("bDivineAlternateGeometry = false;", phase)
        self.assertIn("AttackSequence % DivineSignatureInterval == DivineSignatureInterval - 1;", enemy)
        self.assertIn("AttackSequence % DivineSignatureInterval == 0)", enemy)

    def test_divine_spell_frequency_escalates_by_phase_without_changing_warnings(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("const int32 DivineSignatureInterval = BossPhase >= 3 ? 2 : (BossPhase == 2 ? 3 : 4);", tick)
        self.assertIn("AttackSequence % DivineSignatureInterval == DivineSignatureInterval - 1;", tick)
        self.assertIn("AttackSequence % DivineSignatureInterval == 0)", tick)
        self.assertEqual(tick.count("const int32 DivineSignatureInterval"), 1)
        self.assertIn("CurrentWindup = BossPhase >= 3 ? 1.2f : 1.7f;", tick)
        self.assertIn("DivineSignatureCastCount % 2 == 1;", tick)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_divine_signatures_can_cast_at_range_without_ranged_melee(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("const bool bDivineSignatureDue = bBoss", tick)
        self.assertIn('DivineBossId == FName(TEXT("z_void_reaper"))', tick)
        self.assertIn('DivineBossId == FName(TEXT("nyra_eclipse_queen"))', tick)
        self.assertIn("AttackSequence % DivineSignatureInterval == DivineSignatureInterval - 1;", tick)
        self.assertIn("const bool bDivineRangedCastCandidate = bDivineSignatureDue", tick)
        self.assertIn("Distance > AttackRange && Distance <= 750.f;", tick)
        self.assertIn("&& Distance < 2200.f && !bDivineRangedCastReady)", tick)
        self.assertIn("} else if (Distance <= AttackRange || bDivineRangedCastReady) {", tick)
        self.assertIn("if (!bDivineRangedCastReady", tick)
        self.assertIn("if (bBoss && (bVoidReaper || bEclipseQueen) && AttackSequence % DivineSignatureInterval == 0)", tick)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(tick.count("Nav->ProjectPointToNavigation("), 1)

    def test_divine_alternation_only_advances_after_completed_cast(self):
        enemy = self.enemy_cpp
        impact = enemy.split("const FName ResolvedMove = ActiveDivineMoveId;", 1)[1]
        selection = enemy.split("// Use the number of *resolved* signatures", 1)[1]
        self.assertIn("DivineSignatureCastCount = FMath::Min(DivineSignatureCastCount + 1, 100000);", impact)
        self.assertLess(impact.index("DivineSignatureCastCount = FMath::Min("),
                        impact.index("OnDivineAttackResolved("))
        self.assertIn("DivineSignatureCastCount % 2 == 1;", selection)
        self.assertNotIn("++DivineSignatureCastCount;", selection)
        interruption = enemy.split("void AMoonfallEnemy::CancelDivineSignature() {", 1)[1].split("void AMoonfallEnemy::ApplyPostureDamage(", 1)[0]
        self.assertNotIn("DivineSignatureCastCount + 1", interruption)
        self.assertIn("DivineSignatureCastCount = 0;", enemy)

    def test_phase_three_divine_attacks_alternate_locked_mark_geometries(self):
        enemy = self.enemy_cpp
        self.assertIn("bool bDivineAlternateGeometry = false;", self.enemy_h)
        self.assertIn("int32 DivineSignatureCastCount = 0;", self.enemy_h)
        self.assertIn("DivineSignatureCastCount = FMath::Min(DivineSignatureCastCount + 1, 100000);", enemy)
        self.assertIn("BossPhase >= 3 && DivineSignatureCastCount % 2 == 1", enemy)
        self.assertIn("bVoidReaper ? GetActorLocation() : HeroPosition", enemy)
        self.assertIn("bVoidReaper ? HeroPosition : GetActorLocation()", enemy)
        self.assertIn("bVoidReaper ? 440.f : 350.f", enemy)
        self.assertIn("bVoidReaper ? 300.f : 410.f", enemy)
        self.assertIn('TEXT("VOID COLLAPSE")', enemy)
        self.assertIn('TEXT("ECLIPSE MARK")', enemy)
        self.assertIn("DivineSignatureCastCount = 0;", enemy)
        self.assertIn("bDivineAlternateGeometry = false;", enemy)
        self.assertIn("FVector::DistSquared2D(HeroPosition, DivineSignatureCenter)", enemy)
        self.assertEqual(enemy.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_divine_signature_attacks_are_unique_telegraphed_and_dodgeable(self):
        enemy = self.enemy_cpp
        self.assertIn("bool bDivineSignatureActive = false;", self.enemy_h)
        self.assertIn("FVector DivineSignatureCenter = FVector::ZeroVector;", self.enemy_h)
        self.assertIn("float DivineSignatureRadius = 0.f;", self.enemy_h)
        self.assertIn('const bool bVoidReaper = DivineBossId == FName(TEXT("z_void_reaper"));', enemy)
        self.assertIn('const bool bEclipseQueen = DivineBossId == FName(TEXT("nyra_eclipse_queen"));', enemy)
        self.assertIn("AttackSequence % DivineSignatureInterval == 0", enemy)
        self.assertIn("DivineSignatureCenter = bDivineAlternateGeometry", enemy)
        self.assertIn("DivineSignatureRadius = bDivineAlternateGeometry", enemy)
        self.assertIn("CurrentWindup = BossPhase >= 3 ? 1.2f : 1.7f;", enemy)
        self.assertIn("FVector::DistSquared2D(HeroPosition, DivineSignatureCenter)", enemy)
        self.assertIn("const bool bWithinBasicAttack = !bDivineSignatureActive", enemy)
        self.assertIn("if (bWithinDivineMarker || bWithinBasicAttack)", enemy)
        self.assertIn('TEXT("VOID JUDGMENT")', enemy)
        self.assertIn('TEXT("ECLIPSE PULSE")', enemy)
        self.assertIn("if (!bDivineSignatureActive && bComboFollowUp", enemy)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", enemy)
        self.assertIn("Hero->TryRewardPerfectDodge()", enemy)

    def test_divine_arena_presets_are_individual_and_use_static_cover(self):
        arena = (ROOT / "MoonfallArena.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallArena.h").read_text(encoding="utf-8")
        self.assertIn("FName FeaturedDivineBossId = NAME_None;", header)
        self.assertIn('FeaturedDivineBossId == FName(TEXT("z_void_reaper"))', arena)
        self.assertIn('FeaturedDivineBossId == FName(TEXT("nyra_eclipse_queen"))', arena)
        self.assertIn("const bool bSingleDivineEncounter = bZEncounter || bNyraEncounter;", arena)
        self.assertIn("if (bSingleDivineEncounter) {", arena)
        self.assertIn("const float CoverOffset = bZEncounter ? 850.f : 700.f;", arena)
        self.assertIn("const FVector CoverScale = bZEncounter ?", arena)
        self.assertIn("const bool bNamedGodSlot = bSingleDivineEncounter && i == Count - 1;", arena)
        self.assertIn("const bool bLegacyGodSlot = !bSingleDivineEncounter && Count >= 5", arena)
        self.assertIn("const bool bZ = bNamedGodSlot ? bZEncounter : i == Count - 2;", arena)
        self.assertIn("else if (!bSingleDivineEncounter && Count == 4", arena)
        self.assertEqual(arena.count("CreateBlock(FVector("), 9)
        self.assertNotIn("PrimaryActorTick.bCanEverTick = true", arena)
        self.assertLess(arena.index("Enemy->DivineBossId ="), arena.index("FinishSpawningActor(Enemy, SpawnTransform)"))

    def test_unreal_divine_bosses_are_distinct_nonplayable_world_actors(self):
        arena = (ROOT / "MoonfallArena.cpp").read_text(encoding="utf-8")
        self.assertIn("FName DivineBossId = NAME_None;", self.enemy_h)
        self.assertIn("FText DivineBossTitle;", self.enemy_h)
        self.assertIn("FName DivineBossRegion = NAME_None;", self.enemy_h)
        self.assertIn("!bSingleDivineEncounter && Count >= 5", arena)
        self.assertIn('TEXT("z_void_reaper")', arena)
        self.assertIn('TEXT("nyra_eclipse_queen")', arena)
        self.assertIn('TEXT("Z, Void Reaper")', arena)
        self.assertIn('TEXT("Nyra, Eclipse Queen")', arena)
        self.assertIn('TEXT("ShatteredRuins")', arena)
        self.assertIn('TEXT("EclipseCitadel")', arena)
        self.assertIn('TEXT("DivineWorldBoss")', arena)
        self.assertLess(arena.index("Enemy->bBoss = true;"),
                        arena.index("FinishSpawningActor(Enemy, SpawnTransform)"))
        # Role identities belong to boss actors, not the controllable pawn.
        self.assertNotIn("DivineBossId", self.hero_h)

    def test_enemy_sight_memory_does_not_track_through_walls(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("float NextSightCheckAt = 0.f;", self.enemy_h)
        self.assertIn("float LastSawHeroAt = -100.f;", self.enemy_h)
        self.assertIn("FVector LastKnownHeroPosition = FVector::ZeroVector;", self.enemy_h)
        self.assertIn("if (!bBoss && Now >= NextSightCheckAt)", tick)
        self.assertIn("NextSightCheckAt = Now + 0.45f", tick)
        self.assertIn("SCENE_QUERY_STAT(MoonfallEnemyPerception)", tick)
        self.assertIn("PerceptionParams.AddIgnoredActor(this);", tick)
        self.assertIn("PerceptionHit.GetActor() != Hero", tick)
        self.assertIn("LastKnownHeroPosition = HeroPosition;", tick)
        self.assertIn("Now - LastSawHeroAt <= 2.5f", tick)
        self.assertIn("if (!bHasRecentSight)", tick)
        self.assertIn("(!bBoss && !bCanSeeHero)", tick)
        self.assertIn("(!bBoss && !bCanSeeHero) ? LastKnownHeroPosition", tick)
        self.assertIn("if ((bBoss || bCanSeeHero) && WindupEndsAt < 0.f", tick)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)

    def test_enemy_formation_lanes_spread_pursuit_without_more_nav_lookups(self):
        tick = self.enemy_cpp.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
        self.assertIn("Distance > AttackRange * 2.f && Distance < 900.f", tick)
        self.assertIn("const uint32 Lane = GetUniqueID() % 4u;", tick)
        self.assertIn("FlankSign = Lane % 2u == 0u ? -1.f : 1.f;", tick)
        self.assertIn("FlankDistance = Lane >= 2u ? 105.f : 70.f;", tick)
        self.assertIn("if (BossPhase >= 2)", tick)
        self.assertIn("FlankDistance = 175.f;", tick)
        self.assertIn("FlankDistance = 125.f;", tick)
        self.assertIn("Target += Side.GetSafeNormal() * FlankDistance * FlankSign;", tick)
        self.assertEqual(tick.count("Nav->FindPathToLocationSynchronously("), 1)
        self.assertEqual(tick.count("Nav->ProjectPointToNavigation("), 1)
        self.assertIn("AddActorWorldOffset(Direction.GetSafeNormal()", tick)

    def test_boss_basic_attack_patterns_randomize_without_immediate_repeats(self):
        self.assertIn("int32 LastBossAttackPattern = -1;", self.enemy_h)
        self.assertIn("const int32 PatternCount = (bElite || (bBoss && BossPhase >= 2)) ? 4 : 3;", self.enemy_cpp)
        self.assertIn("int32 Pattern = AttackSequence++ % PatternCount;", self.enemy_cpp)
        self.assertIn("if (bBoss) {", self.enemy_cpp)
        self.assertIn("FMath::RandRange(0, PatternCount - 2)", self.enemy_cpp)
        self.assertIn("if (Pattern >= LastBossAttackPattern) ++Pattern;", self.enemy_cpp)
        self.assertIn("LastBossAttackPattern = Pattern;", self.enemy_cpp)
        self.assertIn("if (bBoss && BossPhase >= 2 && AttackSequence % 7 == 0)", self.enemy_cpp)
        self.assertIn("LockedAttackDirection = Difference.GetSafeNormal2D();", self.enemy_cpp)

    def test_twelve_stage_attack_combo_requires_real_hits_and_has_bounded_damage(self):
        self.assertIn("float HitComboWindowSeconds = 2.f;", self.hero_h)
        self.assertIn("float HitComboDamagePerStage = 0.04f;", self.hero_h)
        self.assertIn("int32 HitComboStage = 0;", self.hero_h)
        self.assertIn("float LastSuccessfulHitAt = -100.f;", self.hero_h)
        self.assertIn("void OnSuccessfulHitCombo(int32 Stage, bool bLeftHand);", self.hero_h)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("Time - LastSuccessfulHitAt <= ComboWindow", cast)
        self.assertIn("LastHitComboTarget.IsValid() && LastHitComboTarget.Get() == Enemy", cast)
        self.assertIn("const bool bComboAlive = bSameTarget &&", cast)
        self.assertIn("FMath::Clamp(HitComboWindowSeconds, 0.5f, 4.f)", cast)
        self.assertIn("FMath::Clamp(HitComboDamagePerStage, 0.f, 0.12f)", cast)
        self.assertIn("HitComboMultiplier = 1.f + ComboStepBonus", cast)
        self.assertIn("* CounterMultiplier * MomentumBonus * HitComboMultiplier;", cast)
        self.assertIn("if (ActualSpellDamage > 0.f) {", cast)
        self.assertIn("HitComboStage = bComboAlive ? (IncomingStage >= 12 ? 1 : IncomingStage + 1) : 1;", cast)
        self.assertIn("const int32 EffectiveIncomingStage = IncomingStage == 12 ? 0 : IncomingStage;", cast)
        self.assertIn("FMath::Clamp(EffectiveIncomingStage, 0, 3)", cast)
        self.assertIn("LastSuccessfulHitAt = Time;", cast)
        self.assertIn("OnSuccessfulHitCombo(HitComboStage, Hand == 0);", cast)
        self.assertLess(cast.index("const float ActualSpellDamage ="),
                        cast.index("OnSuccessfulHitCombo(HitComboStage, Hand == 0);"))
        self.assertLess(cast.index("if (School == 3 && Health >= MaxHealth) return false;"),
                        cast.index("const float HitComboMultiplier"))
        injury = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("if (DamageTaken > 0.f) {", injury)
        self.assertIn("HitComboStage = 0;", injury)
        self.assertIn("LastSuccessfulHitAt = -100.f;", injury)
        self.assertNotIn("FinisherShockwave", self.hero_cpp)

    def test_perfect_parry_counterspell_is_time_limited_and_consumed_on_hit(self):
        self.assertIn("float ParryCounterWindowSeconds = 2.f;", self.hero_h)
        self.assertIn("float ParryCounterDamageMultiplier = 1.25f;", self.hero_h)
        self.assertIn("float ParryCounterUntil = -1.f;", self.hero_h)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("ParryCounterUntil = Now + FMath::Clamp(ParryCounterWindowSeconds, 0.f, 5.f)", damage)
        self.assertIn("if (DamageTaken > 0.f) {", damage)
        self.assertLess(damage.index("const float DamageTaken = PreviousHealth - Health;"),
                        damage.index("if (DamageTaken > 0.f) {",))
        self.assertLess(damage.index("if (DamageTaken > 0.f) {",),
                        damage.index("if (Health <= 0.f) {\n  // Prevent lingering movement/jump input from carrying through defeat.\n  StopJumping();\n  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();\n  // Clear any held sprint so a later respawn cannot inherit it.\n  EndSprint();\n  EndGuard();"))
        break_guard = self.hero_cpp.split("void AMoonfallHeroPawn::BreakGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginGuard()", 1)[0]
        self.assertIn("ParryCounterUntil = -1.f;", break_guard)
        self.assertLess(break_guard.index("ParryCounterUntil = -1.f;"),
                        break_guard.index("OnGuardBroken();"))
        self.assertIn("FMath::IsFinite(ParryCounterWindowSeconds)", damage)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("Time <= ParryCounterUntil", cast)
        self.assertIn("FMath::Clamp(ParryCounterDamageMultiplier, 1.f, 2.f)", cast)
        self.assertIn("Enemy->TakeSpellDamage(SpellDamage);", cast)
        self.assertIn("* CounterMultiplier * MomentumBonus * HitComboMultiplier;", cast)
        self.assertIn("if (bCounterReady && ActualSpellDamage > 0.f)", cast)
        self.assertIn("ParryCounterUntil = -1.f;", cast)
        self.assertLess(cast.index("Enemy->TakeSpellDamage(SpellDamage);"),
                        cast.index("if (bCounterReady && ActualSpellDamage > 0.f)"))
        self.assertLess(cast.index("if (!IsValid(Enemy) || Enemy->Health <= 0.f) return false;"),
                        cast.index("const bool bCounterReady"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_spell_school_posture_weights_are_bounded_and_require_real_hits(self):
        self.assertIn("float OffensiveSpellPostureMultiplier = 1.f;", self.hero_h)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("SchoolPostureWeights[6] = {0.8f, 0.65f, 1.35f, 0.f, 1.15f, 0.9f}", cast)
        self.assertIn("FMath::Clamp(0.018f * WeaponTier, 0.f, 0.35f)", cast)
        self.assertIn("const float ActualSpellDamage = FMath::Clamp(HealthBeforeSpell - Enemy->Health, 0.f, HealthBeforeSpell);", cast)
        self.assertIn("ActualSpellDamage > 0.f && Enemy->Health > 0.f", cast)
        self.assertIn("FMath::IsFinite(OffensiveSpellPostureMultiplier)", cast)
        self.assertIn("FMath::Clamp(OffensiveSpellPostureMultiplier, 0.f, 3.f)", cast)
        self.assertIn("FMath::IsFinite(ExtraPosture) && ExtraPosture > 0.f", cast)
        self.assertIn("Enemy->ApplyPostureDamage(FMath::Min(ExtraPosture, 100.f));", cast)
        self.assertLess(cast.index("Enemy->TakeSpellDamage(SpellDamage);"),
                        cast.index("const float ActualSpellDamage ="))
        self.assertLess(cast.index("const float ActualSpellDamage ="),
                        cast.index("Enemy->ApplyPostureDamage(FMath::Min(ExtraPosture, 100.f));"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_counterspell_posture_pressure_requires_real_spell_damage(self):
        self.assertIn("float ParryCounterPostureDamage = 15.f;", self.hero_h)
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("const float SpellDamage =", cast)
        self.assertIn("!FMath::IsFinite(SpellDamage) || SpellDamage <= 0.f", cast)
        self.assertIn("const float HealthBeforeSpell = Enemy->Health;", cast)
        self.assertIn("Enemy->TakeSpellDamage(SpellDamage);", cast)
        self.assertIn("if (bCounterReady && ActualSpellDamage > 0.f)", cast)
        self.assertIn("Enemy->Health > 0.f && FMath::IsFinite(ParryCounterPostureDamage)", cast)
        self.assertIn("Enemy->ApplyPostureDamage(FMath::Clamp(ParryCounterPostureDamage, 0.f, 100.f));", cast)
        self.assertIn("ParryCounterUntil = -1.f;", cast)
        self.assertLess(cast.index("!FMath::IsFinite(SpellDamage)"), cast.index("Enemy->TakeSpellDamage(SpellDamage);"))
        self.assertLess(cast.index("Enemy->TakeSpellDamage(SpellDamage);"), cast.index("if (bCounterReady && ActualSpellDamage > 0.f)"))
        self.assertLess(cast.index("Enemy->ApplyPostureDamage("), cast.index("ParryCounterUntil = -1.f;"))
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_dodge_immunity_cannot_overlap_new_guard_parry(self):
        guard = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() < DamageImmunityUntil", guard)
        self.assertLess(guard.index("GetWorld()->GetTimeSeconds() < DamageImmunityUntil"),
                        guard.index("bGuarding = true;"))
        self.assertLess(guard.index("GetWorld()->GetTimeSeconds() < DamageImmunityUntil"),
                        guard.index("ParryWindowEndsAt = Now +"))
        dodge = self.hero_cpp.split("void AMoonfallHeroPawn::Dodge()", 1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()", 1)[0]
        self.assertIn("DamageImmunityUntil = Now + 0.24f;", dodge)
        self.assertIn("EndGuard();", dodge)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_guard_break_cooldown_without_hud_warning(self):
        self.assertIn("float GuardBrokenUntil = 0.f;", self.hero_h)
        self.assertIn("float GuardBreakRecoverySeconds = 1.0f;", self.hero_h)
        self.assertIn("void OnGuardBroken();", self.hero_h)
        self.assertIn("void AMoonfallHeroPawn::BreakGuard()", self.hero_cpp)
        guard_break = self.hero_cpp.split("void AMoonfallHeroPawn::BreakGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::BeginGuard()", 1)[0]
        self.assertIn("if (!bGuarding || !GetWorld()) return;", guard_break)
        self.assertIn("EndGuard();", guard_break)
        self.assertIn("FMath::IsFinite(GuardBreakRecoverySeconds)", guard_break)
        self.assertIn("FMath::Clamp(GuardBreakRecoverySeconds, 0.f, 3.f) : 1.f;", guard_break)
        self.assertIn("GuardBrokenUntil = GetWorld()->GetTimeSeconds() + RecoverySeconds;", guard_break)
        self.assertIn("OnGuardBroken();", guard_break)
        guard_start = self.hero_cpp.split("void AMoonfallHeroPawn::BeginGuard()", 1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() < GuardBrokenUntil", guard_start)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("if (Stamina < GuardStaminaCost) BreakGuard();", damage)
        self.assertIn("Insufficient stamina breaks guard without granting free damage reduction.", damage)
        self.assertIn("BreakGuard();", damage)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)
        self.assertNotIn("Hero->GuardBrokenUntil", self.hud)

    def test_guard_and_parry_are_bounded_and_input_driven(self):
        self.assertIn('BindAction(TEXT("Guard"), IE_Pressed', self.hero_cpp)
        self.assertIn('BindAction(TEXT("Guard"), IE_Released', self.hero_cpp)
        self.assertIn('void AMoonfallHeroPawn::BeginGuard()', self.hero_cpp)
        self.assertIn('void AMoonfallHeroPawn::EndGuard()', self.hero_cpp)
        self.assertIn("NextParryAt = Now + FMath::Clamp(ParryCooldownSeconds, 0.3f, 2.f);", self.hero_cpp)
        self.assertIn("Stamina < GuardStaminaCost) return;", self.hero_cpp)
        self.assertIn("if (Health <= 0.f) {\n  // Prevent lingering movement/jump input from carrying through defeat.\n  StopJumping();\n  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();\n  // Clear any held sprint so a later respawn cannot inherit it.\n  EndSprint();\n  EndGuard();\n  // Defeat clears every gameplay menu so the death/respawn screen stays visible.\n  bInventoryOpen = false;\n  bOptionsOpen = false;\n }", self.hero_cpp)
        self.assertIn("ParryWindowEndsAt = -1.f", self.hero_cpp)
        self.assertIn("DamageAfterGuard *= 0.4f", self.hero_cpp)
        self.assertIn("Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina)", self.hero_cpp)
        self.assertNotIn("Mana -= 4.f", self.hero_cpp)
        self.assertIn("if (Stamina < GuardStaminaCost) BreakGuard();", self.hero_cpp)
        self.assertIn("OnGuardImpact(ArmoredDamage - DamageAfterGuard, bPerfectParry)", self.hero_cpp)
        self.assertIn('void AMoonfallHeroPawn::OpenOptions() {', self.hero_cpp)

    def test_directional_melee_uses_locked_windup_facing(self):
        self.assertIn("FVector LockedAttackDirection = FVector::ZeroVector;", self.enemy_h)
        self.assertEqual(self.enemy_cpp.count("LockedAttackDirection = Difference.GetSafeNormal2D();"), 2)
        self.assertIn("const bool bSpinningSweep = bBoss && BossSpecialMove == 4;", self.enemy_cpp)
        self.assertIn("const bool bWithinAttackArc", self.enemy_cpp)
        self.assertIn("bWithinAttackArc && Difference.Size() <= AttackRange * CurrentAttackRangeMultiplier;", self.enemy_cpp)
        self.assertIn("if (bWithinDivineMarker || bWithinBasicAttack)", self.enemy_cpp)

    def test_enemy_attacks_cannot_go_through_cover(self):
        self.assertIn("SightParams.AddIgnoredActor(this)", self.enemy_cpp)
        self.assertIn("LineTraceSingleByChannel(", self.enemy_cpp)
        self.assertIn("if (!bBlocked)", self.enemy_cpp)

    def test_hero_damage_respects_dodge_immunity(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() < DamageImmunityUntil", damage)
        self.assertLess(damage.index("DamageImmunityUntil"), damage.index("Health = FMath::Max"))

    def test_player_and_enemy_health_are_clamped_to_zero(self):
        self.assertIn("Health = FMath::Max(0.f, Health - DamageAfterGuard)", self.hero_cpp)
        self.assertIn("const float ArmoredDamage = Amount *", self.hero_cpp)
        self.assertIn("Health = FMath::Max(0.0f, Health -", self.enemy_cpp)

    def test_enemy_level_scaling_is_spawn_only(self):
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay() {", 1)[1].split("\n}", 1)[0]
        self.assertIn("SpawnBalanceLevel = IsValid(Player)", begin)
        self.assertIn("Health = FMath::Clamp(BaseHealth, 1.f, 100000.f) * HPScale", begin)
        self.assertIn("AttackDamage = FMath::Clamp(BaseDamage, 1.f, 10000.f) * DamageScale", begin)

    def test_offensive_spell_requires_live_target(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("if (!IsValid(Enemy) || Enemy->Health <= 0.f) return false", cast)
        self.assertLess(cast.index("Enemy->Health <= 0.f"), cast.index("Enemy->TakeSpellDamage"))

    def test_healing_at_full_health_costs_no_mana(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("if (School == 3 && Health >= MaxHealth) return false", cast)
        self.assertLess(cast.index("Health >= MaxHealth"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_spell_multiplier_validated_before_cast(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("!FMath::IsFinite(Multiplier)", cast)
        self.assertIn("Multiplier <= 0.f", cast)
        self.assertLess(cast.index("!FMath::IsFinite(Multiplier)"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_hostile_spell_rejects_nonfinite_enemy_health_before_mutation(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        self.assertIn("if (!IsValid(Enemy) || Enemy->Health <= 0.f) return false;", cast)
        self.assertIn("!FMath::IsFinite(Enemy->Health)", cast)
        self.assertIn("!FMath::IsFinite(Enemy->MaxHealth)", cast)
        self.assertIn("Enemy->MaxHealth <= 0.f", cast)
        self.assertLess(cast.index("!FMath::IsFinite(Enemy->Health)"),
                        cast.index("Enemy->TakeSpellDamage(SpellDamage);"))
        self.assertLess(cast.index("!FMath::IsFinite(Enemy->Health)"),
                        cast.index("Mana -= Costs[School] * Multiplier;"))
        self.assertLess(cast.index("!FMath::IsFinite(Enemy->Health)"),
                        cast.index("HitComboStage = bComboAlive"))

    def test_spell_resource_values_validated(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("!FMath::IsFinite(Mana)", cast)
        self.assertIn("!FMath::IsFinite(SpellPowerMultiplier)", cast)
        self.assertIn("SpellPowerMultiplier <= 0.f", cast)
        self.assertLess(cast.index("!FMath::IsFinite(Mana)"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_dual_cast_checks_both_spell_indices(self):
        dual = self.hero_cpp.split("void AMoonfallHeroPawn::CastBoth()", 1)[1].split("\n}", 1)[0]
        self.assertIn("LeftSpell < 0 || LeftSpell >= 6", dual)
        self.assertIn("RightSpell < 0 || RightSpell >= 6", dual)
        self.assertIn("TryCast(LeftSpell, 0, 2.f)", dual)

    def test_spell_cast_validates_health_state(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        for guard in ("!FMath::IsFinite(Health)", "!FMath::IsFinite(MaxHealth)", "MaxHealth <= 0.f"):
            self.assertIn(guard, cast)
            self.assertLess(cast.index(guard), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_spell_cast_validates_maximum_mana(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("!FMath::IsFinite(MaxMana)", cast)
        self.assertIn("MaxMana <= 0.f", cast)
        self.assertLess(cast.index("MaxMana <= 0.f"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_spell_cast_rejects_overcap_health(self):
        cast = self.hero_cpp.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split("\n}", 1)[0]
        self.assertIn("Health > MaxHealth", cast)
        self.assertLess(cast.index("Health > MaxHealth"), cast.index("Mana -= Costs[School] * Multiplier"))

    def test_dodge_and_blocked_hit_checks_retained(self):
        self.assertIn("GetSafeNormal2D()", self.enemy_cpp)
        self.assertIn("LineTraceSingleByChannel", self.enemy_cpp)
        self.assertIn("ApplyMoonfallDamage", self.enemy_cpp)

if __name__ == "__main__":
    unittest.main()
