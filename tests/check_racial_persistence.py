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
        self.assertIn("int32 Version = 3", self.save)
        self.assertIn("int64 RacePowerReadyUtc", self.save)
        self.assertIn("FDateTime::UtcNow().ToUnixTimestamp()", self.cpp)
        self.assertIn("Save->RacePowerReadyUtc =", self.cpp)
        self.assertIn("Save->Version < 1 || Save->Version > 3", self.cpp)
        self.assertIn("Save->Version >= 3 && Save->RacePowerReadyUtc > NowUtc", self.cpp)
        self.assertIn("RacePowerReadyAt = GetWorld()", self.cpp)
        self.assertIn("if (!SaveWorldProgress()) {", self.cpp)
        self.assertIn("RacePowerReadyAt = PreviousReadyAt;", self.cpp)
        self.assertIn("RacePowerActiveUntil = PreviousActiveUntil;", self.cpp)
        self.assertIn("OnRacePowerActivated(FName(MoonfallRacePowerNames[RaceId]));", self.cpp)

    def test_not_a_new_run(self):
        self.assertIn('MoonfallPersistentWorldV1', self.cpp)
        self.assertIn("if (SecondsSinceLastWorldSave >= 2.f)", self.cpp)
        self.assertIn("if (DefeatElapsedSeconds >= 3.f) RespawnAtCheckpoint();", self.cpp)
        self.assertIn("if (Health <= 0.f || !GetWorld()", self.cpp)

if __name__ == "__main__":
    unittest.main()
