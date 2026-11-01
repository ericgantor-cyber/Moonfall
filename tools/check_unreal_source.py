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
        start = self.hero_cpp.index("void AMoonfallHeroPawn::ApplyMoonfallDamage")
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

    def test_enemy_attacks_cannot_go_through_cover(self):
        self.assertIn("SightParams.AddIgnoredActor(this)", self.enemy_cpp)
        self.assertIn("LineTraceSingleByChannel(", self.enemy_cpp)
        self.assertIn("if (!bBlocked)", self.enemy_cpp)

    def test_hero_damage_respects_dodge_immunity(self):
        damage = self.hero_cpp.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {", 1)[1].split("\n}", 1)[0]
        self.assertIn("GetWorld()->GetTimeSeconds() < DamageImmunityUntil", damage)
        self.assertLess(damage.index("DamageImmunityUntil"), damage.index("Health = FMath::Max"))

    def test_player_and_enemy_health_are_clamped_to_zero(self):
        self.assertIn("Health = FMath::Max(0.f, Health - Amount", self.hero_cpp)
        self.assertIn("Health = FMath::Max(0.0f, Health -", self.enemy_cpp)

    def test_enemy_level_scaling_is_spawn_only(self):
        begin = self.enemy_cpp.split("void AMoonfallEnemy::BeginPlay() {", 1)[1].split("\n}", 1)[0]
        self.assertIn("SpawnBalanceLevel = IsValid(Player)", begin)
        self.assertIn("Health = FMath::Max(1.f, Health * HPScale)", begin)
        self.assertIn("AttackDamage = FMath::Max(1.f, AttackDamage * DamageScale)", begin)

    def test_dodge_and_blocked_hit_checks_retained(self):
        self.assertIn("GetSafeNormal2D()", self.enemy_cpp)
        self.assertIn("LineTraceSingleByChannel", self.enemy_cpp)
        self.assertIn("ApplyMoonfallDamage", self.enemy_cpp)

if __name__ == "__main__":
    unittest.main()
