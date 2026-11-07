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
        self.assertIn("if (-not $CompileOnly)", script)
        self.assertIn("Playable startup level missing:", script)
        self.assertIn("foreach ($mapKey in @('GameDefaultMap', 'EditorStartupMap'))", script)
        self.assertIn("GlobalDefaultGameMode", script)
        self.assertIn("Get-Item -LiteralPath $startupMap", script)
        self.assertIn('"-map=/Game/Maps/MoonfallStart"', script)
        self.assertIn("BuildCookRun", script)
        self.assertIn("$matchingEntries.Count -ne 1", script)
        self.assertIn("$mapSectionMatches.Count -ne 1", script)
        self.assertIn("if ($PreflightOnly)", script)
        self.assertIn("if (-not $CompileOnly) {", script)
        self.assertNotIn("if (-not $CompileOnly -and -not $PreflightOnly)", script)
        self.assertIn("Get-Command $command", script)
        self.assertIn("makensis.exe", script)
        self.assertIn("dotnet --list-sdks", script)
        self.assertLess(script.index("if (-not $CompileOnly) {", script.index("# Packaging preflight")),
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
        self.assertIn("Hero->AwardEquipmentDrop", self.enemy_cpp)

    def test_equipment_stats_affect_combat(self):
        for name in ("WeaponTier", "ArmorTier", "AccessoryTier"):
            self.assertIn(name, self.hero_h)
            self.assertIn(name, self.hero_cpp)
        self.assertIn("InventoryItemCodes.Num() >= 30", self.hero_cpp)

    def test_damage_hud_uses_current_max_health(self):
        start = self.hero_cpp.index("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {")
        end = self.hero_cpp.index("void AMoonfallHeroPawn::", start + 5)
        damage_code = self.hero_cpp[start:end]
        self.assertIn("OnHealthChanged(Health, MaxHealth)", damage_code)
        self.assertNotIn("OnHealthChanged(Health, 100.f)", damage_code)

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
        self.assertIn("!bSprintExhausted && !bGuarding && !bOptionsOpen && Health > 0.f", tick)
        self.assertIn("GetLastMovementInputVector().SizeSquared2D() > 0.01f", tick)
        self.assertIn("GetTimeSeconds() >= DamageImmunityUntil", tick)
        self.assertIn("if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;", self.hero_cpp)
        self.assertIn("GetVelocity().SizeSquared2D()", tick)
        self.assertIn("Stamina = FMath::Max(0.f, Stamina - SprintStaminaDrainPerSecond", tick)
        self.assertIn("bSprintExhausted = true;", tick)
        self.assertIn("!bSprinting && !bGuarding && GetWorld()", tick)
        self.assertIn("FMath::IsFinite(StaminaRegenPerSecond) && StaminaRegenPerSecond > 0.f", tick)
        self.assertIn("Stamina + StaminaRegenPerSecond * StaminaDelta", tick)
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
        self.assertIn("StaminaRegenStartsAt = Now + 0.8f", dodge)
        self.assertLess(dodge.index("Stamina < DodgeStaminaCost"), dodge.index("LaunchCharacter("))
        tick = self.hero_cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent", 1)[0]
        self.assertIn("GetTimeSeconds() >= StaminaRegenStartsAt", tick)
        self.assertIn("FMath::Clamp(Stamina + StaminaRegenPerSecond * StaminaDelta, 0.f, MaxStamina)", tick)
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
        self.assertIn("Stamina = FMath::Clamp(Stamina + FMath::Max(0.f, PerfectDodgeStaminaReward), 0.f, MaxStamina);", reward)
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
        self.assertGreater(damage.index("Hero->AwardEquipmentDrop", death), death)

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
        self.assertIn("const bool bPerfectParry = bGuarding && Now <= ParryWindowEndsAt && Now <= NextParryAt;", damage)
        self.assertIn("float GuardImpactStaminaScale = 0.15f;", self.hero_h)
        self.assertIn("FMath::IsFinite(GuardImpactStaminaScale)", damage)
        self.assertIn("ArmoredDamage * FMath::Min(GuardImpactStaminaScale, 1.f)", damage)
        self.assertIn("const float HitGuardCost =", damage)
        self.assertLess(damage.index("const float HitGuardCost ="), damage.index("const float RequiredGuardCost ="))
        self.assertLess(damage.index("Stamina >= RequiredGuardCost"), damage.index("Stamina = FMath::Clamp(Stamina - RequiredGuardCost"))
        self.assertLess(damage.index("const bool bPerfectParry"), damage.index("Stamina >= RequiredGuardCost"))
        self.assertIn("Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina)", damage)
        self.assertIn("StaminaRegenStartsAt = Now + 0.8f", damage)
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
                        tick.index("BreakGuard();"))
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
        self.assertIn("FMath::Max(StaminaRegenStartsAt, GetWorld()->GetTimeSeconds() + 0.4f)", end_guard)
        self.assertNotIn('TEXT("GUARD BROKEN")', self.hud)

    def test_perfect_parry_counterspell_is_time_limited_and_consumed_on_hit(self):
        self.assertIn("float ParryCounterWindowSeconds = 2.f;", self.hero_h)
        self.assertIn("float ParryCounterDamageMultiplier = 1.25f;", self.hero_h)
        self.assertIn("float ParryCounterUntil = -1.f;", self.hero_h)
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)", 1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()", 1)[0]
        self.assertIn("ParryCounterUntil = Now + FMath::Clamp(ParryCounterWindowSeconds, 0.f, 5.f)", damage)
        self.assertIn("if (DamageTaken > 0.f) ParryCounterUntil = -1.f;", damage)
        self.assertLess(damage.index("const float DamageTaken = PreviousHealth - Health;"),
                        damage.index("if (DamageTaken > 0.f) ParryCounterUntil = -1.f;"))
        self.assertLess(damage.index("if (DamageTaken > 0.f) ParryCounterUntil = -1.f;"),
                        damage.index("if (Health <= 0.f) EndGuard();"))
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
        self.assertIn("Enemy->TakeSpellDamage(", cast)
        self.assertIn("* CounterMultiplier)", cast)
        self.assertIn("if (bCounterReady) ParryCounterUntil = -1.f;", cast)
        self.assertLess(cast.index("Enemy->TakeSpellDamage("),
                        cast.index("if (bCounterReady) ParryCounterUntil = -1.f;"))
        self.assertLess(cast.index("if (!IsValid(Enemy) || Enemy->Health <= 0.f) return false;"),
                        cast.index("const bool bCounterReady"))
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
        self.assertIn("NextParryAt = Now + 0.8f", self.hero_cpp)
        self.assertIn("Stamina < GuardStaminaCost) return;", self.hero_cpp)
        self.assertIn("if (Health <= 0.f) EndGuard();", self.hero_cpp)
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
        self.assertIn("if (bWithinAttackArc && Difference.Size() <= AttackRange * CurrentAttackRangeMultiplier)", self.enemy_cpp)

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
        self.assertIn("Health = FMath::Max(1.f, Health * HPScale)", begin)
        self.assertIn("AttackDamage = FMath::Max(1.f, AttackDamage * DamageScale)", begin)

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
