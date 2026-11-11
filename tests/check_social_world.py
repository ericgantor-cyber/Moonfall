#!/usr/bin/env python3
"""Source contracts for settlement NPCs, persistent factions and social response."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
class TestSocialWorld(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.npc=(ROOT/"MoonfallWorldNPC.cpp").read_text()
  cls.npc_h=(ROOT/"MoonfallWorldNPC.h").read_text()
  cls.hero=(ROOT/"MoonfallHeroPawn.cpp").read_text()
  cls.hero_h=(ROOT/"MoonfallHeroPawn.h").read_text()
  cls.save=(ROOT/"MoonfallPersistentSave.h").read_text()
  cls.arena=(ROOT/"MoonfallArena.cpp").read_text()
 def test_fixed_settlement(self):
  self.assertIn('SettlementId = FName(TEXT("Crossroads"))',self.arena)
  for resident in ("Birchroot","Anvilkeeper","Silverwatch","Ember Scout"):
   self.assertIn('TEXT("'+resident+'")',self.arena)
  self.assertIn("PrimaryActorTick.bCanEverTick = false;",self.npc)
 def test_social_reaction(self):
  self.assertIn("GetSocialDisposition",self.npc_h)
  self.assertIn("GetCurrentActivity()",self.npc_h)
  self.assertIn("FMath::FloorToInt",self.npc)
  self.assertIn("GetCurrentActivity() == TEXT",self.npc)
  for word in ("Hostile","Wary","Neutral","Friendly","Trusted"):
   self.assertIn('TEXT("'+word+'")',self.npc)
  self.assertIn("OnDialogueStarted(Hero, Reaction);",self.npc)
  self.assertIn("LineTraceSingleByChannel",self.npc)
 def test_all_twelve_races_and_ancestry_dialogue(self):
  names=("Alden","Starweaver","Leafwhisper","Anvilkeeper",
    "Pip Hearthfoot","Ember Scout","Geargrin","Mossfang",
    "Birchroot","Duskveil","Silverwatch","Ashscale")
  residents=self.arena.split("const FResident Residents[] = {",1)[1].split("};",1)[0]
  for name in names:
   self.assertIn('TEXT("'+name+'")',residents)
  for race in range(12):
   self.assertIn(", "+str(race)+", TEXT(", residents)
  self.assertIn("SharesRaceWith",self.npc_h)
  self.assertIn("Hero->bRaceChosen",self.npc)
  self.assertIn("Hero->RaceId == MoonfallRaces::Validated(RaceId)",self.npc)
  self.assertIn('TEXT("A fellow %s! %s")',self.npc)
  self.assertIn("!SharesRaceWith(Hero)",self.npc)
  for role in ("Scholar","Tinker","Elder"):
   self.assertIn('FName(TEXT("'+role+'"))',self.npc)
 def test_reputation_is_bounded_and_saved(self):
  self.assertIn("TMap<FName, int32> FactionReputation",self.hero_h)
  self.assertIn("TMap<FName, int32> FactionReputation",self.save)
  self.assertIn("Version = 4",self.save)
  self.assertIn("Save->FactionReputation = FactionReputation;",self.hero)
  self.assertIn("if (Save->Version >= 4)",self.hero)
  self.assertIn("FMath::Clamp(*Value, -100, 100)",self.hero)
  self.assertIn("if (!SaveWorldProgress())",self.hero)
  self.assertIn("FactionReputation.Remove(Faction)",self.hero)
if __name__=="__main__": unittest.main()
