"""Static guardrails for the isolated Moonfall engine comparison.

These checks are NOT Unity or Unreal compilation or gameplay tests.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
UNITY = ROOT / "unity/Assets/Scripts/MoonfallPrototype.cs"
CPP = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.cpp"
HEADER = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.h"
SAVES = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallProgressSave.h"
INPUT = ROOT / "unreal/MoonfallPrototype/Config/DefaultInput.ini"
UPROJECT = ROOT / "unreal/MoonfallPrototype/MoonfallPrototype.uproject"


class PrototypeContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.unity = UNITY.read_text(encoding="utf-8")
        cls.cpp = CPP.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")

    def test_sources_exist(self):
        for path in (UNITY, CPP, HEADER, SAVES, INPUT, UPROJECT):
            with self.subTest(path=path):
                self.assertTrue(path.is_file(), f"Missing {path}")

    def test_attack_mapping_is_click_only(self):
        input_text = INPUT.read_text(encoding="utf-8")
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', input_text)
        self.assertNotIn('ActionName="Attack",Key=SpaceBar', input_text)
        self.assertIn("Input.GetMouseButtonDown(0)", self.unity)

    def test_dodge_has_recovery_not_permanent_immunity(self):
        self.assertIn("DodgeCooldown = 1.2f;", self.cpp)
        self.assertIn("DodgeCooldown <= 0.f", self.cpp)
        self.assertIn("DodgeCooldown = FMath::Max", self.cpp)
        self.assertIn("dodgeCooldown=1.2f;", self.unity)
        self.assertIn("dodgeCooldown<=0", self.unity)
        self.assertIn("dodgeCooldown = Mathf.Max", self.unity)

    def test_boss_timing_parity(self):
        # Unity still has the single-phase comparison baseline; Unreal now has three tiers.
        self.assertIn("BossAttackPhase == 2 ? 1.05f : 1.15f", self.cpp)
        self.assertIn("BossAttackPhase == 2 ? 2.9f : 3.5f", self.cpp)
        self.assertIn("castTimer=1.15f;", self.unity)
        self.assertIn("castCd=3.5f;", self.unity)
        self.assertIn("BossHealth - Damage", self.cpp)
        self.assertIn("bossHp-damage", self.unity)

    def test_progression_cap_and_save_isolation(self):
        self.assertIn("FMath::Min(100, Victories + 1)", self.cpp)
        self.assertIn("Mathf.Min(100, wins + 1)", self.unity)
        self.assertIn('MoonfallPrototypeBossRewards', self.cpp)
        self.assertIn('moonfall_experiment_wins', self.unity)
        self.assertIn("SaveGameToSlot", self.cpp)
        self.assertIn("PlayerPrefs.Save()", self.unity)

    def test_godot_inventory_ports_include_guardrails(self):
        unity = (ROOT / "unity/Assets/Scripts/MoonfallInventoryRules.cs").read_text(encoding="utf-8")
        unreal = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallInventoryRules.h").read_text(encoding="utf-8")
        for src in (unity, unreal):
            self.assertIn("BagStartCapacity = 100", src)
            self.assertIn("BagMaxCapacity = 500", src)
            self.assertIn("MaterialStackSize = 999", src)
            self.assertIn("MaterialLimit = 99999", src)
            self.assertIn("CraftIssue", src)
            self.assertIn("TryCraft", src)
            self.assertIn("Bag full.", src)
            self.assertIn("Invalid recipe cost.", src)

    def test_gear_transfer_sort_and_discard_ports(self):
        unity = (ROOT / "unity/Assets/Scripts/MoonfallGearRules.cs").read_text(encoding="utf-8")
        unreal = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallGearRules.h").read_text(encoding="utf-8")
        for src in (unity, unreal):
            for token in ("Transfer(", "MatchingIndices(", "FirstDiscardableDrop(",
                          "Common", "Uncommon", "Legendary", "Mythic",
                          "Power", "Rarity", "Name"):
                self.assertIn(token, src)
        self.assertIn("source.RemoveAt(index)", unity)
        self.assertIn("Source.RemoveAt(Index)", unreal)
        self.assertIn("a.CompareTo(b)", unity)
        self.assertIn("A < B", unreal)

    def test_unreal_regen_only_credits_post_delay_frame_time(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("const float StaminaRegenDt = FMath::Max(0.f, Dt - StaminaRegenDelay);", tick)
        self.assertLess(tick.index("const float StaminaRegenDt ="),
                        tick.index("StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay - Dt);"))
        self.assertIn("Stamina + 30.f * StaminaRegenDt", tick)
        self.assertNotIn("Stamina + 30.f * Dt", tick)

    def test_unreal_three_hit_melee_combo_finisher_and_reset(self):
        self.assertIn("int32 MeleeComboCount = 0;", self.header)
        self.assertIn("float MeleeComboTimer = 0.f;", self.header)
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("MeleeComboTimer = FMath::Max(0.f, MeleeComboTimer - ReactionDt);", tick)
        self.assertIn("if (MeleeComboTimer <= 0.f) MeleeComboCount = 0;", tick)
        melee = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        self.assertIn("if (EnemyIndex == INDEX_NONE && !bBossInRange)", melee)
        self.assertIn("MeleeComboCount = 0;", melee)
        self.assertIn("MeleeComboTimer = 0.f;", melee)
        self.assertIn("MeleeComboCount = MeleeComboTimer > 0.f ? MeleeComboCount + 1 : 1;", melee)
        self.assertIn("const bool bComboFinisher = MeleeComboCount >= 3;", melee)
        self.assertIn("if (bComboFinisher) MeleeComboCount = 0;", melee)
        self.assertIn("MeleeComboTimer = 1.1f;", melee)
        self.assertIn("const float ComboMultiplier = bComboFinisher ? 1.5f : 1.f;", melee)
        self.assertIn("BasicAttackDamage() * ComboMultiplier *", melee)
        self.assertIn("(bRiposte ? CounterMultiplier : 1.f)", melee)
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        self.assertNotIn("MeleeComboCount", bolt)

    def test_unreal_melee_finisher_restores_stamina_only_on_hit(self):
        melee = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        self.assertIn("if (bComboFinisher)\n            Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 8.f);", melee)
        self.assertLess(melee.index("if (EnemyIndex != INDEX_NONE || bBossInRange)"),
                        melee.index("Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 8.f);"))
        self.assertNotIn("Stamina + 8.f", bolt)
        self.assertNotIn("Health + 8.f", melee)

    def test_unreal_combo_timeout_uses_hitch_safe_reaction_dt(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("const float ReactionDt = FMath::Min(Dt, 0.05f);", tick)
        self.assertIn("MeleeComboTimer = FMath::Max(0.f, MeleeComboTimer - ReactionDt);", tick)
        self.assertLess(tick.index("const float ReactionDt = FMath::Min(Dt, 0.05f);"),
                        tick.index("MeleeComboTimer = FMath::Max(0.f, MeleeComboTimer - ReactionDt);"))
        self.assertNotIn("MeleeComboTimer - Dt", tick)
        self.assertIn("if (MeleeComboTimer <= 0.f) MeleeComboCount = 0;", tick)

    def test_unreal_melee_finisher_interrupts_boss_without_nerfing_riposte(self):
        melee = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        self.assertIn("if (bRiposte && bCasting && BossHealth > 0.f)", melee)
        self.assertIn("CastCooldown = 2.f;", melee)
        self.assertIn("else if (bComboFinisher && bCasting && BossHealth > 0.f)", melee)
        self.assertIn("CastCooldown = 1.5f;", melee)
        self.assertEqual(melee.count("WarningMesh->SetVisibility(false);"), 2)
        self.assertNotIn("bComboFinisher", bolt)
        self.assertIn("if (BossHealth <= 0.f) Victory();", melee)

    def test_unreal_melee_finisher_staggers_only_regular_enemies(self):
        melee = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        damage = self.cpp.split("void AMoonfallPawn::DamageWorldEnemy(", 1)[1].split(
            "TArray<FVector> AMoonfallPawn::GetChamberMapPositions()", 1)[0]
        self.assertIn("DamageWorldEnemy(EnemyIndex, Damage, bRiposte, bComboFinisher);", melee)
        self.assertIn("DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true);", bolt)
        self.assertIn("bComboFinisher && !Enemy.bGuardian", damage)
        self.assertIn("Enemy.bGuardian ? 0.8f : 0.4f", damage)
        self.assertIn("Enemy.bGuardian ? 120.f : 90.f", damage)
        self.assertIn("Enemy.bGuardian ? 600.f : 900.f", damage)

    def test_unreal_final_stagger_frame_blocks_movement_and_contact(self):
        self.assertIn("bool bStaggeredThisFrame = false;", self.header)
        ai = self.cpp.split("void AMoonfallPawn::UpdateWorldEnemies(float Dt)", 1)[1].split(
            "int32 AMoonfallPawn::NearestEnemyIndex", 1)[0]
        self.assertIn("Enemy.bStaggeredThisFrame = Enemy.StaggerTimer > 0.f;", ai)
        self.assertIn("if (Enemy.bStaggeredThisFrame)", ai)
        self.assertLess(ai.index("Enemy.bStaggeredThisFrame = Enemy.StaggerTimer > 0.f;"),
                        ai.index("Enemy.StaggerTimer = FMath::Max(0.f, Enemy.StaggerTimer - MoveDt);"))
        self.assertIn("if (!Candidate.Mesh || Candidate.bReturningHome ||\n                    Candidate.bStaggeredThisFrame || !Candidate.bAttackReadyThisFrame) continue;", self.cpp)

    def test_unreal_guardian_riposte_knockback_stays_in_chamber(self):
        damage = self.cpp.split("void AMoonfallPawn::DamageWorldEnemy(", 1)[1].split(
            "TArray<FVector> AMoonfallPawn::GetChamberMapPositions()", 1)[0]
        self.assertIn("if ((bRiposte && Enemy.bGuardian) || (bComboFinisher && !Enemy.bGuardian))", damage)
        self.assertIn("Enemy.StaggerTimer = Enemy.bGuardian ? 0.8f : 0.4f;", damage)
        self.assertIn("if (Enemy.Mesh && HeroMesh)", damage)
        self.assertIn("Away.Z = 0.f;", damage)
        self.assertIn("if (!Away.IsNearlyZero())", damage)
        self.assertIn("(Enemy.bGuardian ? 120.f : 90.f)", damage)
        self.assertIn("LeashDelta.GetClampedToMaxSize2D(Enemy.bGuardian ? 600.f : 900.f)", damage)
        self.assertIn("Enemy.Mesh->SetRelativeLocation(Bounded);", damage)
        self.assertLess(damage.index("if (Enemy.Health > 0.f)"), damage.index("if ((bRiposte && Enemy.bGuardian)"))

    def test_unreal_riposte_staggers_guardians_and_blocks_contact(self):
        self.assertIn("float StaggerTimer = 0.f;", self.header)
        self.assertIn("void AMoonfallPawn::DamageWorldEnemy(int32 Index, float Damage, bool bRiposte, bool bComboFinisher, bool bMagic)", self.cpp)
        self.assertIn("if ((bRiposte && Enemy.bGuardian) || (bComboFinisher && !Enemy.bGuardian))", self.cpp)
        self.assertIn("Enemy.StaggerTimer = Enemy.bGuardian ? 0.8f : 0.4f;", self.cpp)
        self.assertIn("Enemy.StaggerTimer = FMath::Max(0.f, Enemy.StaggerTimer - MoveDt);", self.cpp)
        self.assertIn("if (!Candidate.Mesh || Candidate.bReturningHome ||\n                    Candidate.bStaggeredThisFrame || !Candidate.bAttackReadyThisFrame) continue;", self.cpp)
        self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, bComboFinisher);", self.cpp)
        self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true);", self.cpp)

    def test_unreal_dodge_stamina_regen_delay_keeps_perfect_dodge_reward(self):
        dodge = self.cpp.split("void AMoonfallPawn::Dodge()", 1)[1].split(
            "int32 AMoonfallPawn::GuardianBlessingCount()", 1)[0]
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("float StaminaRegenDelay = 0.f;", self.header)
        self.assertIn("StaminaRegenDelay = 0.6f;", dodge)
        self.assertIn("Stamina -= 45.f;", dodge)
        self.assertIn("StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay - Dt);", tick)
        self.assertIn("const float StaminaRegenDt = FMath::Max(0.f, Dt - StaminaRegenDelay);", tick)
        self.assertIn("if (StaminaRegenDt > 0.f)", tick)
        self.assertIn("Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 30.f * StaminaRegenDt);", tick)
        self.assertIn("Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 10.f);", tick)

    def test_unreal_mob_and_boss_strikes_share_damage_recovery(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("if (Health > 0.f && EnemyDamageCooldown <= 0.f)", tick)
        self.assertIn("if (ContactIndex != INDEX_NONE)", tick)
        boss = tick.split("if (CastTimer <= 0.f)", 1)[1]
        self.assertIn("if (bDodgeActiveThisFrame)", boss)
        self.assertIn("else if (EnemyDamageCooldown <= 0.f)", boss)
        self.assertIn("EnemyDamageCooldown = 1.2f;", boss)
        self.assertLess(boss.index("else if (EnemyDamageCooldown <= 0.f)"),
                        boss.index("Health = FMath::Max(0.f, Health -"))
        self.assertIn("EnemyDamageCooldown = FMath::Max(0.f, EnemyDamageCooldown - Dt);", tick)

    def test_unreal_targeting_ignores_missing_meshes_and_avoids_sqrt(self):
        nearest = self.cpp.split("int32 AMoonfallPawn::NearestEnemyIndex(float Radius) const", 1)[1].split(
            "TArray<FVector> AMoonfallPawn::GetEnemyMapPositions()", 1)[0]
        self.assertIn("if (!HeroMesh || Radius <= 0.f) return INDEX_NONE;", nearest)
        self.assertIn("if (!Enemy.Mesh) continue;", nearest)
        self.assertIn("float BestDistanceSq = FMath::Square(Radius);", nearest)
        self.assertIn("const float DistanceSq = Delta.SizeSquared2D();", nearest)
        self.assertIn("if (DistanceSq < BestDistanceSq)", nearest)
        self.assertNotIn("FVector::Dist2D", nearest)

    def test_unreal_retreating_enemies_cannot_mask_engaged_contact(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        selection = tick.split("// Find the nearest *engaged* contact.", 1)[1].split(
            "if (ContactIndex != INDEX_NONE)", 1)[0]
        self.assertIn("if (!Candidate.Mesh || Candidate.bReturningHome ||\n                    Candidate.bStaggeredThisFrame || !Candidate.bAttackReadyThisFrame) continue;", selection)
        self.assertIn("HomeOffset.SizeSquared2D() >= FMath::Square(Leash)", selection)
        self.assertIn("if (DistanceSq < ContactDistanceSq)", selection)
        self.assertIn("ContactIndex = I;", selection)
        self.assertNotIn("NearestEnemyIndex(175.f)", selection)
        combat = tick.split("if (ContactIndex != INDEX_NONE)", 1)[1].split(
            "EnemyDamageCooldown = 1.2f;", 1)[0]
        self.assertIn("if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;", combat)
        self.assertIn("Health = FMath::Max(0.f, Health - ContactDamage);", combat)
        self.assertIn("Contact.bEnraged ? 4.f : 0.f", combat)

    def test_unreal_guardian_frenzy_phase_at_low_health(self):
        self.assertIn("bool bEnraged = false;", self.header)
        ai = self.cpp.split("void AMoonfallPawn::UpdateWorldEnemies(float Dt)", 1)[1].split(
            "int32 AMoonfallPawn::NearestEnemyIndex", 1)[0]
        self.assertIn("Enemy.bEnraged = Enemy.bGuardian &&", ai)
        self.assertIn("Enemy.Health <= 0.35f * 1.8f * MoonfallWorldRules::EnemyMaxHealth(Enemy.Region)", ai)
        self.assertIn("Enemy.bEnraged ? 360.f : (Enemy.bGuardian ? 280.f : 200.f)", ai)
        self.assertIn("(Contact.bEnraged ? 4.f : 0.f)", self.cpp)
        self.assertIn("if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;", self.cpp)
        self.assertNotIn("Health += 4.f;", self.cpp)

    def test_unreal_enemy_leash_has_return_hysteresis(self):
        self.assertIn("bool bReturningHome = false;", self.header)
        ai = self.cpp.split("void AMoonfallPawn::UpdateWorldEnemies(float Dt)", 1)[1].split(
            "int32 AMoonfallPawn::NearestEnemyIndex", 1)[0]
        self.assertIn("Enemy.bReturningHome = true;", ai)
        self.assertIn("Enemy.bReturningHome = false;", ai)
        self.assertIn("FMath::Square(Leash - 140.f)", ai)
        self.assertIn("!Enemy.bReturningHome && FromHome.SizeSquared()", ai)
        self.assertIn("Enemy.Mesh->SetRelativeLocation(From - FromHome.GetSafeNormal() * ReturnStep);", ai)

    def test_unreal_riposte_window_survives_frame_stall(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("const float ReactionDt = FMath::Min(Dt, 0.05f);", tick)
        self.assertIn("CounterWindow = FMath::Max(0.f, CounterWindow - ReactionDt);", tick)
        self.assertNotIn("CounterWindow = FMath::Max(0.f, CounterWindow - Dt);", tick)
        self.assertIn("CounterWindow = CounterDuration;", tick)

    def test_unreal_active_dodge_snapshot_survives_final_frame(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("const bool bDodgeActiveThisFrame = DodgeTimer > 0.f;", tick)
        self.assertLess(tick.index("const bool bDodgeActiveThisFrame = DodgeTimer > 0.f;"),
                        tick.index("DodgeTimer = FMath::Max(0.f, DodgeTimer - ReactionDt);"))
        self.assertIn("Direction * (bDodgeActiveThisFrame ? 1200.f : 600.f) * MovementDt", tick)
        self.assertEqual(tick.count("if (bDodgeActiveThisFrame)"), 2)
        self.assertEqual(tick.count("DodgeTimer > 0.f"), 1)

    def test_unreal_dodge_window_matches_hitch_safe_boss_telegraph(self):
        tick = self.cpp.split("void AMoonfallPawn::Tick(", 1)[1]
        self.assertIn("const float ReactionDt = FMath::Min(Dt, 0.05f);", tick)
        self.assertIn("DodgeTimer = FMath::Max(0.f, DodgeTimer - ReactionDt);", tick)
        self.assertNotIn("DodgeTimer = FMath::Max(0.f, DodgeTimer - Dt);", tick)
        self.assertIn("const float TelegraphDt = FMath::Min(Dt, 0.05f);", tick)

    def test_unreal_boss_telegraph_survives_frame_hitch(self):
        start = self.cpp.index("if (!bCasting)", self.cpp.index("void AMoonfallPawn::Tick("))
        boss = self.cpp[start:]
        self.assertIn("const float TelegraphDt = FMath::Min(Dt, 0.05f);", boss)
        self.assertIn("CastTimer -= TelegraphDt;", boss)
        self.assertNotIn("CastTimer -= Dt;", boss)
        self.assertIn("WarningMesh->SetVisibility(false);", boss)

    def test_unreal_three_phase_boss_telegraph_and_dodge(self):
        self.assertIn("BossHealth <= 30.f ? 3 : (BossHealth <= 65.f ? 2 : 1)", self.cpp)
        self.assertIn("BossAttackCount % 2 == 0", self.cpp)
        self.assertIn("BossWarningRadius / 50.f", self.cpp)
        self.assertIn("BossAttackPhase == 3 ? 0.95f", self.cpp)
        self.assertIn("BossAttackPhase == 3 ? 2.25f", self.cpp)
        self.assertIn("BossAttackPhase == 3 ? 28.f", self.cpp)
        self.assertIn("FVector::Dist2D(HeroMesh->GetRelativeLocation(), TargetWorld) < BossWarningRadius", self.cpp)
        self.assertIn("if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;", self.cpp)
        self.assertIn("float BossWarningRadius = 250.f;", self.header)

    def test_boss_victory_loot_is_wired_into_both_prototypes(self):
        self.assertIn("MoonfallLootRules.RarityIndex(Random.Range(0, 1000), 0, 0)", self.unity)
        self.assertIn("bossLoot = MoonfallLootRules.Rarities[rank]", self.unity)
        self.assertIn("MoonfallLootRules::RarityIndex(FMath::RandRange(0, 999), 0, 0)", self.cpp)
        self.assertIn("BossLoot = FString(MoonfallLootRules::Rarities[Rank])", self.cpp)
        self.assertIn("bWon = true;", self.cpp)
        self.assertIn("won=true;", self.unity)

    def test_boss_relic_inventory_save_restore(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn("TArray<FString> BossRelics", save)
        self.assertIn("Progress->BossRelics = BossInventory;", self.cpp)
        self.assertIn("Progress->BossRelics", self.cpp)
        self.assertIn("BossInventory.Num() < 100", self.cpp)
        self.assertIn("moonfall_experiment_relic_count", self.unity)
        self.assertIn("moonfall_experiment_relic_", self.unity)
        self.assertIn("bossInventory.Count < 100", self.unity)
        self.assertIn("bossInventory.Add(bossLoot)", self.unity)
        self.assertIn("PlayerPrefs.Save()", self.unity)

    def test_godot_rpg_character_progression_integrated(self):
        unity = (ROOT / "unity/Assets/Scripts/MoonfallProgression.cs").read_text()
        unreal = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallProgression.h").read_text()
        for src in (unity, unreal):
            for token in ("75 + (Level - 1) * 40", "15 +", "HeroPoints", "TamerPoints",
                          "Combo", "Experience", "AwardDefeat("):
                self.assertIn(token, src)
        self.assertIn("progression.AwardDefeat(0, true, true, Time.time)", self.unity)
        self.assertIn("Progression.AwardDefeat(0, true, true,", self.cpp)
        self.assertIn("moonfall_experiment_level", self.unity)
        self.assertIn("Progress->HeroLevel = Progression.Level;", self.cpp)
        self.assertIn("Progress->HeroXP = Progression.Experience;", self.cpp)

    def test_live_rpg_orbs_and_minimap_are_on_screen(self):
        unity = self.unity
        unreal_hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text()
        game_mode = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallGameMode.cpp").read_text()
        self.assertIn("DrawRpgHud();", unity)
        self.assertIn("void DrawRpgHud()", unity)
        self.assertIn("progression.ExperienceToNext", unity)
        self.assertIn("new Color(1f, .22f, .30f)", unity)
        self.assertIn("new Color(.18f, .48f, 1f)", unity)
        self.assertIn("new Color(.70f, .40f, 1f)", unity)
        self.assertIn("void AMoonfallHud::DrawHUD()", unreal_hud)
        self.assertIn("GetHudXpRatio()", unreal_hud)
        self.assertIn("GetHeroMapPosition()", unreal_hud)
        self.assertIn("HUDClass = AMoonfallHud::StaticClass();", game_mode)
        # Gameplay UI contains only the map and orbs; no debug or number panels.
        self.assertNotIn("GUI.Box(new Rect(10,10", unity)
        self.assertIn("void DrawOrbValue(", unity)
        self.assertIn("DrawOrbValue(new Rect(", unity)
        self.assertIn("GetHudStamina()", unreal_hud)
        self.assertIn("GetHudExperienceToNext()", unreal_hud)
        self.assertNotIn("GUI.Box(new Rect(10,10", unity)
        self.assertNotIn("AddOnScreenDebugMessage", self.cpp)

    def test_optional_transparent_orb_skin_without_extra_hud(self):
        unreal_hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text()
        self.assertIn('Resources.Load<Texture2D>("MoonfallOrbs")', self.unity)
        self.assertIn('ornateOrbs != null', self.unity)
        self.assertIn('DrawRpgHud();', self.unity)
        self.assertIn('MoonfallOrbs.MoonfallOrbs', unreal_hud)
        self.assertIn('if (OrnateOrbs)', unreal_hud)
        self.assertIn('DrawTexture(OrnateOrbs,', unreal_hud)
        self.assertIn('DrawRing(OrbCenter', unreal_hud)
        self.assertNotIn("GUI.Box(new Rect(10,10", self.unity)
        self.assertIn("DrawOrbValue(", self.unity)
        self.assertIn("DrawText(FString::Printf(", unreal_hud)
        self.assertNotIn("GUI.Box(new Rect(10,10", self.unity)

    def test_stamina_is_live_and_orb_values_are_bounded(self):
        unreal_header = HEADER.read_text(encoding="utf-8")
        unreal_hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text()
        self.assertIn("stamina = Mathf.Min(MaxStamina, stamina + 30f * dt)", self.unity)
        self.assertIn("stamina -= 45f", self.unity)
        self.assertIn("Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 30.f * StaminaRegenDt)", self.cpp)
        self.assertIn("Stamina -= 45.f", self.cpp)
        self.assertIn("GetHudStamina()", unreal_header)
        self.assertIn("GetHudMana()", unreal_header)
        self.assertIn("GetHudExperience()", unreal_header)
        self.assertIn("DrawText(FString::Printf(", unreal_hud)
        # Numbers are drawn only within the ornate-asset branch.
        self.assertIn("if (OrnateOrbs)", unreal_hud)
        self.assertNotIn("AddOnScreenDebugMessage", self.cpp)

    def test_mana_spells_and_healing_are_real_and_bounded(self):
        config = INPUT.read_text(encoding="utf-8")
        self.assertIn('ActionName="ArcaneBolt",Key=Q', config)
        self.assertIn('ActionName="ArcaneHeal",Key=E', config)
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', config)
        self.assertIn("Input.GetKeyDown(KeyCode.Q)", self.unity)
        self.assertIn("Input.GetKeyDown(KeyCode.E)", self.unity)
        self.assertIn("mana >= 30f", self.unity)
        self.assertIn("mana >= 40f", self.unity)
        self.assertIn("mana -= 30f;", self.unity)
        self.assertIn("mana -= 40f;", self.unity)
        self.assertIn("mana = Mathf.Min(MaxMana, mana + 8f * dt)", self.unity)
        self.assertIn("Ring(new Vector2(90f, Screen.height - 108f), 34f, mana / MaxMana", self.unity)
        self.assertIn("AMoonfallPawn::CastBolt()", self.cpp)
        self.assertIn("AMoonfallPawn::CastHeal()", self.cpp)
        self.assertIn("Mana < 30.f", self.cpp)
        self.assertIn("Mana < 40.f", self.cpp)
        self.assertIn("Mana = FMath::Min(180.f, Mana + 8.f * Dt)", self.cpp)
        self.assertNotIn("AddOnScreenDebugMessage", self.cpp)
        self.assertNotIn("GUI.Box(new Rect(10,10", self.unity)

    def test_original_god_boss_art_is_not_playable(self):
        config = INPUT.read_text(encoding="utf-8")
        self.assertIn('Resources.Load<Texture2D>("MoonfallArt/heroes/Z_Original")', self.unity)
        self.assertIn('Resources.Load<Texture2D>("MoonfallArt/heroes/Nyra_Original")', self.unity)
        self.assertIn('bossPortrait = AttachPortrait(boss, zPortrait, 3.6f)', self.unity)
        self.assertIn('isNyraEncounter = chamberId == MoonfallWorldRules.TotalChambers - 1', self.unity)
        self.assertIn('guardianArt = isNyraEncounter ? nyraPortrait', self.unity)
        self.assertIn('BossPortrait->SetSprite(ZArtwork);', self.cpp)
        self.assertIn('ChamberId == MoonfallWorldRules::TotalChambers - 1 && NyraArtwork', self.cpp)
        self.assertIn('NyraBillboard->SetSprite(NyraArtwork);', self.cpp)
        self.assertIn('Sprite.Create(original', self.unity)
        self.assertNotIn('SelectHero(', self.unity)
        self.assertNotIn('heroPortrait', self.unity)
        self.assertNotIn('HeroPortrait', self.cpp)
        self.assertNotIn('ApplyHeroArtwork', self.cpp)
        self.assertNotIn('SelectZArt', config)
        self.assertNotIn('SelectNyraArt', config)
        self.assertNotIn('GetKeyDown(KeyCode.Alpha1)', self.unity)
        self.assertNotIn('GetKeyDown(KeyCode.Alpha2)', self.unity)

    def test_continuous_rpg_regions_fixed_enemies_no_waves(self):
        unity_world = (ROOT / "unity/Assets/Scripts/MoonfallWorldRules.cs").read_text()
        unreal_world = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallWorldRules.h").read_text()
        save = SAVES.read_text()
        unreal_hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text()
        for world in (unity_world, unreal_world):
            self.assertIn("RegionCount = 6", world)
            self.assertIn("EnemiesPerRegion = 3", world)
            self.assertIn("TotalEnemies = RegionCount * EnemiesPerRegion", world)
            self.assertIn("EnemyPosition(", world)
            self.assertIn("RegionCenter(", world)
        self.assertIn("BuildWorld();", self.unity)
        self.assertIn("BuildWorld();", self.cpp)
        self.assertIn("MoonfallWorldRules.EnemyPosition", self.unity)
        self.assertIn("MoonfallWorldRules::EnemyPosition", self.cpp)
        self.assertIn("moonfall_experiment_defeated_mobs", self.unity)
        self.assertIn("DefeatedEnemyMask", save)
        self.assertIn("DefeatedEnemyMask |= 1 << Enemy.Id;", self.cpp)
        self.assertIn("defeatedEnemyMask |= 1 << enemy.id;", self.unity)
        self.assertIn("NearestEnemy(4f)", self.unity)
        self.assertIn("NearestEnemyIndex(400.f)", self.cpp)
        self.assertIn("GetEnemyMapPositions()", unreal_hud)
        self.assertIn("foreach (WorldEnemy enemy in enemies)", self.unity)
        self.assertNotIn("SpawnWave(", self.unity)
        self.assertNotIn("SpawnWave(", self.cpp)

    def test_post_boss_exploration_and_xp_awards(self):
        self.assertIn("if (Input.GetMouseButtonDown(0) && attackCd<=0)", self.unity)
        self.assertIn("bool bossInRange = !won && Vector3.Distance(hero.position,boss.position)<4f;", self.unity)
        self.assertIn("if (Health <= 0.f || AttackCooldown > 0.f) return;", self.cpp)
        self.assertIn("const bool bBossInRange = BossDistance <= 400.f;", self.cpp)
        self.assertNotIn("if (Health <= 0.f || bWon || AttackCooldown > 0.f)", self.cpp)

    def test_thirty_connected_chambers_and_persistent_loot(self):
        unity_world = (ROOT / "unity/Assets/Scripts/MoonfallWorldRules.cs").read_text()
        unreal_world = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallWorldRules.h").read_text()
        saved = SAVES.read_text()
        unreal_hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text()
        for world in (unity_world, unreal_world):
            self.assertIn("ChambersPerRegion = 5", world)
            self.assertIn("TotalChambers = RegionCount * ChambersPerRegion", world)
            self.assertIn("ChamberCenter(", world)
            self.assertIn("ChamberReward(", world)
        self.assertIn("CollectNearbyChambers();", self.unity)
        self.assertIn("CollectNearbyChambers();", self.cpp)
        self.assertIn("moonfall_experiment_chambers", self.unity)
        self.assertIn("ClaimedChamberMask", saved)
        self.assertIn("Progress->ClaimedChamberMask = ClaimedChamberMask", self.cpp)
        self.assertIn("claimedChamberMask |= 1 << id", self.unity)
        self.assertIn("ClaimedChamberMask |= 1 << Id", self.cpp)
        self.assertIn("GetChamberMapPositions()", unreal_hud)
        self.assertIn("foreach (Transform cache in chamberRewards)", self.unity)

    def test_boss_stays_dead_and_only_attacks_nearby(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn("bBossDefeated = false", save)
        self.assertIn("Progress->bBossDefeated = bWon;", self.cpp)
        self.assertIn("bWon = Progress->bBossDefeated;", self.cpp)
        self.assertIn("moonfall_experiment_boss_defeated", self.unity)
        self.assertIn('won = PlayerPrefs.GetInt("moonfall_experiment_boss_defeated", 0) == 1;', self.unity)
        self.assertIn("Distance(hero.position, boss.position) > 16f", self.unity)
        self.assertIn("Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) > 1600.f", self.cpp)
        self.assertIn("warning.gameObject.SetActive(false)", self.unity)
        self.assertIn("WarningMesh->SetVisibility(false)", self.cpp)

    def test_thirty_guardians_gate_chamber_rewards_and_save(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn('moonfall_experiment_guardians', self.unity)
        self.assertIn('defeatedGuardianMask |= 1 << enemy.id;', self.unity)
        self.assertIn('if ((defeatedGuardianMask & (1 << id)) == 0) continue;', self.unity)
        self.assertIn('guardian = true', self.unity)
        self.assertIn('MoonfallWorldRules.ChamberReward(region, chamber)', self.unity)
        self.assertIn('DefeatedGuardianMask = 0', save)
        self.assertIn('Guardian.bGuardian = true;', self.cpp)
        self.assertIn('if (Enemy.bGuardian) DefeatedGuardianMask |= 1 << Enemy.Id;', self.cpp)
        self.assertIn('if ((DefeatedGuardianMask & (1 << Id)) == 0) continue;', self.cpp)
        self.assertIn('Progress->DefeatedGuardianMask = DefeatedGuardianMask;', self.cpp)
        self.assertIn('Progress->DefeatedGuardianMask &', self.cpp)
        self.assertIn('Progression.AwardDefeat(Region, bGuardian, false', self.cpp)

    def test_leashed_enemy_ai_does_not_spawn_waves_or_cross_regions(self):
        self.assertIn("void UpdateEnemies(float dt)", self.unity)
        self.assertIn("UpdateEnemies(dt);", self.unity)
        self.assertIn("enemy.guardian ? 6f : 9f", self.unity)
        self.assertIn("enemy.body.position += offset.normalized * step;", self.unity)
        self.assertIn("void AMoonfallPawn::UpdateWorldEnemies(float Dt)", self.cpp)
        self.assertIn("UpdateWorldEnemies(Dt);", self.cpp)
        self.assertIn("Enemy.bGuardian ? 600.f : 900.f", self.cpp)
        self.assertIn("Enemy.Home = Body->GetRelativeLocation();", self.cpp)
        self.assertIn("Guardian.Home = GuardianBody->GetRelativeLocation();", self.cpp)
        self.assertNotIn("SpawnWave(", self.unity)
        self.assertNotIn("SpawnWave(", self.cpp)

    def test_unreal_ai_home_return_and_hitch_safety(self):
        # Local enemies must go home when the hero leaves, not stick at leash range.
        self.assertIn("const bool bAggro = HeroDistSq", self.cpp)
        self.assertIn("FromHome.SizeSquared() < FMath::Square(Leash)", self.cpp)
        self.assertIn("FromHome.SizeSquared() > FMath::Square(30.f)", self.cpp)
        self.assertIn("Enemy.Mesh->SetRelativeLocation(From - FromHome.GetSafeNormal() * ReturnStep);", self.cpp)
        # Large frame delays must not cause giant positional jumps.
        self.assertIn("const float MoveDt = FMath::Clamp(Dt, 0.f, 0.05f);", self.cpp)
        self.assertIn("const float MovementDt = FMath::Min(Dt, 0.05f);", self.cpp)
        self.assertIn("Direction * (bDodgeActiveThisFrame ? 1200.f : 600.f) * MovementDt", self.cpp)
        # A stationary player cannot spend dodge stamina for no displacement.
        self.assertIn("FVector2D(Forward, Right).SizeSquared() > 0.01f", self.cpp)
        self.assertNotIn("SpawnWave(", self.cpp)

    def test_unreal_level_max_hp_respects_existing_saves_without_kill_healing(self):
        self.assertIn("100.f + 5.f * Victories + 2.f * (Progression.Level - 1)", self.header)
        self.assertIn("2.f * (FMath::Clamp(Progress->HeroLevel, 1, 100) - 1)", self.cpp)
        self.assertIn("Health = GetHudMaxHealth();", self.cpp)
        self.assertIn("const float MaxHealth = GetHudMaxHealth();", self.cpp)
        self.assertIn("Health = FMath::Min(MaxHealth, Health + HealingSpellAmount());", self.cpp)
        self.assertIn("Progress->SavedHealth = Health;", self.cpp)
        self.assertNotIn("Health += 2.f;", self.cpp)
        self.assertNotIn("Health += GuardianBlessingCount()", self.cpp)

    def test_unreal_unique_guardian_blessings_persist_without_extra_save_fields(self):
        self.assertIn("int32 AMoonfallPawn::GuardianBlessingCount() const", self.cpp)
        self.assertIn("Id < MoonfallWorldRules::TotalChambers", self.cpp)
        self.assertIn("(DefeatedGuardianMask & (1 << Id)) != 0", self.cpp)
        self.assertIn("0.5f * GuardianBlessingCount()", self.cpp)
        self.assertIn("1.f * GuardianBlessingCount()", self.cpp)
        self.assertIn("if (Enemy.bGuardian) DefeatedGuardianMask |= 1 << Enemy.Id;", self.cpp)
        self.assertIn("Progress->DefeatedGuardianMask = DefeatedGuardianMask;", self.cpp)
        self.assertIn("DefeatedGuardianMask = Progress->DefeatedGuardianMask &", self.cpp)
        self.assertNotIn("Health += GuardianBlessingCount()", self.cpp)

    def test_unreal_click_attack_miss_recovers_without_spending_riposte(self):
        attack = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        self.assertIn("AttackCooldown = (EnemyIndex != INDEX_NONE || bBossInRange) ? 0.35f : 0.12f;", attack)
        self.assertIn("if (EnemyIndex != INDEX_NONE || bBossInRange)", attack)
        self.assertIn("CounterWindow = 0.f;", attack)
        self.assertLess(attack.index("if (EnemyIndex != INDEX_NONE || bBossInRange)"), attack.index("CounterWindow = 0.f;"))
        self.assertNotIn("AttackCooldown = 0.35f;", attack)

    def test_unreal_attacks_prioritize_nearest_valid_target(self):
        attack = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        for combat in (attack, bolt):
            self.assertIn("const float EnemyDistance =", combat)
            self.assertIn("const bool bHitBoss = bBossInRange && BossDistance < EnemyDistance;", combat)
            self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte", combat)
            self.assertIn("BossHealth = FMath::Max(0.f, BossHealth - Damage);", combat)
        self.assertIn("const bool bBossInRange = BossDistance <= 400.f;", attack)
        self.assertIn("const bool bBossInRange = BossDistance <= 1000.f;", bolt)

    def test_unreal_perfect_dodge_interrupts_boss_for_both_attack_types(self):
        attack = self.cpp.split("void AMoonfallPawn::Attack()", 1)[1].split(
            "void AMoonfallPawn::CastBolt()", 1)[0]
        bolt = self.cpp.split("void AMoonfallPawn::CastBolt()", 1)[1].split(
            "void AMoonfallPawn::CastHeal()", 1)[0]
        for source in (attack, bolt):
            self.assertIn("const bool bRiposte = CounterWindow > 0.f;", source)
            self.assertIn("if (bRiposte && bCasting && BossHealth > 0.f)", source)
            self.assertIn("WarningMesh->SetVisibility(false);", source)
            self.assertIn("CastCooldown = 2.f; // Start a fresh, exact interrupt recovery window.", source)
            self.assertNotIn("CastCooldown = FMath::Max(CastCooldown, 2.f);", source)
            self.assertLess(source.index("BossHealth = FMath::Max(0.f, BossHealth - Damage);"),
                            source.index("if (bRiposte && bCasting && BossHealth > 0.f)"))
            self.assertIn("if (BossHealth <= 0.f) Victory();", source)
            self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte", source)

    def test_unreal_spell_riposte_uses_successful_target_only(self):
        start = self.cpp.index("void AMoonfallPawn::CastBolt()")
        end = self.cpp.index("void AMoonfallPawn::CastHeal()", start)
        bolt = self.cpp[start:end]
        self.assertIn("if (Health <= 0.f || BoltCooldown > 0.f || Mana < 30.f) return;", bolt)
        self.assertIn("if (EnemyIndex == INDEX_NONE && !bBossInRange) return;", bolt)
        self.assertLess(bolt.index("if (EnemyIndex == INDEX_NONE"), bolt.index("CounterWindow = 0.f;"))
        self.assertIn("(bRiposte ? CounterMultiplier : 1.f)", bolt)
        self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true);", bolt)
        self.assertIn("BossHealth - Damage", bolt)
        self.assertIn("Mana -= 30.f;", bolt)
        self.assertIn("BoltCooldown = 1.1f;", bolt)

    def test_unreal_boss_dodge_stamina_and_death_cleanup(self):
        self.assertIn("if (bWon || Health <= 0.f)", self.cpp)
        self.assertIn("bCasting = false;\n        WarningMesh->SetVisibility(false);\n        return;", self.cpp)
        self.assertIn("Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 10.f);", self.cpp)
        self.assertIn("CounterWindow = CounterDuration;", self.cpp)
        self.assertIn("BossAttackPhase == 3 ? 28.f", self.cpp)
        self.assertNotIn("Health = FMath::Min(GetHudMaxHealth(), Health + 10.f)", self.cpp)

    def test_unreal_melee_aggro_and_boss_warning_pulse(self):
        self.assertIn("const bool bAggro = HeroDistSq < FMath::Square(800.f)", self.cpp)
        self.assertIn("if (HeroDistSq > FMath::Square(160.f))", self.cpp)
        self.assertIn("ToCandidateHome.GetClampedToMaxSize2D(Leash)", self.cpp)
        self.assertIn("BossWarningRadius / 50.f * Pulse", self.cpp)
        self.assertIn("BossAttackPhase == 3 ? 17.f : 11.f", self.cpp)
        self.assertIn("WarningMesh->SetVisibility(false);", self.cpp)

    def test_hero_class_combat_without_passive_kill_healing(self):
        self.assertIn("float BasicAttackDamage()", self.unity)
        self.assertIn("float ArcaneBoltDamage()", self.unity)
        self.assertIn("float HealingSpellAmount()", self.unity)
        self.assertIn("return 18f + progression.Level;", self.unity)
        self.assertIn("return 28f + 2f * progression.Level;", self.unity)
        self.assertIn("DamageEnemy(foe, damage)", self.unity)
        self.assertIn("DamageEnemy(foe, ArcaneBoltDamage())", self.unity)
        self.assertIn("hp + HealingSpellAmount()", self.unity)
        self.assertIn("float AMoonfallPawn::BasicAttackDamage() const", self.cpp)
        self.assertIn("float AMoonfallPawn::ArcaneBoltDamage() const", self.cpp)
        self.assertIn("float AMoonfallPawn::HealingSpellAmount() const", self.cpp)
        self.assertIn("return 18.f + Progression.Level + 0.5f * GuardianBlessingCount();", self.cpp)
        self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, bComboFinisher);", self.cpp)
        self.assertIn("if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true);", self.cpp)
        self.assertIn("Health + HealingSpellAmount()", self.cpp)

    def test_legacy_hero_selection_is_ignored_without_deleting_saves(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn("SelectedHeroIndex = 0;", save)  # Legacy save field stays readable.
        self.assertIn("Progress->bBossDefeated = bWon;", self.cpp)
        self.assertIn("bWon = Progress->bBossDefeated;", self.cpp)
        self.assertIn('moonfall_experiment_boss_defeated', self.unity)
        self.assertNotIn("HeroArtIndex", self.cpp)
        self.assertNotIn("selectedHero", self.unity)
        self.assertNotIn('PlayerPrefs.SetInt("moonfall_experiment_hero_art"', self.unity)
        self.assertNotIn("Progress->SelectedHeroIndex =", self.cpp)
        self.assertIn("HeroMesh->SetRelativeLocation(", self.cpp)
        self.assertIn('hero = Make("Hero placeholder"', self.unity)

    def test_kills_and_level_ups_never_automatically_heal(self):
        # Manual E spell healing remains; XP/loot and permanent max-HP bonuses remain.
        self.assertIn("hp + HealingSpellAmount()", self.unity)
        self.assertIn("Health + HealingSpellAmount()", self.cpp)
        self.assertIn("progression.AwardDefeat(enemy.region, enemy.guardian, false, Time.time);", self.unity)
        self.assertIn("Progression.AwardDefeat(Region, bGuardian, false", self.cpp)
        self.assertIn("progression.AwardDefeat(0, true, true, Time.time);", self.unity)
        self.assertIn("Progression.AwardDefeat(0, true, true", self.cpp)
        self.assertNotIn("hp + 20f * levelsGained", self.unity)
        self.assertNotIn("hp + 20 * levelsGained", self.unity)
        self.assertNotIn("hp += 5 * (wins - previousWins)", self.unity)
        self.assertNotIn("Health + 20.f * LevelsGained", self.cpp)
        self.assertNotIn("Health += 5.f * (Victories - Previous)", self.cpp)
        self.assertNotIn("enemy.guardian ? 16f : 8f", self.unity)
        self.assertNotIn("bGuardian ? 16.f : 8.f", self.cpp)

    def test_original_world_art_is_connected_without_changing_source(self):
        manifest = (ROOT / "ART_IMPORT_MANIFEST.json").read_text(encoding="utf-8")
        for art in ("v31_4_bestiary/region_0.png",
                    "v31_4_bestiary/region_5.png",
                    "v31_3_art/treasure_cache.png"):
            self.assertIn(art, manifest)
        self.assertIn('MoonfallArt/v31_4_bestiary/region_', self.unity)
        self.assertIn('MoonfallArt/v31_3_art/treasure_cache', self.unity)
        self.assertIn("regionArt.Add(art)", self.unity)
        self.assertIn("regionArt.Add(cachePortrait)", self.unity)
        self.assertIn("if (landmark != null) landmark.rotation = view.transform.rotation;", self.unity)
        self.assertIn("UBillboardComponent* AMoonfallPawn::AddWorldArtwork(", self.cpp)
        self.assertIn("if (!Art || !Parent) return nullptr;", self.cpp)
        self.assertIn("region_%d.region_%d", self.cpp)
        self.assertIn("treasure_cache.treasure_cache", self.cpp)
        self.assertIn("if (Cache.Artwork) Cache.Artwork->DestroyComponent();", self.cpp)
        self.assertIn("if (Reward.Artwork) Cache->SetHiddenInGame(true);", self.cpp)
        self.assertNotIn("SpawnWave(", self.cpp)
        self.assertNotIn("SpawnWave(", self.unity)

    def test_new_ai_environment_art_hooks_without_replacing_heroes(self):
        self.assertIn('MoonfallGeneratedArt/" + resource', self.unity)
        self.assertIn('PlaceAilandmark("moonfall_ai_waterfall_ruins"', self.unity)
        self.assertIn('PlaceAilandmark("moonfall_ai_moonlit_gate"', self.unity)
        self.assertIn('if (art == null) return;', self.unity)
        self.assertIn("MoonfallGeneratedArt/moonfall_ai_waterfall_ruins", self.cpp)
        self.assertIn("MoonfallGeneratedArt/moonfall_ai_moonlit_gate", self.cpp)
        self.assertIn("AddWorldArtwork(Root,", self.cpp)
        self.assertIn("Z_Original.Z_Original", self.cpp)
        self.assertIn("Nyra_Original.Nyra_Original", self.cpp)
        self.assertNotIn("hp + 20f * levelsGained", self.unity)
        self.assertNotIn("Health + 20.f * LevelsGained", self.cpp)

    def test_ai_art_pack_engine_import_paths(self):
        importer = (ROOT / "unreal/MoonfallPrototype/Content/Python/import_moonfall_art.py").read_text(encoding="utf-8")
        instructions = (ROOT / "GENERATED_ART_INSTALL.md").read_text(encoding="utf-8")
        self.assertIn('NEW_SOURCE = PROJECT / "SourceArt" / "MoonfallGeneratedArt"', importer)
        self.assertIn('NEW_ROOT = "/Game/MoonfallGeneratedArt"', importer)
        self.assertIn('moonfall_ai_moonlit_gate.jpg', importer)
        self.assertIn('moonfall_ai_waterfall_ruins.jpg', importer)
        self.assertIn('if not path.is_file():', importer)
        self.assertIn('import_task(tools, path, NEW_ROOT)', importer)
        self.assertIn('engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt/', instructions)
        self.assertIn('engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt/', instructions)
        self.assertIn('AI_ART_SHA256.json', instructions)

    def test_windows_ai_art_installer_has_hash_checks_and_safe_destinations(self):
        installer = (ROOT / "tools/install_generated_art.ps1").read_text(encoding="utf-8")
        self.assertIn("AI_ART_SHA256.json", installer)
        self.assertIn("$checks.generated_art", installer)
        self.assertIn("[Security.Cryptography.SHA256]::Create()", installer)
        self.assertIn("[IO.File]::WriteAllBytes", installer)
        self.assertIn("foreach ($asset in $stage)", installer)
        self.assertIn("GENERATED_ART_SHA256.json", installer)
        self.assertNotIn("foreach ($relative in $targets)", installer)
        self.assertIn("Checksum mismatch:", installer)
        self.assertIn("Moonfall_AI_Art_Ready_To_Import.zip", installer)
        self.assertIn("MoonfallGeneratedArt", installer)
        self.assertIn("engine-comparison/unity/Assets/Resources/", installer)
        self.assertIn("engine-comparison/unreal/MoonfallPrototype/SourceArt/", installer)
        self.assertNotIn("godot-v38", installer)
        self.assertNotIn("Start-Process", installer)

    def test_python_ai_art_installer_verifies_four_jpgs_before_writing(self):
        installer = (ROOT / "tools/install_generated_art.py").read_text(encoding="utf-8")
        self.assertIn("Moonfall_AI_Art_Ready_To_Import.zip", installer)
        self.assertIn("AI_ART_SHA256.json", installer)
        self.assertIn("hashlib.sha256(data).hexdigest() != sha", installer)
        self.assertIn("if len(data) != expected_size:", installer)
        self.assertIn("if data[:3] != b", installer)
        self.assertIn("staged.append(", installer)
        self.assertIn("for target, data in staged:", installer)
        self.assertIn("engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt", installer)
        self.assertIn("engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt", installer)
        self.assertNotIn("godot-v38", installer)

    def test_optional_art_publish_is_isolated_to_experimental_pr(self):
        installer = (ROOT / "tools/install_generated_art.py").read_text(encoding="utf-8")
        self.assertIn("--commit-and-push", installer)
        self.assertIn('branch = "prototype/unreal-unity-comparison-20261104"', installer)
        self.assertIn('current_branch = git("branch", "--show-current")', installer)
        self.assertIn('"commit", "--only"', installer)
        self.assertIn('"push", "origin", f"HEAD:{branch}"', installer)
        self.assertIn('if args.commit_and_push:', installer)
        self.assertNotIn("git push --force", installer)
        self.assertNotIn("godot-v38", installer)

    def test_pinned_generated_art_hashes_and_ci_validation(self):
        import json
        import subprocess
        import sys
        manifest = json.loads((ROOT / "GENERATED_ART_SHA256.json").read_text())
        self.assertEqual(len(manifest["generated_art"]), 2)
        self.assertEqual({x["filename"] for x in manifest["generated_art"]},
                         {"moonfall_ai_moonlit_gate.jpg", "moonfall_ai_waterfall_ruins.jpg"})
        for record in manifest["generated_art"]:
            self.assertEqual(len(record["sha256"]), 64)
            self.assertGreater(record["bytes"], 10000)
        installer = (ROOT / "tools/install_generated_art.py").read_text()
        self.assertIn("if record != pinned:", installer)
        verifier = ROOT / "tools/verify_generated_art.py"
        self.assertIn("if not all(present):", verifier.read_text())
        result = subprocess.run([sys.executable, str(verifier)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_exploration_checkpoints_keep_position_and_real_hp(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn("bHasCheckpoint = false", save)
        self.assertIn("FVector SavedHeroPosition", save)
        self.assertIn("float SavedHealth", save)
        self.assertIn("CheckpointInterval = 8f", self.unity)
        self.assertIn("CheckpointInterval = 8.f", self.header)
        self.assertIn('PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_x"', self.unity)
        self.assertIn('PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_z"', self.unity)
        self.assertIn('PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_hp"', self.unity)
        self.assertIn("if (hero != null && hp > 0f)", self.unity)
        self.assertIn('PlayerPrefs.GetInt("moonfall_experiment_checkpoint_active", 0) == 1', self.unity)
        self.assertIn("!float.IsNaN(savedHp) && !float.IsInfinity(savedHp)", self.unity)
        self.assertIn("hp = Mathf.Clamp(savedHp, 1f, 100f + 5f * wins)", self.unity)
        self.assertIn("if (checkpointTimer >= CheckpointInterval)", self.unity)
        self.assertIn("if (Progress->bHasCheckpoint && FMath::IsFinite(Progress->SavedHealth)", self.cpp)
        self.assertIn("Progress->SavedHeroPosition.ContainsNaN()", self.cpp)
        self.assertIn("Progress->SavedHealth = Health;", self.cpp)
        self.assertIn("Progress->SavedHeroPosition = HeroMesh->GetRelativeLocation();", self.cpp)
        self.assertIn("if (!bLoadedCheckpoint)", self.cpp)
        self.assertIn("if (CheckpointTimer >= CheckpointInterval)", self.cpp)
        # No combat kill path should restore HP.
        self.assertNotIn("Health + 20.f * LevelsGained", self.cpp)
        self.assertNotIn("hp + 20f * levelsGained", self.unity)

    def test_unreal_death_save_does_not_wipe_last_living_checkpoint(self):
        # SaveProgress must load/reuse a prior SaveGame, rather than always
        # replacing it with default bHasCheckpoint=false when Health is zero.
        start = self.cpp.index("void AMoonfallPawn::SaveProgress()")
        end = self.cpp.index("void AMoonfallPawn::MoveForward(", start)
        save_code = self.cpp[start:end]
        self.assertIn('LoadGameFromSlot(TEXT("MoonfallPrototypeBossRewards"), 0)', save_code)
        self.assertIn("if (!Progress)", save_code)
        self.assertIn("CreateSaveGameObject(", save_code)
        self.assertLess(save_code.index("LoadGameFromSlot("),
                        save_code.index("CreateSaveGameObject("))
        self.assertIn("if (HeroMesh && Health > 0.f)", save_code)
        self.assertIn("Progress->bHasCheckpoint = true;", save_code)
        self.assertNotIn("Progress->bHasCheckpoint = false;", save_code)
        self.assertIn("Progress->SavedHealth = Health;", save_code)

    def test_clean_exit_and_suspend_persist_checkpoint_progress(self):
        self.assertIn("void OnApplicationPause(bool paused)", self.unity)
        self.assertIn("if (paused && hero != null) SaveProgress();", self.unity)
        self.assertIn("void OnApplicationQuit()", self.unity)
        self.assertIn("if (hero != null) SaveProgress();", self.unity)
        self.assertIn("EndPlay(const EEndPlayReason::Type EndPlayReason) override", self.header)
        start = self.cpp.index("void AMoonfallPawn::EndPlay(")
        end = self.cpp.index("void AMoonfallPawn::BeginPlay()", start)
        code = self.cpp[start:end]
        self.assertIn("EEndPlayReason::Quit", code)
        self.assertIn("EEndPlayReason::LevelTransition", code)
        self.assertIn("if (HeroMesh && GetWorld()) SaveProgress();", code)
        self.assertIn("Super::EndPlay(Reason);", code)
        self.assertNotIn("EEndPlayReason::EndPlayInEditor", code)
        self.assertIn("if (HeroMesh && Health > 0.f)", self.cpp)

    def test_checkpoint_resource_bars_preserve_mana_and_stamina(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn('bool bHasResourceCheckpoint = false;', save)
        self.assertIn('float SavedMana = 180.f;', save)
        self.assertIn('float SavedStamina = 150.f;', save)
        for token in ('moonfall_experiment_checkpoint_mana',
                      'moonfall_experiment_checkpoint_stamina',
                      'moonfall_experiment_checkpoint_resources_active'):
            self.assertIn(token, self.unity)
        self.assertIn('if (PlayerPrefs.GetInt("moonfall_experiment_checkpoint_resources_active", 0) == 1)', self.unity)
        self.assertIn('mana = Mathf.Clamp(savedMana, 0f, MaxMana)', self.unity)
        self.assertIn('stamina = Mathf.Clamp(savedStamina, 0f, MaxStamina)', self.unity)
        self.assertIn('!float.IsInfinity(savedMana)', self.unity)
        self.assertIn('!float.IsInfinity(savedStamina)', self.unity)
        self.assertIn('if (Progress->bHasResourceCheckpoint)', self.cpp)
        self.assertIn('FMath::IsFinite(Progress->SavedMana)', self.cpp)
        self.assertIn('FMath::IsFinite(Progress->SavedStamina)', self.cpp)
        self.assertIn('Progress->SavedMana = Mana;', self.cpp)
        self.assertIn('Progress->SavedStamina = Stamina;', self.cpp)
        self.assertIn('Progress->bHasResourceCheckpoint = true;', self.cpp)
        self.assertIn('if (HeroMesh && Health > 0.f)', self.cpp)
        self.assertIn('if (hero != null && hp > 0f)', self.unity)

    def test_boss_hp_resets_but_victory_remains_saved(self):
        save = SAVES.read_text(encoding="utf-8")
        # Unfinished fights restart normally; no saved partial boss HP.
        for src in (self.unity, self.cpp, save):
            self.assertNotIn("SavedBossHealth", src)
            self.assertNotIn("bHasBossHealthCheckpoint", src)
            self.assertNotIn("checkpoint_boss_hp", src)
        # Permanent boss defeat remains recorded across restarts.
        self.assertIn('PlayerPrefs.SetInt("moonfall_experiment_boss_defeated", 1)', self.unity)
        self.assertIn('PlayerPrefs.GetInt("moonfall_experiment_boss_defeated", 0) == 1', self.unity)
        self.assertIn('if (won) { bossHp = 0f; boss.gameObject.SetActive(false); }', self.unity)
        self.assertIn('Progress->bBossDefeated = bWon;', self.cpp)
        self.assertIn('bWon = Progress->bBossDefeated;', self.cpp)
        self.assertIn('if (bWon)', self.cpp)

    def test_inconsistent_chamber_claims_cannot_skip_nyra_guardian(self):
        # A claimed flag is valid only if the matching guardian was defeated.
        # Applies to all 30 chambers, including the Nyra encounter at ID 29.
        self.assertIn("claimedChamberMask &= defeatedGuardianMask;", self.unity)
        self.assertIn("ClaimedChamberMask &= DefeatedGuardianMask;", self.cpp)
        self.assertIn("if ((claimedChamberMask & (1 << chamberId)) != 0) continue;", self.unity)
        self.assertIn("if ((ClaimedChamberMask & (1 << ChamberId)) != 0) continue;", self.cpp)
        self.assertIn("if ((defeatedGuardianMask & (1 << id)) == 0) continue;", self.unity)
        self.assertIn("if ((DefeatedGuardianMask & (1 << Id)) == 0) continue;", self.cpp)
        self.assertIn("chamberId == MoonfallWorldRules.TotalChambers - 1", self.unity)
        self.assertIn("ChamberId == MoonfallWorldRules::TotalChambers - 1", self.cpp)
        for old_claims, defeated in [(0, 0), (1 << 29, 0), ((1 << 30) - 1, 1 << 29),
                                     (1 << 29, 1 << 29)]:
            valid_claims = old_claims & defeated
            self.assertEqual(valid_claims & ~defeated, 0)
            if not defeated & (1 << 29):
                self.assertFalse(valid_claims & (1 << 29))

    def test_successful_dodge_grants_one_timed_riposte(self):
        # A real avoided hit grants a single stronger click attack.
        # Neither a missed attack nor simply pressing dodge should grant it.
        self.assertIn("const float CounterDuration = 1.5f;", self.unity)
        self.assertIn("const float CounterMultiplier = 1.75f;", self.unity)
        self.assertIn("static constexpr float CounterDuration = 1.5f;", self.header)
        self.assertIn("static constexpr float CounterMultiplier = 1.75f;", self.header)
        self.assertIn("counterWindow = Mathf.Max(0f, counterWindow - dt);", self.unity)
        self.assertIn("CounterWindow = FMath::Max(0.f, CounterWindow - ReactionDt);", self.cpp)
        self.assertNotIn("CounterWindow = FMath::Max(0.f, CounterWindow - Dt);", self.cpp)
        self.assertIn("if (dodge > 0f) counterWindow = CounterDuration;", self.unity)
        self.assertIn("if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;", self.cpp)
        self.assertIn("if (foe != null || bossInRange)", self.unity)
        self.assertIn("if (EnemyIndex != INDEX_NONE || bBossInRange)", self.cpp)
        self.assertIn("counterWindow = 0f; // One counter per successful evasion.", self.unity)
        self.assertIn("CounterWindow = 0.f; // The riposte is spent only on a successful in-range attack.", self.cpp)
        self.assertIn("Input.GetMouseButtonDown(0)", self.unity)
        self.assertIn('Input->BindAction(TEXT("Attack"), IE_Pressed', self.cpp)
        self.assertNotIn("Health += Damage", self.cpp)
        self.assertNotIn("hp += damage", self.unity)

    def test_unreal_enemy_strikes_require_visible_windups(self):
        # Contact alone must not deal damage: the foe must finish its warning.
        self.assertIn("float AttackWindup = 0.f;", self.header)
        self.assertIn("float AttackRecovery = 0.f;", self.header)
        self.assertIn("bool bAttackReadyThisFrame = false;", self.header)
        self.assertIn("Enemy.bAttackReadyThisFrame = false;", self.cpp)
        self.assertIn("Enemy.AttackWindup = Enemy.bGuardian ? 0.85f : 0.6f;", self.cpp)
        self.assertIn("Enemy.AttackRecovery = Enemy.bGuardian ? 1.7f : 1.3f;", self.cpp)
        self.assertIn("Enemy.bAttackReadyThisFrame = bAggro &&", self.cpp)
        self.assertIn("!Candidate.bAttackReadyThisFrame", self.cpp)
        self.assertIn("Enemy.AttackWindup = 0.f; // A successful interrupt cancels the warning.", self.cpp)
        self.assertIn("Enemy.Portrait->SetRelativeScale3D(FVector(2.f * WarningScale))", self.cpp)
        self.assertIn("Enemy.bGuardian ? 1.3f : 1.1f", self.cpp)
        self.assertLess(self.cpp.index("UpdateWorldEnemies(Dt);"),
                        self.cpp.index("!Candidate.bAttackReadyThisFrame"))
        self.assertNotIn("Health = FMath::Min(GetHudMaxHealth(), Health + ContactDamage)", self.cpp)

    def test_unreal_transient_damage_numbers_are_bounded_and_world_anchored(self):
        hud = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallHud.cpp").read_text(encoding="utf-8")
        self.assertIn("struct FDamageNumber", self.header)
        self.assertIn("TArray<FDamageNumber> DamageNumbers", self.header)
        self.assertIn("GetDamageNumbers() const", self.header)
        self.assertIn("DamageNumberLifetime = 1.25f", self.header)
        self.assertIn("if (DamageNumbers.Num() >= 24) DamageNumbers.RemoveAt(0);", self.cpp)
        self.assertIn("DamageNumbers[I].Age += Dt;", self.cpp)
        self.assertIn("DamageNumbers.RemoveAtSwap(I, 1, EAllowShrinking::No);", self.cpp)
        self.assertIn("RecordDamageNumber(Enemy.Mesh->GetRelativeLocation()", self.cpp)
        self.assertEqual(self.cpp.count("RecordDamageNumber(BossMesh->GetRelativeLocation()"), 2)
        self.assertIn("DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true)", self.cpp)
        self.assertIn("Hero->GetDamageNumbers()", hud)
        self.assertIn("ProjectWorldLocationToScreen(World, Screen, false)", hud)
        self.assertIn("Hit.bHeavy", hud)
        self.assertIn("Hit.bMagic", hud)
        self.assertIn("1.f - T", hud)
        self.assertNotIn("NewObject<", hud)

    def test_original_art_import_is_checksum_guarded(self):
        import_tool = (ROOT / "tools/import_godot_art.py").read_text()
        verify_tool = (ROOT / "tools/verify_art_import.py").read_text()
        self.assertIn("shutil.copyfile(src, destination)", import_tool)
        self.assertIn("source_sha256", import_tool)
        self.assertIn("Original Z and Nyra hero art not included", verify_tool)
        self.assertIn("Original source SHA mismatch", verify_tool)
        manifest = ROOT / "ART_IMPORT_MANIFEST.json"
        if manifest.is_file():
            import subprocess
            import sys
            result = subprocess.run([sys.executable, str(ROOT / "tools/verify_art_import.py")],
                                    cwd=str(ROOT.parent), capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_original_godot_project_not_copied_into_engine_source(self):
        self.assertFalse(list(ROOT.rglob("*.godot")))
        self.assertFalse(list(ROOT.rglob("*.tscn")))


if __name__ == "__main__":
    unittest.main()
