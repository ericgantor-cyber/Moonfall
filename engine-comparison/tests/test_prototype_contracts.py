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
        self.assertIn("CastTimer = 1.15f;", self.cpp)
        self.assertIn("castTimer=1.15f;", self.unity)
        self.assertIn("CastCooldown = 3.5f;", self.cpp)
        self.assertIn("castCd=3.5f;", self.unity)
        self.assertIn("BossHealth - BasicAttackDamage()", self.cpp)
        self.assertIn("bossHp-BasicAttackDamage()", self.unity)

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
        self.assertIn("Stamina = FMath::Min(150.f, Stamina + 30.f * Dt)", self.cpp)
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

    def test_original_character_and_boss_art_are_connected(self):
        config = INPUT.read_text(encoding="utf-8")
        self.assertIn('Resources.Load<Texture2D>("MoonfallArt/heroes/Z_Original")', self.unity)
        self.assertIn('Resources.Load<Texture2D>("MoonfallArt/heroes/Nyra_Original")', self.unity)
        self.assertIn('Resources.Load<Texture2D>("MoonfallArt/v31_4_bestiary/boss_0")', self.unity)
        self.assertIn("Sprite.Create(original", self.unity)
        self.assertIn("SelectHero(", self.unity)
        self.assertIn("Input.GetKeyDown(KeyCode.Alpha1)", self.unity)
        self.assertIn("Input.GetKeyDown(KeyCode.Alpha2)", self.unity)
        self.assertIn("HeroPortrait->SetSprite(Chosen)", self.cpp)
        self.assertIn("BossPortrait->SetSprite(BossArtwork)", self.cpp)
        self.assertIn("Z_Original.Z_Original", self.cpp)
        self.assertIn("Nyra_Original.Nyra_Original", self.cpp)
        self.assertIn("boss_0.boss_0", self.cpp)
        self.assertIn("HeroPortrait->SetHiddenInGame(true)", self.cpp)
        self.assertIn('ActionName="SelectZArt",Key=One', config)
        self.assertIn('ActionName="SelectNyraArt",Key=Two', config)

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
        self.assertIn("else if (!won && Vector3.Distance(hero.position,boss.position)<4f)", self.unity)
        self.assertIn("if (Health <= 0.f || AttackCooldown > 0.f) return;", self.cpp)
        self.assertIn("else if (!bWon && FVector::Dist2D", self.cpp)
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

    def test_hero_class_combat_without_passive_kill_healing(self):
        self.assertIn("float BasicAttackDamage()", self.unity)
        self.assertIn("float ArcaneBoltDamage()", self.unity)
        self.assertIn("float HealingSpellAmount()", self.unity)
        self.assertIn("selectedHero == 0 ? 20f : 14f", self.unity)
        self.assertIn("selectedHero == 1", self.unity)
        self.assertIn("DamageEnemy(foe, BasicAttackDamage())", self.unity)
        self.assertIn("DamageEnemy(foe, ArcaneBoltDamage())", self.unity)
        self.assertIn("hp + HealingSpellAmount()", self.unity)
        self.assertIn("float AMoonfallPawn::BasicAttackDamage() const", self.cpp)
        self.assertIn("float AMoonfallPawn::ArcaneBoltDamage() const", self.cpp)
        self.assertIn("float AMoonfallPawn::HealingSpellAmount() const", self.cpp)
        self.assertIn("HeroArtIndex == 0 ? 20.f : 14.f", self.cpp)
        self.assertIn("DamageWorldEnemy(EnemyIndex, BasicAttackDamage())", self.cpp)
        self.assertIn("DamageWorldEnemy(EnemyIndex, ArcaneBoltDamage())", self.cpp)
        self.assertIn("Health + HealingSpellAmount()", self.cpp)

    def test_hero_selection_survives_missing_assets_and_restarts(self):
        save = SAVES.read_text(encoding="utf-8")
        self.assertIn("SelectedHeroIndex = 0;", save)
        self.assertIn("Progress->SelectedHeroIndex = HeroArtIndex;", self.cpp)
        self.assertIn("HeroArtIndex = FMath::Clamp(Progress->SelectedHeroIndex, 0, 1);", self.cpp)
        self.assertIn("ApplyHeroArtwork(HeroArtIndex);", self.cpp)
        self.assertIn("HeroPortrait->SetHiddenInGame(Chosen == nullptr);", self.cpp)
        self.assertIn("HeroMesh->SetHiddenInGame(Chosen != nullptr);", self.cpp)
        self.assertIn("ChooseNyra() { ApplyHeroArtwork(1); SaveProgress(); }", self.cpp)
        self.assertIn("selectedHero = index;", self.unity)
        self.assertIn("heroPortrait = selected != null ? AttachPortrait", self.unity)
        self.assertIn('PlayerPrefs.SetInt("moonfall_experiment_hero_art", selectedHero);', self.unity)

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
