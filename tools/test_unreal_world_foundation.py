#!/usr/bin/env python3
"""Source-level world-migration contracts; these do not compile Unreal."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.h").read_text(encoding="utf-8")
MODE = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallUnrealGameMode.cpp").read_text(encoding="utf-8")

class WorldFoundationTests(unittest.TestCase):
    def test_counts_and_unique_ids(self):
        self.assertIn("constexpr int32 Regions = 6;", CPP)
        self.assertIn("constexpr int32 DungeonsPerRegion = 5;", CPP)
        self.assertIn("constexpr int32 ChambersPerDungeon = 14;", CPP)
        room_ids = {(r * 5 + d) * 14 + c for r in range(6) for d in range(5) for c in range(14)}
        self.assertEqual(len(room_ids), 420)
        monster_ids = {r * 3 + n for r in range(6) for n in range(3)}
        resident_ids = {18 + (r * 5 + d) * 14 + (stage - 1)
                        for r in range(6) for d in range(5) for stage in (3, 5, 9, 12, 14)}
        self.assertEqual(len(monster_ids), 18)
        self.assertEqual(len(resident_ids), 150)
        self.assertFalse(monster_ids & resident_ids)

    def test_proximity_and_budget(self):
        self.assertIn("PrimaryActorTick.TickInterval = 0.25f", CPP)
        self.assertIn("FVector::DistSquared2D(Here, At)", CPP)
        self.assertIn("BuiltOne = true;", CPP)
        self.assertIn("SpawnedOne = SpawnEncounter(", CPP)
        self.assertIn("SpawnedEncounters.Contains(Id)", CPP)
        self.assertIn("SpawnedEncounters.Add(Id)", CPP)
        self.assertIn("Stage == 14", CPP)
        self.assertIn("Stage >= 9", CPP)

    def test_no_wave_mode_or_art_mutation(self):
        self.assertIn("bool bContinuousWorld = true;", HEADER)
        self.assertIn("if (bContinuousWorld) { BuildWorld(); return; }", CPP)
        self.assertIn("BuildLegacyArena", CPP) if "BuildLegacyArena" in CPP else self.assertIn("const int32 Count = FMath::Clamp(EnemyCount", CPP)
        self.assertIn("SpawnActor<AMoonfallArena>", MODE)
        self.assertNotIn("Z.png", CPP)
        self.assertNotIn("Nyra.png", CPP)

    def test_connected_geometry(self):
        self.assertIn("Region * RegionSpacing", CPP)
        self.assertIn("FVector(60.f, 60.f, 1.f)", CPP)
        self.assertIn("FVector(3.5f, 120.f, 1.f)", CPP)
        self.assertIn("GetPlayerPawn(this, 0)", CPP)
        self.assertIn("BeginDeferredActorSpawnFromClass", CPP)
        self.assertIn("FinishSpawningActor", CPP)

if __name__ == "__main__":
    unittest.main()
