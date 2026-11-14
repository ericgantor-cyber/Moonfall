#!/usr/bin/env python3
"""Static regression contracts for UE5 racial persistence; native compile still required."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
INPUT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Config" / "DefaultInput.ini"

class TestRacialPersistence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.save = (ROOT / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        cls.input = INPUT.read_text(encoding="utf-8")

    def test_race_roster_and_input(self):
        self.assertIn("constexpr FMoonfallRaceProfile MoonfallRaces[]", self.cpp)
        roster = self.cpp.split("constexpr FMoonfallRaceProfile MoonfallRaces[]", 1)[1].split("};", 1)[0]
        for race in ("Human", "High Elf", "Wood Elf", "Dwarf", "Halfling",
                     "Orc", "Goblin", "Troll", "Entkin", "Shadowborn",
                     "Moonborn", "Draconid"):
            self.assertIn('TEXT("' + race + '")', roster)
        self.assertIn('ActionName="RacePower",Key=T', self.input)
        self.assertIn('ActionName="RacePower",Key=Gamepad_FaceButton_Top', self.input)
        self.assertIn("&AMoonfallHeroPawn::TriggerRacePower", self.cpp)
        self.assertIn("void TriggerRacePower() { ActivateRacePower(); }", self.header)

    def test_race_is_chosen_before_passive_or_active_bonuses(self):
        self.assertIn("if (bRaceChosen || NewRaceId < 0 || NewRaceId > 11", self.cpp)
        for method in ("GetRaceMightBonus", "GetRaceInsightBonus",
                       "GetRaceResolveBonus", "GetRaceStaminaRegenBonus",
                       "GetRaceManaRegenBonus"):
            self.assertIn(method + "() const { return bRaceChosen ?", self.cpp)
        self.assertIn("if (!bRaceChosen || !GetWorld()", self.cpp)

    def test_persistent_cooldown_and_save_migration(self):
        self.assertIn("int32 Version = 6", self.save)
        self.assertIn("int64 RacePowerReadyUtc", self.save)
        self.assertIn("FDateTime::UtcNow().ToUnixTimestamp()", self.cpp)
        self.assertIn("Save->RacePowerReadyUtc =", self.cpp)
        self.assertIn("Save->Version < 1 || Save->Version > 6", self.cpp)
        self.assertIn("Save->Version >= 3 && Save->RacePowerReadyUtc > NowUtc", self.cpp)
        self.assertIn("RacePowerReadyAt = GetWorld()", self.cpp)
        self.assertIn("if (!SaveWorldProgress()) {", self.cpp)
        self.assertIn("RacePowerReadyAt = PreviousReadyAt;", self.cpp)
        self.assertIn("RacePowerActiveUntil = PreviousActiveUntil;", self.cpp)
        self.assertIn("OnRacePowerActivated(FName(MoonfallRacePowerNames[RaceId]));", self.cpp)

    def test_malformed_saves_cannot_restore_unknown_quests_or_scan_unbounded_inventory(self):
        # Both are source contracts until a real UE5 save migration test is run.
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("IsNamedMoonfallResident(Entry.Key)", loader)
        self.assertIn("Entry.Value == 1 || Entry.Value == 2", loader)
        self.assertIn("Save->ResidentQuestStates.Num() <= 64", loader)
        self.assertIn("FMath::Min(Save->InventoryItemCodes.Num(), 10000)", loader)
        self.assertIn("Index < LoadScanCount", loader)
        self.assertIn("ValidMoonfallItemCode(Code)", loader)
        self.assertNotIn("for (int32 Code : Save->InventoryItemCodes)", loader)
        self.assertIn("bool IsNamedMoonfallResident(FName Id);", self.cpp)
        self.assertIn("bool IsNamedMoonfallResident(FName Id) {", self.cpp)

    def test_future_version_save_is_not_overwritten_by_autosave(self):
        self.assertIn("bool bPersistentSaveWritesBlocked = false;", self.header)
        save = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        load = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("if (bPersistentSaveWritesBlocked) return false;", save)
        self.assertLess(save.index("if (bPersistentSaveWritesBlocked) return false;"),
                        save.index("UGameplayStatics::SaveGameToSlot("))
        self.assertIn("Save->Version < 1 || Save->Version > 6", load)
        self.assertIn("bPersistentSaveWritesBlocked = true;", load)
        self.assertIn("bPersistentSaveWritesBlocked = false;", load)
        self.assertLess(load.index("bPersistentSaveWritesBlocked = true;"),
                        load.index("ResidentQuestStates.Reset();"))
        self.assertLess(load.index("bPersistentSaveWritesBlocked = false;"),
                        load.index("ResidentQuestStates.Reset();"))
        self.assertIn("LoadWorldProgress();", self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0])
        self.assertIn("if (SaveWorldProgress()) SecondsSinceLastWorldSave = 0.f;", self.cpp)

    def test_not_a_new_run(self):
        self.assertIn('MoonfallPersistentWorldV1', self.cpp)
        self.assertIn("if (SecondsSinceLastWorldSave >= 2.f && GetWorld()", self.cpp)
        self.assertIn("if (DefeatElapsedSeconds >= 3.f) RespawnAtCheckpoint();", self.cpp)
        self.assertIn("if (Health <= 0.f || !GetWorld()", self.cpp)
        self.assertIn("GetCharacterMovement()->IsFalling()", self.cpp)
        self.assertIn("GetActorLocation().ContainsNaN()", self.cpp)
        self.assertIn("DamageImmunityUntil = FMath::Max(DamageImmunityUntil, Now + 3.f)", self.cpp)
        self.assertIn("NextSafeCheckpointAt = Now + 5.f;", self.cpp)

if __name__ == "__main__":
    unittest.main()
