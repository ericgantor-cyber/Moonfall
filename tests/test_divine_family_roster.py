"""Source-level regression checks for Moonfall divine-family presentation.

Run: python -m unittest tests.test_divine_family_roster
These checks do not imply a successful Unreal/Godot compile or playtest.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class DivineFamilyRosterTests(unittest.TestCase):
    def test_godot_new_game_lore_has_three_divine_members(self):
        text = read("godot-v38/scripts/native_v11.gd")
        for label in ("Z • Void Reaper", "Nyra • Eclipse Queen",
                      "Morvella Velmora • Goddess of Death"):
            self.assertIn(label, text)
        self.assertIn("these gods are not playable", text)

    def test_unreal_optional_boss_preset_keeps_existing_gods(self):
        arena = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp")
        identity = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        self.assertIn('TEXT("z_void_reaper")', arena)
        self.assertIn('TEXT("nyra_eclipse_queen")', arena)
        self.assertIn('TEXT("death_goddess_crimson_reaper")', identity)
        self.assertIn("bSingleDivineEncounter", arena)
        self.assertIn("bDeathGoddessEncounter", arena)

    def test_unreal_hud_uses_divine_title_and_four_phase_total(self):
        hud = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp")
        self.assertIn("Boss->DivineBossTitle", hud)
        self.assertIn('TEXT("Morvella Velmora")', read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp"))
        self.assertIn("GODS OF MOONFALL", hud)
        self.assertIn("MoonfallDeathGoddess::IsDeathGoddess", hud)
        self.assertIn(" ? 4 : 3", hud)

    def test_morvella_four_signature_moves_use_existing_damage_pipeline(self):
        enemy = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")
        rules = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        names = ("GraveNeedle", "SoulHarvest", "CrimsonEclipse", "FinalVerdict")
        for name in names:
            self.assertIn('TEXT("' + name + '")', rules)
        self.assertIn("MoonfallDeathGoddess::SignatureForPhase(BossPhase)", enemy)
        self.assertIn("MoonfallDeathGoddess::WarningForPhase(BossPhase)", enemy)
        self.assertIn("OnDivineAttackTelegraphed(ActiveDivineMoveId", enemy)
        self.assertIn("OnDivineAttackResolved(ResolvedMove", enemy)
        self.assertIn("CancelDivineSignature()", enemy)
        self.assertIn("(bVoidReaper || bEclipseQueen || bDeathGoddess)", enemy)
        # Phase two is caster-centered and should not be attempted from ranged casting distance.
        self.assertIn("SignatureForPhase(BossPhase).bCenteredOnCaster", enemy)
        self.assertIn("CurrentWindup = Move.WarningSeconds", enemy)
        self.assertIn("CurrentAttackMultiplier = Move.DamageMultiplier", enemy)

    def test_distinct_morvella_hit_shapes_are_wired(self):
        enemy = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")
        header = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        footprint = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddessFootprint.h")
        self.assertIn("HitsSignature(ActiveDivineMoveId, DivineSignatureOrigin", enemy)
        self.assertIn("DivineSignatureOrigin = GetActorLocation()", enemy)
        self.assertIn("FootprintForMove(FName Move)", header)
        self.assertIn("SoulHarvestSafeFraction = 0.40f", footprint)
        self.assertIn("EclipseCrossHalfWidthFraction = 0.20f", footprint)
        self.assertIn('TEXT("SAFE CENTER")', enemy)
        for shape in ("GraveNeedle", "SoulHarvest", "CrimsonEclipse", "FinalVerdict"):
            self.assertIn("Shape::" + shape, footprint)

    def test_world_spawn_connects_necropolis_and_preserves_existing_gods(self):
        arena = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp")
        header = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.h")
        self.assertIn("bool bSpawnMorvellaInWorld = true;", header)
        self.assertIn("MorvellaWorldActorClass", header)
        self.assertIn("SpawnMorvellaWorldEncounter()", arena)
        self.assertIn("bEnableStartingSanctuary && !bDeathGoddessEncounter", arena)
        self.assertIn("constexpr float X = 16875.f;", arena)
        self.assertIn("FVector(X, -1050.f, -20.f), FVector(5.5f, 34.f, 0.4f)", arena)
        self.assertIn("FVector(X, 850.f, -20.f), FVector(20.f, 18.f, 0.4f)", arena)
        self.assertIn("Morvella->DivineBossId = MoonfallDeathGoddess::BossId();", arena)
        self.assertIn("Morvella->Tags.AddUnique(FName(TEXT(\"MoonfallMorvellaWorldBoss\")))", arena)
        self.assertIn("Morvella->Health = 340.f;", arena)
        self.assertIn("UGameplayStatics::FinishSpawningActor(Morvella, SpawnTransform)", arena)
        # The public road is Y -2500 with 450cm half-width. The new
        # 3400cm walkway and 1800cm-wide necropolis platform overlap it.
        road_min, road_max = -2950, -2050
        walkway_min, walkway_max = -1050 - 1700, -1050 + 1700
        arena_min, arena_max = 850 - 900, 850 + 900
        self.assertLessEqual(walkway_min, road_max)
        self.assertGreaterEqual(walkway_max, road_min)
        self.assertLessEqual(arena_min, walkway_max)
        self.assertGreaterEqual(arena_max, walkway_min)
        self.assertIn('TEXT("z_void_reaper")', arena)
        self.assertIn('TEXT("nyra_eclipse_queen")', arena)

    def test_morvella_victory_survives_old_save_schema_and_load_order(self):
        save = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallPersistentSave.h")
        hero_header = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.h")
        hero = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp")
        enemy = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")
        arena = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp")
        self.assertIn("int32 Version = 12;", save)
        self.assertIn("bool bMorvellaDefeated = false;", save)
        self.assertIn("bool MarkMorvellaDefeated(AMoonfallEnemy* DefeatedBoss);", hero_header)
        self.assertIn("Save->bMorvellaDefeated = bMorvellaDefeated;", hero)
        self.assertIn("bMorvellaDefeated = Save->bMorvellaDefeated;", hero)
        self.assertIn("MarkMorvellaDefeated(this);", enemy)
        self.assertIn("OnMorvellaVictory();", hero)
        self.assertIn("if (!SaveWorldProgress()) {", hero)
        self.assertIn("bMorvellaDefeated = false;", hero)
        self.assertIn("MoonfallDeathGoddess::IsDeathGoddess(Enemy->DivineBossId)", hero)
        self.assertIn("if (Hero->HasDefeatedMorvella()) return;", arena)
        self.assertIn("TActorIterator<AMoonfallEnemy>", hero)

    def test_phase_thresholds_and_stable_divine_roles(self):
        rules = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddess.h")
        enemy = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp")
        for threshold in ("0.75f", "0.50f", "0.25f"):
            self.assertIn(threshold, rules)
        self.assertIn("PhaseForHealthRatio(HealthRatio)", enemy)
        self.assertIn('TEXT("z_void_reaper")', enemy)
        self.assertIn('TEXT("nyra_eclipse_queen")', enemy)


if __name__ == "__main__":
    unittest.main()
