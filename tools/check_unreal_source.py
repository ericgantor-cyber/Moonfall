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
        self.assertIn('MAP = "/Game/Maps/MoonfallStart"', generator)
        self.assertIn("EditorAssetLibrary.does_asset_exist(MAP)", generator)
        self.assertIn("levels.new_level(MAP, is_partitioned_world=False)", generator)
        self.assertIn("unreal.PlayerStart", generator)
        self.assertIn("unreal.NavMeshBoundsVolume", generator)
        self.assertIn("levels.save_current_level()", generator)

    def test_windows_package_requires_project_startup_level(self):
        script = (ROOT.parents[1] / "Build" / "Build-Windows.ps1").read_text(encoding="utf-8")
        self.assertIn("MoonfallStart.umap", script)
        self.assertIn("GameDefaultMap", script)
        self.assertIn("if (-not $CompileOnly)", script)
        self.assertIn("Playable startup level missing:", script)
        self.assertIn("Set GameDefaultMap=/Game/Maps/MoonfallStart", script)
        self.assertIn("if ($PreflightOnly)", script)

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
        self.assertIn("if (!bBoss) HitReactionEndsAt = LastHitTime + 0.22f", damage)
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
        self.assertIn("NextAttackAt = Now + 1.5f", damage)
        self.assertIn("bComboSecondHitPending = false", damage)
        self.assertIn("bComboSecondHitActive = false", damage)

    def test_enemy_ai_uses_distance_based_tick_intervals(self):
        self.assertIn("bInCombatArea ? 0.1f : 0.4f", self.enemy_cpp)
        self.assertIn("SetActorTickInterval(DesiredTickInterval)", self.enemy_cpp)

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
        self.assertIn("HitReactionEndsAt = Now + ParryStaggerSeconds;", self.enemy_cpp)
        self.assertIn("NextAttackAt = FMath::Max(NextAttackAt, HitReactionEndsAt + 0.35f);", self.enemy_cpp)
        self.assertIn("bComboSecondHitPending = false;", self.enemy_cpp)
        self.assertIn("if (bParried)", self.enemy_cpp)

    def test_guard_only_catches_front_facing_enemy_strikes(self):
        self.assertIn("ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation)", self.hero_h)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", self.enemy_cpp)
        self.assertIn("GetActorLocation());", self.enemy_cpp)
        self.assertIn("const FVector GuardForward = GetControlRotation().Vector().GetSafeNormal2D();", self.hero_cpp)
        self.assertIn("const FVector ToSource = (IncomingDamageSource - GetActorLocation()).GetSafeNormal2D();", self.hero_cpp)
        self.assertIn("FVector::DotProduct(GuardForward, ToSource) >= 0.258819f", self.hero_cpp)
        self.assertIn("bGuarding && bFrontAttack && !bOptionsOpen", self.hero_cpp)
        self.assertIn("bHasIncomingDamageSource = false;", self.hero_cpp)

    def test_guard_and_parry_are_bounded_and_input_driven(self):
        self.assertIn('BindAction(TEXT("Guard"), IE_Pressed', self.hero_cpp)
        self.assertIn('BindAction(TEXT("Guard"), IE_Released', self.hero_cpp)
        self.assertIn('void AMoonfallHeroPawn::BeginGuard()', self.hero_cpp)
        self.assertIn('void AMoonfallHeroPawn::EndGuard()', self.hero_cpp)
        self.assertIn("NextParryAt = Now + 0.8f", self.hero_cpp)
        self.assertIn("Mana < 4.f || bGuarding) return;", self.hero_cpp)
        self.assertIn("if (Health <= 0.f) EndGuard();", self.hero_cpp)
        self.assertIn("ParryWindowEndsAt = -1.f", self.hero_cpp)
        self.assertIn("DamageAfterGuard *= 0.4f", self.hero_cpp)
        self.assertIn("Mana -= 4.f", self.hero_cpp)
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
