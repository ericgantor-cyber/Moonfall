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
  cls.input=(ROOT.parents[1]/"Config"/"DefaultInput.ini").read_text()
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
 def test_personal_bonds_persist_and_are_not_farmable(self):
  self.assertIn("TMap<FName, int32> ResidentBonds",self.hero_h)
  self.assertIn("TMap<FName, int32> ResidentBonds",self.save)
  self.assertIn("Save->ResidentBonds = ResidentBonds;",self.hero)
  self.assertIn("if (Save->Version >= 5)",self.hero)
  self.assertIn("ResidentBonds.Remove(ResidentId)",self.hero)
  self.assertIn("Hero->GetResidentBond(ResidentId) != 0",self.npc)
  self.assertIn("GetBondWithHero(Hero)",self.npc)
  self.assertIn("ResidentId = FName",self.arena)
 def test_racial_contract_save_and_atomic_rewards(self):
  self.assertIn("TMap<FName, int32> ResidentQuestStates",self.hero_h)
  self.assertIn("TMap<FName, int32> ResidentQuestStates",self.save)
  self.assertIn("Save->ResidentQuestStates = ResidentQuestStates;",self.hero)
  self.assertIn("Save->Version >= 6",self.hero)
  self.assertIn("GetResidentQuestState",self.hero)
  self.assertIn("if (!SaveWorldProgress())",self.hero)
  self.assertIn("ResidentQuestStates.Add(Id, 2);",self.hero)
  self.assertIn("ResidentQuestStates.Add(Id, 1);",self.hero)
  self.assertIn("Salvage = PreviousSalvage;",self.hero)
  self.assertIn("FactionReputation.Add(Faction",self.hero)
  self.assertIn("ResidentBonds.Add(Id",self.hero)
  self.assertIn("GetContractSalvageCost",self.npc)
  self.assertIn("CanHeroInteract(Hero)",self.npc)
  self.assertIn("Hero->GetResidentQuestState(ResidentId) == 1)",self.npc)
 def test_native_canvas_menu_and_input_safety(self):
  hud=(ROOT/"MoonfallCombatHUD.cpp").read_text()
  self.assertIn("Hero->FocusedResident.Get()",hud)
  self.assertIn("Hero->ResidentContextSelection",hud)
  self.assertIn("Accept contract",hud)
  self.assertIn("Turn in salvage",hud)
  self.assertIn("ResidentFeedbackUntil",hud)
  self.assertIn("void AMoonfallHeroPawn::ResidentContextMouseClick()",self.hero)
  self.assertIn("PC->GetMousePosition(MouseX, MouseY)",self.hero)
  self.assertIn("ResidentContextSelection = Row;",self.hero)
  self.assertIn("ChooseResidentContextAction(Row)",self.hero)
  self.assertIn("ChooseResidentContextAction(ResidentContextSelection)",self.hero)
  self.assertIn("bResidentContextOpen || !GetWorld() || School < 0",self.hero)
  self.assertIn('ActionName="ResidentMouseClick",Key=LeftMouseButton',self.input)
 def test_modal_cleanup_after_npc_streamout(self):
  self.assertIn("bool bResidentContextOpen = false",self.hero_h)
  self.assertIn("if (!bResidentContextOpen && !FocusedResident.IsValid()) return;",self.hero)
  self.assertIn("bResidentContextOpen = false;",self.hero)
  self.assertIn("PC->bShowMouseCursor = bResidentCursorWasVisible;",self.hero)
  self.assertIn("if (bResidentContextOpen &&",self.hero)
  self.assertIn("!FocusedResident.IsValid() || !FocusedResident->CanHeroInteract(this)",self.hero)
  self.assertIn("!bResidentContextOpen && FMath::IsFinite(Value)",self.hero)
  self.assertIn("bResidentContextOpen || !GetWorld() || School < 0",self.hero)
 def test_no_n_v_quest_bindings(self):
  self.assertNotIn('ActionName="TalkNPC"',self.input)
  self.assertNotIn('ActionName="ResidentQuest"',self.input)
  self.assertIn('ActionName="CastRightMouse",Key=RightMouseButton',self.input)
  self.assertIn('ActionName="ResidentContext",Key=Gamepad_DPad_Up',self.input)
  self.assertNotIn('ActionName="CastRight",Key=RightMouseButton',self.input)
 def test_contextual_npc_controls(self):
  self.assertIn("GetNearestResidentNPC()",self.hero)
  self.assertIn("&AMoonfallHeroPawn::CastRightMouse",self.hero)
  self.assertIn("&AMoonfallHeroPawn::OpenFocusedResidentContext",self.hero)
  self.assertIn("void AMoonfallHeroPawn::CastRightMouse()",self.hero)
  self.assertIn("LineTraceSingleByChannel",self.hero)
  self.assertIn("GetPlayerViewPoint",self.hero)
  self.assertIn("CastRight();",self.hero)
  self.assertIn("ChooseResidentContextAction(int32 Choice)",self.hero)
  self.assertIn("Resident->OfferResidentQuest(this)",self.hero)
  self.assertIn("Resident->TurnInResidentQuest(this)",self.hero)
  self.assertIn("if (Salvage < Resident->GetContractSalvageCost())",self.hero)
  self.assertIn("OnResidentContextOpened",self.hero_h)
  self.assertIn("OnResidentQuestFeedback",self.hero_h)
  self.assertIn("Select Accept in our conversation",self.npc)
  self.assertIn("Select Turn In when ready",self.npc)
 def test_reputation_is_bounded_and_saved(self):
  self.assertIn("TMap<FName, int32> FactionReputation",self.hero_h)
  self.assertIn("TMap<FName, int32> FactionReputation",self.save)
  self.assertIn("Version = 6",self.save)
  self.assertIn("Save->FactionReputation = FactionReputation;",self.hero)
  self.assertIn("if (Save->Version >= 4)",self.hero)
  self.assertIn("FMath::Clamp(*Value, -100, 100)",self.hero)
  self.assertIn("if (!SaveWorldProgress())",self.hero)
  self.assertIn("FactionReputation.Remove(Faction)",self.hero)
if __name__=="__main__": unittest.main()
