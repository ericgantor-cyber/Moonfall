#!/usr/bin/env python3
"""NPC/enemy behavior source contracts; UE5 native compilation still required."""
from pathlib import Path
import unittest

BASE = Path(__file__).resolve().parents[1]
SRC = BASE / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class NPCEnemySourceChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.npc_h = (SRC / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        cls.npc_cpp = (SRC / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.arena = (SRC / "MoonfallArena.cpp").read_text(encoding="utf-8")
        cls.input = (BASE / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")

    def test_dialogue_is_range_and_sight_gated(self):
        self.assertIn("bool AMoonfallWorldNPC::TryTalk", self.npc_cpp)
        self.assertIn("Hero->Health <= 0.f", self.npc_cpp)
        self.assertIn("FVector::DistSquared(From, To) > FMath::Square(Range)", self.npc_cpp)
        self.assertIn("LineTraceSingleByChannel", self.npc_cpp)
        self.assertIn("if (bHit && Hit.GetActor() != this) return false;", self.npc_cpp)
        self.assertIn("OnDialogueStarted(Hero, Greeting);", self.npc_cpp)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", self.npc_cpp)

    def test_npc_can_be_interacted_with(self):
        self.assertIn('ActionName="TalkNPC",Key=N', self.input)
        self.assertIn("&AMoonfallHeroPawn::TalkToNearestNPC", self.hero)
        self.assertIn("Checked < 64", self.hero)
        self.assertIn("Nearest->TryTalk(this);", self.hero)
        self.assertIn("SpawnActor<AMoonfallWorldNPC>", self.arena)

    def test_struck_enemies_investigate_without_omniscience(self):
        self.assertIn("if (!bBoss && ActualDamage > 0.f && GetWorld())", self.enemy)
        self.assertIn("Attacker->Health > 0.f", self.enemy)
        self.assertIn("FMath::Square(1800.f)", self.enemy)
        self.assertIn("LastKnownHeroPosition = Attacker->GetActorLocation();", self.enemy)
        self.assertIn("Now - LastSawHeroAt <= 2.5f", self.enemy)

if __name__ == "__main__":
    unittest.main()
