"""Source contracts for living-world NPC save wiring (not native UE5 tests)."""
from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class LivingWorldNPCWiringTests(unittest.TestCase):
    def test_saved_bonds_drive_readonly_relationship_rank(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn('#include "MoonfallLivingWorldRules.h"', hero)
        self.assertIn("FString GetResidentRelationshipRank(FName ResidentId) const;", header)
        rank = hero.split("FString AMoonfallHeroPawn::GetResidentRelationshipRank(", 1)[1].split(
            "bool AMoonfallHeroPawn::ChangeResidentBond(", 1
        )[0]
        self.assertIn("MoonfallLivingWorld::RelationshipRank(GetResidentBond(ResidentId))", rank)
        for label in ("Hostile", "Wary", "Neutral", "Trusted", "Devoted"):
            self.assertIn('TEXT("' + label + '")', rank)
        self.assertNotIn("SaveWorldProgress(", rank)
        self.assertIn("Save->ResidentBonds = ResidentBonds;", hero)

    def test_community_outlook_uses_persisted_quest_and_shrine_ledgers(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("FString GetWorldCommunityOutlook() const;", header)
        outlook = hero.split("FString AMoonfallHeroPawn::GetWorldCommunityOutlook() const", 1)[1].split(
            "bool AMoonfallHeroPawn::AcceptResidentQuest(", 1)[0]
        for expected in (
            "for (const auto& Entry : ResidentQuestStates)",
            "if (Entry.Value == 2) ++CompletedContracts;",
            "Progress.completedContracts = CompletedContracts;",
            "Progress.shrineDiscovered = !DiscoveredShrineIds.IsEmpty();",
            "MoonfallLivingWorld::SettlementStatus(Progress)",
            "Endangered", "Recovering", "Flourishing",
        ):
            self.assertIn(expected, outlook)
        for forbidden in ("SaveWorldProgress(", "GetAllActorsOfClass(", "SpawnActor(",
                          "ResidentQuestStates.Add(", "DiscoveredShrineIds.Add("):
            self.assertNotIn(forbidden, outlook)
        self.assertIn("Save->ResidentQuestStates = ResidentQuestStates;", hero)
        self.assertIn("Save->DiscoveredShrineIds = DiscoveredShrineIds;", hero)
    def test_weapon_resonance_reads_valid_equipped_gear_and_saved_boss_mask(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("FString GetEquippedWeaponResonance() const;", header)
        method = hero.split("FString AMoonfallHeroPawn::GetEquippedWeaponResonance() const", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        for required in ("InventoryItemCodes.Contains(EquippedWeaponCode)",
                         "DecodeEquippedItem(EquippedWeaponCode, 0, Trait, Rarity, Tier)",
                         "DefeatedDungeonBossMask",
                         "MoonfallLivingWorld::BossResonance(DefeatedBosses, Tier, Rarity)",
                         "Victories &= Victories - 1u;",
                         "Awakened", "Ascendant", "Mythic"):
            self.assertIn(required, method)
        for forbidden in ("SaveWorldProgress(", "InventoryItemCodes.Add(", "EquippedWeaponCode ="):
            self.assertNotIn(forbidden, method)
        self.assertIn("Hero->GetEquippedWeaponResonance()", hud)
        self.assertIn("Save->DefeatedDungeonBossMask = DefeatedDungeonBossMask;", hero)
    def test_dialogue_reflects_completed_contract_and_never_grants_bond(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        section = npc.split("bool AMoonfallWorldNPC::TryTalk(", 1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(", 1
        )[0]
        self.assertIn("GetResidentQuestState(ResidentId) == 2", section)
        self.assertIn("GetResidentRelationshipRank(ResidentId)", section)
        self.assertIn('Rank == TEXT("Trusted")', section)
        self.assertIn('Rank == TEXT("Devoted")', section)
        self.assertNotIn("ChangeResidentBond(", section)
        self.assertNotIn("SaveWorldProgress(", section)


if __name__ == "__main__":
    unittest.main()
