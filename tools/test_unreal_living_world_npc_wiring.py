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
    def test_sanctuary_unlock_uses_saved_firstlight_and_proven_victories(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool IsSanctuaryRoomUnlocked(int32 RoomId) const;", header)
        method = hero.split("bool AMoonfallHeroPawn::IsSanctuaryRoomUnlocked(int32 RoomId) const", 1)[1].split(
            "FString AMoonfallHeroPawn::PreviewCouncilDecision(", 1
        )[0]
        for expected in ('HasDiscoveredShrine(FName(TEXT("sanctuary_firstlight")))',
                         "GetTotalDefeatedDungeonBosses()", "Entry.Value == 2",
                         "MoonfallLivingWorld::SanctuaryRoomUnlocked(",
                         "RoomId > static_cast<int32>(MoonfallLivingWorld::SanctuaryRoom::Portal)"):
            self.assertIn(expected, method)
        self.assertNotIn("SaveWorldProgress(", method)
        self.assertNotIn("SpawnActor(", method)

    def test_council_choice_is_readonly_preview(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("FString PreviewCouncilDecision(int32 DecisionId) const;", header)
        method = hero.split("FString AMoonfallHeroPawn::PreviewCouncilDecision(int32 DecisionId) const", 1)[1].split(
            "bool AMoonfallHeroPawn::AcceptResidentQuest(", 1
        )[0]
        for required in ("MoonfallLivingWorld::Decide(Decision)", "Protect Residents",
                         "Fund Reconstruction", "Fortify Borders", "preview only",
                         "Invalid decision"):
            self.assertIn(required, method)
        for forbidden in ("SaveWorldProgress(", "ChangeFactionReputation(",
                          "FactionReputation.Add(", "ResidentQuestStates.Add("):
            self.assertNotIn(forbidden, method)

    def test_mixed_school_cast_is_single_transactional_hit(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cast = hero.split("void AMoonfallHeroPawn::CastBoth()", 1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()", 1)[0]
        self.assertIn("void OnSpellWeaved(int32 LeftSchool, int32 RightSchool, int32 HybridType);", header)
        for required in ("LeftSpell != RightSpell", "MoonfallLivingWorld::Weave(LeftSpell, RightSpell)",
                         "Mana < RequiredMana", "TryCast(LeftSpell, 0, 1.25f)",
                         "Mana -= Costs[RightSpell] * 0.5f;",
                         "LastCastTime[1] = LastCastTime[0];",
                         "OnSpellWeaved(LeftSpell, RightSpell, static_cast<int32>(Pair))",
                         "if (TryCast(LeftSpell, 0, 2.f))"):
            self.assertIn(required, cast)
        self.assertLess(cast.index("Mana < RequiredMana"), cast.index("TryCast(LeftSpell, 0, 1.25f)"))
        self.assertLess(cast.index("TryCast(LeftSpell, 0, 1.25f)"), cast.index("Mana -= Costs[RightSpell] * 0.5f;"))
        self.assertLess(cast.index("Mana -= Costs[RightSpell] * 0.5f;"), cast.index("OnSpellWeaved("))
        self.assertNotIn("TryCast(RightSpell", cast)
        self.assertNotIn("SaveWorldProgress(", cast)
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
