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
  # Ensure quest status never overrides a hostile faction response.
  self.assertIn('if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 1)',self.npc)
  self.assertIn('else if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 2)',self.npc)
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
 def test_native_spoken_dialogue_without_blueprint_widget(self):
  self.assertIn("LastSpokenLine",self.npc_h)
  self.assertIn("LastSpokenLine = Reaction;",self.npc)
  self.assertIn("ResidentFeedbackMessage = Resident->LastSpokenLine.ToString();",self.hero)
  self.assertIn("GetTimeSeconds() + 8.f",self.hero)
 def test_modal_cleanup_after_npc_streamout(self):
  self.assertIn("bool bResidentContextOpen = false",self.hero_h)
  self.assertIn("if (!bResidentContextOpen && !FocusedResident.IsValid()) return;",self.hero)
  self.assertIn("bResidentContextOpen = false;",self.hero)
  self.assertNotIn("bResidentCursorWasVisible",self.hero)
  close=self.hero.split("void AMoonfallHeroPawn::CloseResidentContext()",1)[1].split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[0]
  self.assertIn("PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen || bAtlasOpen;",close)
  self.assertNotIn("PC->bShowMouseCursor = false;",close)
  self.assertIn("if (bResidentContextOpen) {",self.hero)
  self.assertIn("const AMoonfallWorldNPC* Resident = FocusedResident.Get();",self.hero)
  self.assertIn("if (!IsValid(Resident) || Health <= 0.f || bOptionsOpen || bInventoryOpen ||",self.hero)
  self.assertIn("FVector::DistSquared(GetActorLocation(), Resident->GetActorLocation()) > FMath::Square(Range)",self.hero)
  self.assertIn("!bResidentContextOpen && FMath::IsFinite(Value)",self.hero)
  self.assertIn("bResidentContextOpen || !GetWorld() || School < 0",self.hero)
 def test_npc_dialogue_blocks_stale_controller_and_keyboard_input(self):
  # This verifies native source guard ordering; it does not run UE5 gameplay.
  def body(name,next_name):
   return self.hero.split("void AMoonfallHeroPawn::"+name+"()",1)[1].split(
       "void AMoonfallHeroPawn::"+next_name+"()",1)[0]
  jump=body("TryJump","MoveForward")
  modal=jump.split("if (bResidentContextOpen) {",1)[1].split(
      "// Jump must obey",1)[0]
  self.assertIn("if (!FocusedResident.IsValid()) {",modal)
  self.assertLess(modal.index("CloseResidentContext();"),modal.index(
      "ChooseResidentContextAction(ResidentContextSelection);"))
  self.assertNotIn("Jump();",modal)
  for name,next_name in (("OptionsUp","OptionsDown"),("OptionsDown","OptionsLeft")):
   nav=body(name,next_name)
   self.assertIn("if (bResidentContextOpen) {",nav)
   self.assertIn("!FocusedResident->CanHeroInteract(this)",nav)
   self.assertLess(nav.index("CloseResidentContext();"),nav.index(
       "ResidentContextSelection ="))
   self.assertIn("AtlasChronicleChapter = -1;",nav)
  npc=body("OpenFocusedResidentContext","CastRightMouse")
  self.assertLess(npc.index("CloseResidentContext();"),npc.index(
      "AMoonfallWorldNPC* Resident = GetNearestResidentNPC();"))
  self.assertIn("!FocusedResident->CanHeroInteract(this)",npc)
  right=self.hero.split(
      "void AMoonfallHeroPawn::CastRightMouse()",1)[1].split(
      "void AMoonfallHeroPawn::OptionsUp()",1)[0]
  self.assertIn("if (bResidentContextOpen) { CloseResidentContext(); return; }",right)
  for name in ("MoveForward","MoveRight"):
   movement=self.hero.split("void AMoonfallHeroPawn::"+name+"(float Value)",1)[1].split("return;",1)[0]
   self.assertIn("bResidentContextOpen",movement)
  for name,next_name in (("Dodge","RewardPerfectDodge"),
                         ("BeginGuard","EndGuard"),("BeginSprint","EndSprint")):
   guard=body(name,next_name).split("return;",1)[0]
   self.assertIn("bResidentContextOpen",guard)
  close=body("CloseResidentContext","OpenFocusedResidentContext")
  self.assertIn("bOptionsOpen || bInventoryOpen || bAtlasOpen;",close)
  self.assertIn('#include "MoonfallChronicleUI.h"',self.hero)
 def test_both_npc_dialogue_openers_clear_dodge_and_enemy_lock(self):
  # Controller and right-click NPC entry have the same combat cleanup.
  # Preserve the original contract: dialogue never forcibly stops movement.
  controller=self.hero.split(
      "void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
      "void AMoonfallHeroPawn::CastRightMouse()",1)[0]
  mouse=self.hero.split(
      "void AMoonfallHeroPawn::CastRightMouse()",1)[1].split(
      "void AMoonfallHeroPawn::ReleaseInventoryPointer()",1)[0]
  for source,entry_guard in (
      (controller,"if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) {"),
      (mouse,"if (Resident->CanHeroInteract(this)) {")):
   self.assertIn(entry_guard,source)
   entry=source.split(entry_guard,1)[1].split("bResidentContextOpen = true;",1)[0]
   for cleanup in ("QueuedDodgeUntil = -1.f;",
                   "QueuedDodgeDirection = FVector::ZeroVector;",
                   "LockedEnemy.Reset();", "LockOccludedSince = -1.f;"):
    self.assertIn(cleanup,entry)
    self.assertLess(entry.index(cleanup),entry.index("FocusedResident = Resident;"))
   for forbidden in ("StopJumping();","StopMovementImmediately();",
                     "EndSprint();","EndGuard();"):
    self.assertNotIn(forbidden,entry)

 def test_dpad_up_prioritizes_resident_and_falls_back_to_lock_on(self):
  # A single controller action navigates overlays, cycles NPC choices,
  # opens reachable NPC dialogue, or toggles combat lock when no NPC is near.
  up=self.hero.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
      "void AMoonfallHeroPawn::CastRightMouse()",1)[0]
  self.assertIn("if (bOptionsOpen || bInventoryOpen || bAtlasOpen) { OptionsUp(); return; }",up)
  self.assertIn("if (!GetWorld() || Health <= 0.f || !FMath::IsFinite(Health)) return;",up)
  self.assertLess(up.index("if (bResidentContextOpen) {"),up.index("GetNearestResidentNPC()"))
  self.assertIn("if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) {",up)
  fallback=up.split("if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) {",1)[1].split("}",1)[0]
  self.assertIn("ToggleLockOn();",fallback)
  self.assertLess(fallback.index("ToggleLockOn();"),fallback.index("return;"))
  self.assertLess(up.index("ToggleLockOn();"),up.index("bResidentContextOpen = true;"))
  lock=self.hero.split("void AMoonfallHeroPawn::ToggleLockOn()",1)[1].split(
      "\n}",1)[0]
  for guard in ("Health <= 0.f","!FMath::IsFinite(Health)","bInventoryOpen",
                "bAtlasOpen","bResidentContextOpen","FocusedResident.IsValid()"):
   self.assertIn(guard,lock)
  self.assertIn('ActionName="ResidentContext",Key=Gamepad_DPad_Up',self.input)
  self.assertIn('ActionName="LockOn",Key=Gamepad_DPad_Down',self.input)

 def test_nearest_npc_skips_occluded_candidates(self):
  self.assertIn("Candidate->CanHeroInteract(this)",self.hero)
  self.assertIn("Checked < 64",self.hero)
  self.assertIn("if (FMath::IsFinite(DistSq) && DistSq < BestDistSq &&",self.hero)
 def test_npc_menu_uses_actual_interaction_distance(self):
  hud=(ROOT/"MoonfallCombatHUD.cpp").read_text()
  self.assertIn("FMath::Clamp(Resident->InteractRange, 50.f, 400.f)",self.hero)
  self.assertIn("FMath::Square(Range)",self.hero)
  self.assertIn("FMath::Clamp(Resident->InteractRange, 50.f, 400.f)",hud)
  self.assertIn("FMath::Square(InteractionRange)",hud)
  self.assertIn("!CanHeroInteract(Hero)",self.npc)
  self.assertNotIn("FMath::Square(FMath::Clamp(InteractRange, 50.f, 400.f))",self.npc)
 def test_turn_in_requires_two_separate_choices(self):
  hud=(ROOT/"MoonfallCombatHUD.cpp").read_text()
  self.assertIn("bResidentTurnInPending",self.hero_h)
  self.assertIn("ResidentTurnInConfirmUntil",self.hero_h)
  self.assertIn("ResidentTurnInConfirmAfter",self.hero_h)
  self.assertIn("GetTimeSeconds() + 6.f",self.hero)
  self.assertIn("GetTimeSeconds() + 0.35f",self.hero)
  self.assertIn("GetTimeSeconds() < ResidentTurnInConfirmAfter",self.hero)
  self.assertIn("Confirm spending %d salvage",self.hero)
  self.assertIn("Confirm turn-in",hud)
  self.assertIn("Resident->TurnInResidentQuest(this)",self.hero)
 def test_opening_dialogue_does_not_force_stop_movement(self):
  source=self.hero.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
    "void AMoonfallHeroPawn::ResidentContextMouseClick()",1)[0]
  self.assertNotIn("StopJumping();",source)
  self.assertNotIn("StopMovementImmediately();",source)
  self.assertNotIn("EndSprint();",source)
  self.assertNotIn("EndGuard();",source)
 def test_hover_and_quest_status_ui(self):
  hud=(ROOT/"MoonfallCombatHUD.cpp").read_text()
  self.assertIn("GetMousePosition(CursorX, CursorY)",hud)
  self.assertIn("const bool bHover",hud)
  self.assertIn("(bHover || Row == Hero->ResidentContextSelection)",hud)
  self.assertIn('TEXT("Available")',hud)
  self.assertIn('TEXT("Active")',hud)
  self.assertIn('TEXT("Completed")',hud)
  self.assertIn('TEXT("Contract: %s  |  Faction %s")',hud)
  self.assertIn("PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen || bAtlasOpen;",self.hero)
  self.assertNotIn("bResidentCursorWasVisible",self.hero)
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
  self.assertIn("Version = 12",self.save)
  self.assertIn("Save->FactionReputation = FactionReputation;",self.hero)
  self.assertIn("if (Save->Version >= 4)",self.hero)
  self.assertIn("FMath::Clamp(*Value, -100, 100)",self.hero)
  self.assertIn("if (!SaveWorldProgress())",self.hero)
  self.assertIn("FactionReputation.Remove(Faction)",self.hero)
if __name__=="__main__": unittest.main()
