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
            "bool AMoonfallHeroPawn::IsSanctuaryRoomUnlocked(", 1)[0]
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
    def test_completed_contract_npcs_acknowledge_durable_council_policy(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        method = npc.split("bool AMoonfallWorldNPC::TryTalk(", 1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(", 1
        )[0]
        for phrase in ("Hero->GetResidentQuestState(ResidentId) == 2",
                       "Hero->CouncilDecisionId >= 0",
                       "switch (Hero->CouncilDecisionId)",
                       "The council chose to protect our people.",
                       "The council funded rebuilding.",
                       "The council ordered stronger borders."):
            self.assertIn(phrase, method)
        for forbidden in ("ChangeFactionReputation(", "ChangeResidentBond(",
                          "SaveWorldProgress(", "SpawnActor("):
            self.assertNotIn(forbidden, method)

    def test_council_v13_commit_is_single_shot_and_save_rollback_safe(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        save = (SOURCE / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        inputs = (SOURCE.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("UPROPERTY() int32 Version = 13;", save)
        self.assertIn("UPROPERTY() int32 CouncilDecisionId = -1;", save)
        self.assertIn("Save->CouncilDecisionId = CouncilDecisionId;", hero)
        self.assertIn("Save->Version >= 13 ? Save->CouncilDecisionId : -1", hero)
        self.assertIn("Save->Version > 13", hero)
        self.assertIn("Save->CouncilDecisionId < -1", hero)
        self.assertIn("Save->CouncilDecisionId > 2", hero)
        self.assertIn("int32 CouncilDecisionId = -1;", header)
        self.assertIn('ActionName="CouncilConfirm",Key=F9', inputs)
        self.assertIn("ConfirmCouncilDecisionInput", header)
        self.assertIn("void AMoonfallHeroPawn::ConfirmCouncilDecisionInput()", hero)
        confirm = hero.split("bool AMoonfallHeroPawn::ConfirmCouncilDecision()", 1)[1].split(
            "void AMoonfallHeroPawn::CloseCouncilPointer()", 1
        )[0]
        self.assertIn("CouncilDecisionId != -1", confirm)
        self.assertIn("!bCouncilDecisionConfirmArmed", confirm)
        self.assertIn("bCouncilDecisionConfirmArmed = true;", confirm)
        self.assertIn("CouncilDecisionId = Selected;", confirm)
        self.assertIn("if (!SaveWorldProgress())", confirm)
        self.assertIn("CouncilDecisionId = OldDecision;", confirm)
        self.assertLess(confirm.index("CouncilDecisionId = Selected;"), confirm.index("if (!SaveWorldProgress())"))
        self.assertLess(confirm.index("if (!SaveWorldProgress())"), confirm.index("CouncilDecisionId = OldDecision;"))
        self.assertNotIn("FactionReputation.Add(", confirm)

    def test_council_pointer_click_preview_is_modal_and_safe(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (SOURCE.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="CouncilPointer",Key=F8', inputs)
        self.assertIn('BindAction(TEXT("CouncilPointer")', hero)
        self.assertIn("bool bCouncilPointerOpen = false;", header)
        toggle = hero.split("void AMoonfallHeroPawn::ToggleCouncilPointer()", 1)[1].split(
            "bool AMoonfallHeroPawn::SelectCouncilPreview(", 1
        )[0]
        self.assertIn("!IsSanctuaryRoomUnlocked(4)", toggle)
        self.assertIn("!IsNearStartingSanctuary()", toggle)
        self.assertIn("PC->bShowMouseCursor = true;", toggle)
        self.assertIn("bCouncilPointerOpen = false;", hero)
        self.assertIn("PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen || bAtlasOpen", hero)
        click = hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1].split(
            "void AMoonfallHeroPawn::TalkToNearestNPC()", 1
        )[0]
        mode = click.split("if (bCouncilPointerOpen) {", 1)[1].split(
            "if (!bResidentContextOpen) { CastLeft(); return; }", 1
        )[0]
        for expected in ("PC->bShowMouseCursor", "ViewW < 1280 || ViewH < 700",
                         "PointerY >= RowY && PointerY <= RowY + 25.f",
                         "SelectCouncilPreview(Choice);"):
            self.assertIn(expected, mode)
        self.assertNotIn("CastLeft();", mode)
        self.assertNotIn("SaveWorldProgress(", mode)
        self.assertIn("bCouncilPointerOpen && (Health <= 0.f || !IsNearStartingSanctuary()", hero)
        self.assertIn("bResidentContextOpen || bCouncilPointerOpen || !GetWorld()", hero)
        self.assertIn('if (bCouncilPointerOpen) { CloseCouncilPointer(); return; }', hero)
        self.assertIn('const TCHAR* ChoiceLabels[3]', hud)
        self.assertIn('LedgerY + 274.f, 98.f, 25.f', hud)

    def test_council_selection_is_controller_accessible_but_never_commits(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bool SelectCouncilPreview(int32 DecisionId);", header)
        self.assertIn("int32 CouncilPreviewSelection = 0;", header)
        selection = hero.split("bool AMoonfallHeroPawn::SelectCouncilPreview(int32 DecisionId)", 1)[1].split(
            "bool AMoonfallHeroPawn::AcceptResidentQuest(", 1
        )[0]
        for required in ("DecisionId < 0 || DecisionId > 2", "IsNearStartingSanctuary()",
                         "IsSanctuaryRoomUnlocked(4)", "CouncilPreviewSelection = DecisionId;"):
            self.assertIn(required, selection)
        for forbidden in ("SaveWorldProgress(", "ChangeFactionReputation(",
                          "FactionReputation.Add(", "UGameplayStatics::SaveGameToSlot("):
            self.assertNotIn(forbidden, selection)
        left = hero.split("void AMoonfallHeroPawn::OptionsLeft()", 1)[1].split(
            "void AMoonfallHeroPawn::OptionsRight()", 1
        )[0]
        right = hero.split("void AMoonfallHeroPawn::OptionsRight()", 1)[1].split(
            "void AMoonfallHeroPawn::AdjustOption(", 1
        )[0]
        self.assertIn("SelectCouncilPreview((CouncilPreviewSelection + 2) % 3)", left)
        self.assertIn("SelectCouncilPreview((CouncilPreviewSelection + 1) % 3)", right)
        self.assertIn("Hero->PreviewCouncilDecision(Selected)", hud)
        self.assertIn("D-pad: preview | F8: mouse | F9 twice: save", hud)

    def test_sanctuary_hud_uses_actual_progress_and_preserves_small_screens(self):
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        block = hud.split("// The sanctuary ledger is read-only:", 1)[1].split(
            "// Compact circular resource HUD at bottom left", 1
        )[0]
        for expected in ("bSanctuaryObjective", "Canvas->SizeX >= 1280.f",
                         "Canvas->SizeY >= 700.f", "!Hero->bAtlasOpen",
                         "!Hero->bOptionsOpen", "Hero->IsSanctuaryRoomUnlocked(Room)",
                         "Hero->GetWorldCommunityOutlook()",
                         "Hero->IsSanctuaryRoomUnlocked(4)",
                         "COUNCIL CHOICES (PREVIEW / CONFIRM)"):
            self.assertIn(expected, block)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "ChangeFactionReputation(",
                          "ResidentQuestStates.Add("):
            self.assertNotIn(forbidden, block)

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
            "FString AMoonfallHeroPawn::GetConfirmedCouncilPolicy() const", 1
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
