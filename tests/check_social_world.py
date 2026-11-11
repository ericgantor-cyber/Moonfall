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
  for word in ("Hostile","Wary","Neutral","Friendly","Trusted"):
   self.assertIn('TEXT("'+word+'")',self.npc)
  self.assertIn("OnDialogueStarted(Hero, Reaction);",self.npc)
  self.assertIn("LineTraceSingleByChannel",self.npc)
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
