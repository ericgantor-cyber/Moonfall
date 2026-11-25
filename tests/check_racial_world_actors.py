#!/usr/bin/env python3
"""Static contracts for racial NPCs and enemies. Not an Unreal runtime test."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
class RacialWorldActors(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.catalog=(ROOT/"MoonfallRaceCatalog.h").read_text()
  cls.npc_h=(ROOT/"MoonfallWorldNPC.h").read_text()
  cls.npc_c=(ROOT/"MoonfallWorldNPC.cpp").read_text()
  cls.enemy_h=(ROOT/"MoonfallEnemy.h").read_text()
  cls.enemy_c=(ROOT/"MoonfallEnemy.cpp").read_text()
  cls.arena=(ROOT/"MoonfallArena.cpp").read_text()
 def test_race_ids_stable(self):
  self.assertIn("static constexpr int32 Count = 12;",self.catalog)
  for race in ("Human","High Elf","Wood Elf","Dwarf","Halfling","Orc","Goblin","Troll","Entkin","Shadowborn","Moonborn","Draconid"):
   self.assertIn('TEXT("'+race+'")',self.catalog)
 def test_npc_race_and_dialogue(self):
  self.assertIn("int32 RaceId = 0",self.npc_h)
  self.assertIn("MoonfallRaces::Greeting(RaceId)",self.npc_c)
  self.assertIn("LineTraceSingleByChannel",self.npc_c)
  self.assertIn("OnDialogueStarted(Hero, Spoken)",self.npc_c)
 def test_enemy_identity_without_boss_art_change(self):
  self.assertIn("int32 RaceId = 0",self.enemy_h)
  self.assertIn("bBoss ? 1.f : MoonfallRaces::HealthScale(RaceId)",self.enemy_c)
  self.assertIn("bBoss ? 1.f : MoonfallRaces::DamageScale(RaceId)",self.enemy_c)
  self.assertIn("Enemy->RaceId = i % 12;",self.arena)
  self.assertIn("Guide->RaceId = 2;",self.arena)
if __name__=="__main__": unittest.main()
