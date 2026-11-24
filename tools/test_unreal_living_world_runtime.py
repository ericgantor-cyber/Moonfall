"""Engine-free region weather, encounter and citizen schedule policy tests."""
from pathlib import Path
import shutil, subprocess, tempfile, unittest
SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
class LivingWorldPolicyTests(unittest.TestCase):
    def test_compiled_six_region_rules(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        if not compiler:self.skipTest("C++17 compiler unavailable")
        code=r'''
#include "MoonfallLivingSimulation.h"
#include "MoonfallRoadLandmarks.h"
#include <cassert>
#include <limits>
using namespace MoonfallLivingSimulation;
int main(){
 assert(MoonfallRoadLandmarks::AllLandmarksSafe());
 assert(MoonfallRoadLandmarks::ClearDungeonFootprint(-675.f));
 assert(!MoonfallRoadLandmarks::ClearDungeonFootprint(-1350.f));
 // Test direction in the middle of a region, not on the X=0 border.
 assert(RoadEncounterOffset(2500.f,-150.f)==-600.f);
 assert(RoadEncounterOffset(2500.f,150.f)==600.f);
 assert(RoadEncounterOffset(2500.f,0.f)==600.f);
 assert(RoadEncounterOffset(0.f,-150.f)==600.f);
 // Direction reverses at a border instead of discarding the encounter.
 assert(RoadEncounterOffset(-13520.f,150.f)==-600.f);
 assert(RoadEncounterOffset(-20240.f,-150.f)==600.f);
 assert(RoadEncounterOffset(20240.f,150.f)==-600.f);
 assert(RoadEncounterOffset(20250.f,0.f)==0.f);
 assert(RoadEncounterOffset(std::numeric_limits<float>::quiet_NaN(),0.f)==0.f);
 assert(RoadEncounterOffset(0.f,std::numeric_limits<float>::infinity())==0.f);
 assert(PauseResidentMotion(true,false,false,false));
 assert(PauseResidentMotion(true,true,true,false));
 assert(PauseResidentMotion(true,true,false,true));
 assert(PauseResidentMotion(false,true,false,false));
 assert(!PauseResidentMotion(false,true,true,false));
 assert(!PauseResidentMotion(false,true,false,true));
 assert(!PauseResidentMotion(false,false,false,false));
 assert(ResidentWindowCount(0)==0);
 assert(ResidentWindowCount(1)==1);
 assert(ResidentWindowCount(19)==19);
 assert(ResidentWindowCount(20)==20);
 assert(ResidentWindowCount(21)==20);
 assert(ResidentWindowCount(80)==20);
 assert(ResidentWindowIndex(0,0,0)==-1);
 assert(ResidentWindowIndex(60,19,80)==79);
 assert(ResidentWindowIndex(60,20,80)==0);
 // Only 20 of 80 residents are nearby: each must respond every pulse.
 int nearbyVisits[80]={};
 int nextCursor=0;
 for (int pulse=0;pulse<4;++pulse) {
  int updates=0;
  nextCursor=ProcessNearbyResidents(nextCursor,80,
    [](int i){return i%4==0;},
    [&](int i){++nearbyVisits[i];++updates;});
  assert(updates==20);
 }
 for(int i=0;i<80;++i)assert(nearbyVisits[i]==(i%4==0?4:0));
 // Crowded settlement: 40 nearby NPCs share the 20-slot budget fairly.
 int crowdedVisits[80]={};
 nextCursor=0;
 for (int pulse=0;pulse<4;++pulse) {
  int updates=0;
  nextCursor=ProcessNearbyResidents(nextCursor,80,
    [](int i){return i<40;},
    [&](int i){++crowdedVisits[i];++updates;});
  assert(updates==20);
 }
 for(int i=0;i<80;++i)assert(crowdedVisits[i]==(i<40?2:0));
 // Sparse/empty/invalid registries must not run any routines or divide by 0.
 int calls=0;
 assert(ProcessNearbyResidents(17,0,
    [](int){return true;},[&](int){++calls;})==0);
 assert(ProcessNearbyResidents(0,80,
    [](int){return false;},[&](int){++calls;})==0);
 assert(calls==0);
 assert(RegionFromWorldX(-16875.f)==0);
 assert(RegionFromWorldX(0.f)==3);
 assert(RegionFromWorldX(16875.f)==5);
 assert(RegionFromWorldX(20250.f)==-1);
 assert(RegionFromWorldX(std::numeric_limits<float>::quiet_NaN())==-1);
 assert(ReactionFor(ERole::Guard,10.f)==EAlertReaction::HoldPost);
 assert(ReactionFor(ERole::Merchant,10.f)==EAlertReaction::SeekShelter);
 assert(ReactionFor(ERole::Citizen,84.f)==EAlertReaction::SeekShelter);
 assert(ReactionFor(ERole::Guard,85.f)==EAlertReaction::Normal);
 assert(ReactionFor(ERole::Citizen,-1.f)==EAlertReaction::Normal);
 assert(ReactionFor(ERole::Merchant,
     std::numeric_limits<float>::quiet_NaN())==EAlertReaction::Normal);
 assert(!ShouldRetireAmbient(99.f,100.f,false,false));
 assert(ShouldRetireAmbient(100.f,100.f,false,false));
 assert(!ShouldRetireAmbient(108.f,100.f,true,false));
 assert(!ShouldRetireAmbient(108.f,100.f,false,true));
 assert(ShouldRetireAmbient(108.f,100.f,false,false));
 assert(!ShouldRetireAmbient(
     std::numeric_limits<float>::quiet_NaN(),100.f,false,false));
 assert(!ShouldRetireAmbient(105.f,
     std::numeric_limits<float>::infinity(),false,false));
 assert(Phase(-1.f,360.f)==0);
 assert(Phase(std::numeric_limits<float>::infinity(),360.f)==0);
 assert(WeatherFor(4,0)==EWeather::Snow||WeatherFor(4,0)==EWeather::Fog);
 assert(ActivityFor(ERole::Merchant,0,EWeather::Storm)==EActivity::Shelter);
 assert(ActivityFor(ERole::Guard,3,EWeather::Rain)==EActivity::Patrol);
 assert(ActivityFor(ERole::Scout,0,EWeather::Clear)==EActivity::Travel);
 assert(!MayEncounter(2,true,true,false,false));
 assert(!MayEncounter(2,false,true,true,false));
 assert(!MayEncounter(2,false,true,false,true));
 assert(MayEncounter(2,false,true,false,false));
 for(int i=0;i<6;++i)for(int j=0;j<10;++j){
  const auto event=EncounterFor(i,j);
  assert(static_cast<int>(event)>=0&&static_cast<int>(event)<=4);
  const auto severe=EncounterForWeather(i,j,EWeather::Storm);
  assert(severe==EEvent::Patrol||severe==EEvent::Rescue);
  assert(EncounterForWeather(i,j,EWeather::Clear)==event);
 }
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"sim.cpp";exe=Path(d)/"sim"
            source.write_text(code)
            subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror","-I",str(SRC),str(source),"-o",str(exe)],capture_output=True,text=True,check=True)
            subprocess.run([str(exe)],check=True)
    def test_danger_changes_npc_activity_and_local_routines(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        activity=npc.split("FString AMoonfallWorldNPC::GetCurrentActivity() const {",1)[1].split(
            "bool AMoonfallWorldNPC::SharesRaceWith",1)[0]
        movement=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertIn("ReactionFor(Role,Now-LastDangerAt)",activity)
        self.assertIn('return TEXT("On Alert")',activity)
        self.assertIn('return TEXT("Seeking Shelter")',activity)
        self.assertIn("ReactionFor(",movement)
        self.assertIn("EAlertReaction::SeekShelter",movement)
        self.assertIn("EAlertReaction::HoldPost",movement)
        self.assertIn("FVector Step=Target-GetActorLocation();",movement)
        self.assertIn("Step.GetClampedToMaxSize2D(32.f)",movement)
        self.assertIn("EncounterForWeather(",mode)
        self.assertIn("WeatherFor(",mode)
        self.assertIn("secondsSinceDanger>=85.f",policy)
        self.assertNotIn("TActorIterator",movement)
        self.assertNotIn("SetTimer",movement)

    def test_alerted_residents_can_move_near_hero_without_interrupting_talk(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        npc_h=(SRC/"MoonfallWorldNPC.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        movement=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertIn("bool bInConversation=false",npc_h)
        self.assertIn("bool bInConversation, int32 HeroBond, bool bAllowLocalGuide, bool bHoldCompanion)",movement)
        self.assertIn("PauseResidentMotion(",movement)
        self.assertIn("bInConversation,bNearHero,bSheltering,bHoldPost",movement)
        self.assertIn("if (MoonfallLivingSimulation::PauseResidentMotion(",movement)
        self.assertIn("inConversation || (nearHero && !sheltering && !holdingPost)",policy)
        self.assertIn("NPC->AdvanceLivingRoutine(Now,HeroLocation,",mode)
        self.assertIn("Hero->bResidentContextOpen && Hero->FocusedResident.Get()==NPC",mode)
        self.assertIn("Step.GetClampedToMaxSize2D(32.f)",movement)
        self.assertNotIn("TActorIterator",movement)
        self.assertNotIn("SetTimer",movement)

    def test_v27_intent_uses_existing_pulse_and_saved_bond(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        header=(SRC/"MoonfallWorldNPC.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        movement=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertIn("const Action Intent=Decide(Context);",movement)
        self.assertIn("NPC->GetBondWithHero(Hero)",mode)
        self.assertIn("Context.trust=FMath::Clamp(HeroBond/100.f",movement)
        self.assertIn("bUsePersonalityRoutines=true",header)
        self.assertIn("if (bMerchant || bScout || bGuard || bInConversation) bRoutineHome=false;",movement)
        self.assertIn("bSheltering || bHoldPost || bRoutineHome",movement)
        self.assertLess(movement.index("PauseResidentMotion("),movement.index("AddActorWorldOffset("))
        self.assertNotIn("TActorIterator",movement)
        self.assertNotIn("SetTimer",movement)

    def test_v27_bounded_social_rumors(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        subsystem=(SRC/"MoonfallLivingWorldSubsystem.cpp").read_text()
        helper=mode.split("void AMoonfallUnrealGameMode::ShareNearbyResidentRumor(",1)[1].split(
            "void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[0]
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("SocialCandidates.Reserve(MoonfallLivingSimulation::ResidentBudget)",pulse)
        self.assertIn("!bCombatNearby && !Hero->bResidentContextOpen",pulse)
        self.assertIn("HeroLocation,false,180.f",pulse)
        self.assertIn("CombatPhase%1024",pulse)
        self.assertIn("Other->SettlementId!=Speaker->SettlementId",helper)
        self.assertIn("Society->GetNpcRelationship(Speaker->ResidentId,Other->ResidentId,Attitude)",helper)
        self.assertLess(helper.index("if (Attitude<=-20) continue;"),helper.index("BestDistance=Distance;"))
        self.assertLess(helper.index("if (Attitude<=-20) continue;"),helper.index("Listener=Other;"))
        self.assertEqual(helper.count("LineTraceSingleByChannel("),1)
        self.assertNotIn("TActorIterator",helper)
        self.assertNotIn("SetTimer",helper)
        self.assertIn("Society->ShareUsefulRumor",helper)
        self.assertLess(helper.index("FindUsefulRumor("),helper.index("BestDistance=Distance;"))
        self.assertLess(helper.index("LineTraceSingleByChannel("),helper.index("Society->ShareUsefulRumor"))
        self.assertIn("if (!Society->ShareUsefulRumor",helper)
        self.assertLess(helper.index("Society->ShareUsefulRumor"),helper.index("OnAmbientNewsShared("))
        self.assertNotIn("OnDialogueStarted(",helper)
        self.assertNotIn("LastSpokenLine",helper)
        selector=subsystem.split("FName UMoonfallLivingWorldSubsystem::FindUsefulRumor(",1)[1].split(
            "bool UMoonfallLivingWorldSubsystem::GetNearbyCombatReport(",1)[0]
        self.assertNotIn("StoreRumor(",selector)
        self.assertNotIn("RelayRumor(",selector)
        self.assertIn("Known.Confidence>=Confidence",selector)
        self.assertIn("return BestId;",selector)
        share=subsystem.split("bool UMoonfallLivingWorldSubsystem::ShareUsefulRumor(",1)[1].split(
            "bool UMoonfallLivingWorldSubsystem::ReportWitnessToGuard(",1)[0]
        self.assertIn("Attitude<=-20",share)
        self.assertIn("R.bCrime||R.ExpiresAt<=Time",share)
        self.assertIn("Known.bDirectWitness||Known.bWitnessReport||Known.Confidence>=Confidence",share)
        self.assertIn("RelayRumor(Speaker,Listener,BestId)",share)

    def test_v27_news_trust_requires_new_successful_transfer(self):
        subsystem=(SRC/"MoonfallLivingWorldSubsystem.cpp").read_text()
        header=(SRC/"MoonfallLivingWorldSubsystem.h").read_text()
        share=subsystem.split("bool UMoonfallLivingWorldSubsystem::ShareUsefulRumor(",1)[1].split(
            "void UMoonfallLivingWorldSubsystem::CreditUsefulNews(",1)[0]
        credit=subsystem.split("void UMoonfallLivingWorldSubsystem::CreditUsefulNews(",1)[1].split(
            "FName UMoonfallLivingWorldSubsystem::FindUsefulRumor(",1)[0]
        self.assertIn("const bool bAlreadyKnown=GetRumor(Listener,BestId,Previous)",share)
        self.assertIn("if(!RelayRumor(Speaker,Listener,BestId))return false;",share)
        self.assertIn("if(!bAlreadyKnown&&GetRumor(Listener,BestId,Received))",share)
        self.assertLess(share.index("if(!RelayRumor"),share.index("CreditUsefulNews("))
        session=(SRC/"MoonfallNpcSocialSessionSubsystem.h").read_text()
        self.assertIn("LastAmbientTrustAt=-1.0",session)
        self.assertIn("FPlatformTime::Seconds()",credit)
        self.assertIn("Session->Relationships",credit)
        self.assertIn("Time-R.LastAmbientTrustAt",credit)
        self.assertIn("AmbientTrustDelta(R.Score,Confidence,Elapsed)",credit)
        self.assertIn("AdjustNpcRelationship(Listener,Speaker,Delta)",credit)
        self.assertNotIn("ChangeResidentBond",credit)

    def test_v27_relationship_session_survives_world_teardown(self):
        world=(SRC/"MoonfallLivingWorldSubsystem.cpp").read_text()
        world_h=(SRC/"MoonfallLivingWorldSubsystem.h").read_text()
        session=(SRC/"MoonfallNpcSocialSessionSubsystem.h").read_text()
        teardown=(SRC/"MoonfallNpcSocialSessionSubsystem.cpp").read_text()
        self.assertIn("public UGameInstanceSubsystem",session)
        self.assertIn("TMap<FName,TArray<FRelationship>> Relationships",session)
        self.assertNotIn("TMap<FName,TArray<FRelationship>> Relationships",world_h)
        self.assertIn("GetWorld()->GetGameInstance()",world)
        self.assertIn("GetSubsystem<UMoonfallNpcSocialSessionSubsystem>()",world)
        self.assertIn("if(!CanWrite())return nullptr;",world)
        self.assertIn("GetNetMode()!=NM_Client",world)
        world_end=world.split("void UMoonfallLivingWorldSubsystem::Deinitialize()",1)[1]
        self.assertNotIn("Relationships.Reset()",world_end)
        for local in ("Knowledge.Reset()","Observations.Reset()","Encounters.Reset()"):
            self.assertIn(local,world_end)
        self.assertIn("Relationships.Reset()",teardown)
        reset=world.split("bool UMoonfallLivingWorldSubsystem::ResetNpcSocialSession()",1)[1].split(
            "bool UMoonfallLivingWorldSubsystem::GetNpcRelationship",1)[0]
        self.assertIn("if(!Session->ResetLedger())return false;",reset)
        self.assertNotIn("SaveGameToSlot",world)

    def test_v27_durable_social_save_isolated_validated_and_rotated(self):
        world=(SRC/"MoonfallLivingWorldSubsystem.cpp").read_text()
        session=(SRC/"MoonfallNpcSocialSessionSubsystem.cpp").read_text()
        schema=(SRC/"MoonfallNpcSocialSave.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        self.assertIn("WorldType==EWorldType::Game&&GetWorld()->GetNetMode()==NM_Standalone",world)
        self.assertIn("SocialMemory->FlushSocialMemory()",mode)
        self.assertIn("Session->bDirty=true",world)
        self.assertIn('TEXT("MoonfallNpcSocial_A")',session)
        self.assertIn('TEXT("MoonfallNpcSocial_B")',session)
        self.assertIn("UPROPERTY(SaveGame) int32 Version=1",schema)
        load=session.split("void UMoonfallNpcSocialSessionSubsystem::InitializePersistence",1)[1].split(
            "bool UMoonfallNpcSocialSessionSubsystem::Flush",1)[0]
        for check in ("Save->Version!=1","Save->Generation<=0","MaxObservers*MaxPerObserver",
                      "Entry.Observer==Entry.Other","Entry.Score<-100","Entry.Score>100",
                      "List->ContainsByPredicate","Save->Generation>Generation",
                      "bAnyExisting&&!bFoundValid","FPlatformTime::Seconds()"):
            self.assertIn(check,load)
        flush=session.split("bool UMoonfallNpcSocialSessionSubsystem::Flush",1)[1].split(
            "bool UMoonfallNpcSocialSessionSubsystem::ResetLedger",1)[0]
        self.assertIn("if(bWriteBlocked)return false;",flush)
        self.assertIn("if(!bDirty)return true;",flush)
        self.assertIn("Time-LastSaveAttempt<60.0",flush)
        self.assertIn("ActiveSlot==0?1:0",flush)
        self.assertLess(flush.index("SaveGameToSlot"),flush.index("ActiveSlot=NextSlot"))
        reset=session.split("bool UMoonfallNpcSocialSessionSubsystem::ResetLedger",1)[1]
        self.assertIn("if(Flush(true))return true;",reset)
        self.assertIn("Relationships=MoveTemp(Previous)",reset)
        self.assertIn("bDirty=bWasDirty",reset)
        self.assertNotIn("MoonfallWorldSlot",session)
        self.assertNotIn("SetTimer",session)

    def test_v27_emotional_state_preserves_quest_and_dialogue_authority(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        header=(SRC/"MoonfallWorldNPC.h").read_text()
        mood=npc.split("EMoonfallNpcMood AMoonfallWorldNPC::GetEmotionalState(",1)[1].split(
            "bool AMoonfallWorldNPC::TryTalk(",1)[0]
        self.assertIn("MoonfallWorldPolicy::ResolveMood",mood)
        self.assertIn("GetNearbyCombatReport",mood)
        self.assertIn("Now-LastDangerAt<85.f",mood)
        self.assertIn("GetBondWithHero(Hero)",mood)
        self.assertIn("GetFactionReputation(FactionId)",mood)
        self.assertIn("EMoonfallNpcMood LastConversationMood",header)
        talk=npc.split("bool AMoonfallWorldNPC::TryTalk(",1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(",1)[0]
        self.assertLess(talk.index("CanHeroInteract(Hero)"),talk.index("LastConversationMood=GetEmotionalState"))
        self.assertLess(talk.index("LastConversationMood==EMoonfallNpcMood::Warm"),talk.index("SupportsResidentContract"))
        self.assertLess(talk.index("LastConversationMood=GetEmotionalState"),talk.index("OnDialogueStarted"))
        self.assertNotIn("ChangeResidentBond(",mood)
        self.assertNotIn("WitnessNearbyDanger(",mood)

    def test_v27_local_scout_guidance_is_safe_and_bounded(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("bool bGuideBudgetUsed=false",pulse)
        self.assertIn("!bGuideBudgetUsed",pulse)
        self.assertIn("if(bGuide)bGuideBudgetUsed=true",pulse)
        self.assertIn("Hero->GetFactionReputation(NPC->FactionId)",pulse)
        self.assertIn("const bool bGuideSafe=!bCombatNearby",pulse)
        for menu in ("bInventoryOpen","bOptionsOpen","bAtlasOpen","bCharacterStatsOpen",
                     "bSpellBookOpen","bResidentContextOpen"):
            self.assertIn("!Hero->"+menu,pulse)
        movement=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertIn("bAllowLocalScoutGuidance",movement)
        self.assertIn("Alert==MoonfallLivingSimulation::EAlertReaction::Normal",movement)
        self.assertIn("FMath::Clamp(Distance-240.0,0.0,180.0)",movement)
        self.assertIn("Hit.ImpactNormal.Z<0.7",movement)
        self.assertIn("FMath::Abs(Hit.ImpactPoint.Z-ExistingGround.ImpactPoint.Z)>10.0",movement)
        self.assertIn("Step.GetClampedToMaxSize2D(32.f)",movement)
        self.assertNotIn("SetActorLocation(",movement)
        self.assertNotIn("MoveToLocation(",movement)

    def test_v27_local_companion_recruitment_is_explicit_and_owner_checked(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        recruit=mode.split("bool AMoonfallUnrealGameMode::RecruitLocalCompanion(",1)[1].split(
            "bool AMoonfallUnrealGameMode::SetLocalCompanionStance(",1)[0]
        for condition in ("HasAuthority()","Hero->IsPlayerControlled()","Resident->CanHeroInteract(Hero)",
                          "Resident->GetBondWithHero(Hero)<60","GetFactionReputation(Resident->FactionId)<0",
                          "LivingResidents.ContainsByPredicate","LocalCompanion.IsValid()"):
            self.assertIn(condition,recruit)
        self.assertIn("CompanionOwner=Hero",recruit)
        self.assertIn("TWeakObjectPtr<AMoonfallWorldNPC> LocalCompanion",header)
        self.assertIn("Follow, Hold, Passive",header)
        self.assertIn("!IsLocalCompanionOwner(Hero)",mode)
        self.assertIn("if(LocalCompanion.Get()==Resident)",mode)
        self.assertIn("bRecruited&&LocalCompanionStance==EMoonfallCompanionStance::Follow",mode)
        self.assertIn("bRecruited&&LocalCompanionStance==EMoonfallCompanionStance::Hold",mode)
        self.assertIn("Bond,bGuide,bHold",mode)
        self.assertIn("bHoldCompanion&&!bInConversation&&!bSheltering&&!bHoldPost",npc)
        self.assertIn('LivingIntent=TEXT("Holding Position");return;',npc)

    def test_v27_companion_menu_uses_shared_geometry_and_dispatch(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        mouse=hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()",1)[1].split(
            "void AMoonfallHeroPawn::TalkToNearestNPC()",1)[0]
        panel=hud.split("// Native Canvas resident menu.",1)[1].split("if (Hero->bAtlasOpen)",1)[0]
        for shared in ("GetResidentContextRowCount()","GetResidentContextPanelHeight(",
                       "GetResidentContextRowPitch(","122.f + Row * Pitch","Pitch-10.f"):
            self.assertIn(shared,mouse)
            self.assertIn(shared,panel)
        self.assertIn("ChooseResidentContextAction(Row)",mouse)
        self.assertIn("(ResidentContextSelection + 1) % GetResidentContextRowCount()",hero)
        self.assertIn("(ResidentContextSelection + GetResidentContextRowCount() - 1) % GetResidentContextRowCount()",hero)
        choose=hero.split("bool AMoonfallHeroPawn::ChooseResidentContextAction(",1)[1].split(
            "// Bounded, persistent faction relationship API.",1)[0]
        self.assertLess(choose.index("CanHeroInteract(this)"),choose.index("if(bResidentCompanionMenu)"))
        self.assertIn("if(Choice==2){bResidentCompanionMenu=false",choose)
        self.assertIn("if(Choice==3&&GetResidentContextRowCount()==4)",choose)
        self.assertIn("bResidentTurnInPending=false;bResidentCompanionMenu=true",choose)
        for command in ("RecruitLocalCompanion","SetLocalCompanionStance","DismissLocalCompanion"):
            self.assertIn(command,choose)
        self.assertLess(choose.index("if(bResidentCompanionMenu)"),choose.index("OfferResidentQuest(this)"))
        self.assertIn('TEXT("Companion")',panel)
        self.assertIn('TEXT("Back")',panel)
        self.assertIn('TEXT("Close")',panel)
        self.assertIn('TEXT("Recruit (60 trust)")',panel)

    def test_v27_guard_reports_remain_indirect_and_contextual(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        subsystem=(SRC/"MoonfallLivingWorldSubsystem.cpp").read_text()
        report=subsystem.split("bool UMoonfallLivingWorldSubsystem::GetNearbyCombatReport(",1)[1].split(
            "bool UMoonfallLivingWorldSubsystem::ReportWitnessToGuard(",1)[0]
        for condition in ("R.bCrime","R.bDirectWitness","R.bWitnessReport","R.Hops<1",
                          "R.Confidence<40","R.ExpiresAt<=Time","NearbyCombat_",
                          "FMath::Square(1800.0)"):
            self.assertIn(condition,report)
        self.assertIn("R.Confidence>Report.Confidence",report)
        movement=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertIn("bReviewingCombatReport=false;",movement)
        self.assertIn("bGuard && !bInConversation",movement)
        self.assertIn("bReviewingCombatReport=Society->GetNearbyCombatReport",movement)
        self.assertIn('TEXT("Reviewing Reports")',movement)
        self.assertNotIn("WitnessNearbyDanger(",movement)
        talk=npc.split("bool AMoonfallWorldNPC::TryTalk(",1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(",1)[0]
        self.assertIn("Nothing is confirmed yet.",talk)
        self.assertIn("I cannot say for certain.",talk)
        self.assertLess(talk.index("Now-LastDangerAt<85.f"),talk.index("GetNearbyCombatReport"))
        self.assertNotIn("ChangeResidentBond(",talk)
        self.assertNotIn("ReportWitnessToGuard(",talk)

    def test_v27_guard_watch_points_stay_inside_authored_patrol(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        move=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero",1)[0]
        self.assertLess(move.index("PauseResidentMotion("),move.index("GuardWatchPoint("))
        self.assertIn("!bSheltering && !bInConversation && !bNearHero",move)
        self.assertIn("!MoonfallLivingSimulation::Severe(Weather)",move)
        self.assertIn("GuardReport.Location-AuthoredHomeLocation",move)
        self.assertIn("GuardReport.Confidence,true",move)
        for point in ("Side=-65.f; Front=0.f","Side=65.f; Front=0.f",
                      "Side=0.f; Front=45.f","Side=0.f; Front=-45.f"):
            self.assertIn(point,move)
        self.assertIn("AuthoredHomeLocation+FVector(Side,Front,0.f)",move)
        self.assertIn("Step.GetClampedToMaxSize2D(32.f)",move)
        self.assertNotIn("MoveToLocation(",move)
        self.assertNotIn("SetActorLocation(",move)

    def test_resident_behavior_uses_weather_with_no_personal_tick(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        self.assertIn("PrimaryActorTick.bCanEverTick = false;",npc)
        self.assertIn("AdvanceLivingRoutine(float Now,",npc)
        self.assertIn("Step.GetClampedToMaxSize2D(32.f)",npc)
        self.assertIn("MoonfallLivingSimulation::ActivityFor(Role,Shift,Weather)",npc)
        self.assertIn("Now-LastDangerAt<85.f",npc)
        self.assertIn("if (bAmbientVisitor) return 0;",npc)
        self.assertIn("if (bAmbientVisitor || ResidentId.IsNone()",npc)
        self.assertIn("IsRecentlyInCombatForAmbientAI() const",hero)
        self.assertNotIn("GetAllActorsOfClass(",npc)

    def test_regional_landmarks_and_weather_hud_are_budgeted(self):
        arena=(SRC/"MoonfallArena.cpp").read_text()
        header=(SRC/"MoonfallArena.h").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        landscape=arena.split("void AMoonfallArena::CreateRegionalLandmarks()",1)[1].split(
            "void AMoonfallArena::BeginPlay()",1)[0]
        self.assertIn("Zone<6",landscape)
        self.assertIn("RegionLandmarkDetails=Details;",landscape)
        self.assertIn("Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);",landscape)
        self.assertIn("Details->SetCullDistances(18000,28000);",landscape)
        self.assertIn("Light->SetCastShadows(false);",landscape)
        self.assertIn("Light->SetAttenuationRadius(3000.f);",landscape)
        self.assertIn("LastRegionWeatherPhase==Phase",landscape)
        self.assertIn("MoonfallLivingSimulation::WeatherFor(Zone,Phase)",landscape)
        self.assertIn("bEnableRegionalLandmarks = true;",header)
        self.assertIn("CreateRegionalLandmarks();",arena)
        quest=hud.split("// World-wide quest tracker,",1)[1].split(
            "// Compact circular resource HUD",1)[0]
        self.assertIn("MoonfallLivingSimulation::WeatherName(Climate)",quest)
        self.assertNotIn("TActorIterator",quest)
        self.assertNotIn("GetAllActorsOfClass",quest)

    def test_bounded_round_robin_residents_do_not_starve(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("constexpr int ResidentBudget=20;",policy)
        self.assertIn("constexpr int ResidentRegistryBudget=80;",policy)
        self.assertIn("ResidentWindowIndex(int cursor,int step,int registered)",policy)
        self.assertIn("int32 NextResidentPulseIndex = 0;",header)
        self.assertIn("ProcessNearbyResidents(",pulse)
        self.assertIn("NextResidentPulseIndex,LivingResidents.Num()",pulse)
        self.assertIn("DistSq<=FMath::Square(1600.f)",pulse)
        self.assertIn("NPC->WitnessNearbyDanger(Now);",pulse)
        self.assertIn("NPC->AdvanceLivingRoutine(Now,HeroLocation,",pulse)
        self.assertIn("Hero->bResidentContextOpen && Hero->FocusedResident.Get()==NPC",pulse)
        self.assertIn("while(inspected<registered && processed<ResidentBudget)",policy)
        self.assertIn("if(!isNearby(index))continue;",policy)
        self.assertIn("return (start+inspected)%registered;",policy)
        self.assertNotIn("for (int32 Step=0;Step<ResidentSlice;++Step)",pulse)
        self.assertIn("LivingResidents.Num() >= MoonfallLivingSimulation::ResidentRegistryBudget",mode)
        self.assertIn("Scanned<MoonfallLivingSimulation::ResidentRegistryBudget",pulse)
        self.assertNotIn("GetAllActorsOfClass(",pulse)

    def test_controller_npc_interaction_search_reaches_late_spawned_residents(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        search=hero.split("AMoonfallWorldNPC* AMoonfallHeroPawn::GetNearestResidentNPC() const {",1)[1].split(
            "void AMoonfallHeroPawn::CloseResidentContext()",1)[0]
        # The old first-64 cap made some of the 80 registered residents,
        # plus transient roadside visitors, impossible to select by controller.
        self.assertIn("for (TActorIterator<AMoonfallWorldNPC> It(GetWorld()); It; ++It)",search)
        self.assertNotIn("Checked < 64",search)
        self.assertNotIn("Checked <",search)
        self.assertIn("Candidate->CanHeroInteract(this)",search)
        self.assertIn("DistSq < BestDistSq",search)
        self.assertIn("constexpr int ResidentRegistryBudget=80;",policy)
        self.assertIn("Visitor->bAmbientVisitor=true;",mode)
        # Nearest-NPC lookups are input-triggered, never performed per frame.
        self.assertIn("AMoonfallWorldNPC* Resident = GetNearestResidentNPC();",
                      hero.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1])
        self.assertNotIn("GetNearestResidentNPC()",mode)

    def test_runtime_residents_register_without_permanent_actor_rescans(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        npc_h=(SRC/"MoonfallWorldNPC.h").read_text()
        self.assertIn("void RegisterLivingResident(AMoonfallWorldNPC* Resident);",header)
        self.assertIn("void UnregisterLivingResident(AMoonfallWorldNPC* Resident);",header)
        self.assertIn("void AMoonfallUnrealGameMode::RegisterLivingResident(",mode)
        self.assertIn("LivingResidents.ContainsByPredicate(",mode)
        self.assertIn("LivingResidents.Num() >= MoonfallLivingSimulation::ResidentRegistryBudget",mode)
        self.assertIn("LivingResidents.RemoveAll(",mode)
        self.assertIn("RegisterLivingResident(*It);",mode)
        self.assertIn("Mode->RegisterLivingResident(this);",npc)
        self.assertIn("Mode->UnregisterLivingResident(this);",npc)
        self.assertIn("virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;",npc_h)
        # The event visitor must be flagged before BeginPlay registration
        # can run, and must never be retained as a permanent resident.
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("SpawnActorDeferred<AMoonfallWorldNPC>",pulse)
        self.assertIn("Visitor->bAmbientVisitor=true;",pulse)
        self.assertIn("Visitor->FinishSpawning(SpawnTransform);",pulse)
        self.assertLess(pulse.index("Visitor->bAmbientVisitor=true;"),
                        pulse.index("Visitor->FinishSpawning(SpawnTransform);"))
        self.assertIn("if (!IsValid(Spawned)) return;",pulse)
        self.assertIn("if (!bAmbientVisitor)",npc)
        self.assertIn("if (!IsValid(Resident) || Resident->IsActorBeingDestroyed()",mode)
        self.assertNotIn("GetAllActorsOfClass(",mode)

    def test_six_regions_have_distinct_road_encounter_dialogue(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        rumor=mode.split("static const TCHAR* const RegionRumors[6] = {",1)[1].split("};",1)[0]
        for region in ("Ashen Wilds","Moonveil Forest","Hollow Depths",
                       "Iron Foundry","Frostbound Reach","Eclipse Wastes"):
            self.assertIn(region,rumor)
        self.assertEqual(rumor.count("TEXT("),6)
        self.assertIn("Visitor->Greeting=FText::FromString(Story);",mode)
        self.assertIn("MoonfallLivingSimulation::Severe(CurrentWeather)",mode)
        self.assertIn("if (Region>=0 && Region<6)",mode)
        self.assertNotIn("SaveWorldProgress",rumor)

    def test_cached_road_event_is_visible_without_scanning_or_blocking_quests(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        hint=mode.split("FString AMoonfallUnrealGameMode::GetNearbyAmbientEventHint(",1)[1].split(
            "void AMoonfallUnrealGameMode::BeginPlay()",1)[0]
        self.assertIn("ActiveAmbientEncounter.Get()",hint)
        self.assertIn("FMath::Square(1250.f)",hint)
        self.assertIn("Visitor->bAmbientVisitor",hint)
        self.assertIn("ROAD EVENT: %s | APPROACH TO TALK",hint)
        self.assertIn("ROAD EVENT: HOSTILE CREATURE | DODGE OR LEAVE",hint)
        self.assertNotIn("TActorIterator",hint)
        self.assertNotIn("GetAllActorsOfClass",hint)
        self.assertIn("FString GetNearbyAmbientEventHint(",header)
        tracker=hud.split("// World-wide quest tracker,",1)[1].split(
            "// Compact circular resource HUD",1)[0]
        self.assertIn("GetNearbyAmbientEventHint(Hero->GetActorLocation())",tracker)
        self.assertIn("if (!EventHint.IsEmpty()) Context=EventHint;",tracker)
        self.assertIn("if (!bSanctuaryObjective && GetWorld())",tracker)
        self.assertIn('Context = TEXT("OPTIONAL: Exit via the south gate whenever ready");',tracker)
        self.assertIn("DrawText(Objective,",tracker)
        self.assertNotIn("TActorIterator",tracker)

    def test_one_global_pulse_and_bounded_roadside_encounters(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        self.assertIn("PulseLivingWorld()",mode)
        self.assertIn("&AMoonfallUnrealGameMode::PulseLivingWorld, 4.f, true, 4.f",mode)
        self.assertIn("if (!bLivingResidentCacheInitialized)",mode)
        self.assertIn("Scanned<MoonfallLivingSimulation::ResidentRegistryBudget",mode)
        self.assertIn("RegisterLivingResident(*It);",mode)
        self.assertIn("NPC->AdvanceLivingRoutine(Now,HeroLocation,",mode)
        self.assertIn("NPC->WitnessNearbyDanger(Now);",mode)
        self.assertIn("LivingArena->UpdateRegionWeather(Now);",mode)
        self.assertIn("MoonfallLivingSimulation::MayEncounter(",mode)
        self.assertIn("Hero->IsNearStartingSanctuary()",mode)
        self.assertIn("HeroLocation.Y > -1800.f",mode)
        self.assertIn("World->LineTraceSingleByChannel(",mode)
        self.assertIn("ActiveAmbientEncounter=Spawned;",mode)
        self.assertIn("NextAmbientEncounterAt=Now+150.f;",mode)
        self.assertIn("Spawned->SetLifeSpan(0.f);",mode)
        self.assertIn("ActiveAmbientExpiresAt=Now+100.f;",mode)
        self.assertIn("MoonfallAmbientEncounter",mode)
        self.assertIn("bAmbientVisitor=true;",mode)
        self.assertIn("FTimerHandle LivingWorldPulseTimer",header)
        self.assertIn("TWeakObjectPtr<AMoonfallWorldNPC>",header)
        self.assertIn("if (!bAmbientVisitor && Disposition",npc)
        self.assertNotIn("GetAllActorsOfClass(",mode)

    def test_dead_or_missing_pawn_still_retires_expired_events(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("const bool bHeroActive=IsValid(Hero) && Hero->IsPlayerControlled()",pulse)
        self.assertIn("Hero->Health>0.f;",pulse)
        self.assertIn("const bool bHeroReady=bHeroActive && !HeroLocation.ContainsNaN();",pulse)
        self.assertIn("const bool bTalkingToVisitor=bHeroReady && Hero->bResidentContextOpen",pulse)
        self.assertIn("const bool bNearbyMonsterBattle=bCombatNearby",pulse)
        self.assertIn("if (!IsValid(Active) || Active->IsActorBeingDestroyed())",pulse)
        self.assertIn("ActiveAmbientEncounter.Reset();",pulse)
        self.assertIn("ActiveAmbientExpiresAt=-1.f;",pulse)
        # Expiry happens before the invalid player returns. Previous code
        # returned early and could leak the one active ambient event forever.
        self.assertLess(pulse.index("ShouldRetireAmbient("),
                        pulse.index("if (!bHeroReady) return;"))
        self.assertLess(pulse.index("if (!bHeroReady) return;"),
                        pulse.index("NPC->AdvanceLivingRoutine(Now,HeroLocation,"))
        self.assertNotIn("Spawned->SetLifeSpan(100.f);",pulse)

    def test_road_encounters_follow_travel_without_crossing_region_edges(self):
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("RoadEncounterOffset(float worldX,float travelVelocityX)",policy)
        self.assertIn("RegionFromWorldX(worldX+preferred)==region",policy)
        self.assertIn("RegionFromWorldX(worldX-preferred)==region",policy)
        self.assertIn("if(region<0)return 0.f;",policy)
        self.assertIn("HeroLocation.X,Hero->GetVelocity().X",pulse)
        self.assertIn("if (CandidateOffset==0.f) return;",pulse)
        self.assertIn("HeroLocation.X+CandidateOffset,-2500.f",pulse)
        self.assertIn("RegionFromWorldX(Candidate.X)!=Region",pulse)
        self.assertEqual(pulse.count("World->LineTraceSingleByChannel("),1)
        self.assertNotIn("HeroLocation.X+600.f,-2500.f",pulse)

    def test_encounter_lifetime_preserves_live_conversations_and_battles(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        for name in ("ActiveAmbientEncounter.Get()",
                     "Hero->bResidentContextOpen",
                     "Hero->FocusedResident.Get()==Active",
                     'Active->ActorHasTag(FName(TEXT("MoonfallAmbientEncounter")))',
                     "bCombatNearby","FMath::Square(1200.f)",
                     "ShouldRetireAmbient(",
                     "Active->Destroy();",
                     "ActiveAmbientEncounter.Reset();",
                     "ActiveAmbientExpiresAt=-1.f;"):
            self.assertIn(name,pulse)
        self.assertLess(pulse.index("ShouldRetireAmbient("),pulse.index("if (Now<NextAmbientEncounterAt) return;"))
        retire_branch=pulse.split("if (MoonfallLivingSimulation::ShouldRetireAmbient(",1)[1].split(
            "ActiveAmbientExpiresAt=-1.f;",1)[0]
        self.assertLess(retire_branch.index("Active->Destroy();"),
                        retire_branch.index("ActiveAmbientEncounter.Reset();"))
        self.assertIn("float ActiveAmbientExpiresAt = -1.f;",header)
        self.assertIn("!conversationActive&&!monsterBattleActive",policy)
        self.assertIn("Spawned->SetLifeSpan(0.f);",pulse)
        self.assertNotIn("Spawned->SetLifeSpan(100.f);",pulse)
        self.assertIn("ActiveAmbientExpiresAt=Now+100.f;",pulse)

    def test_story_only_residents_cannot_offer_unsaveable_contracts(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hero_h=(SRC/"MoonfallHeroPawn.h").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        self.assertIn("bool SupportsResidentContract(FName ResidentId) const;",hero_h)
        self.assertIn("return IsNamedMoonfallResident(ResidentId);",hero)
        # The resident allowlist includes both Firstlight sanctuary NPCs.
        validator=hero.split("bool IsNamedMoonfallResident(FName Id) {",1)[1].split(
            "bool AMoonfallHeroPawn::SupportsResidentContract(",1)[0]
        for name in ("CrossroadsGuide","FirstlightArchivist","FirstlightWatchkeeper"):
            self.assertIn('FName(TEXT("'+name+'"))',validator)
        # Do not present quest offers for unknown editor-spawned or dynamic NPCs.
        talk=npc.split("bool AMoonfallWorldNPC::TryTalk(",1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(",1)[0]
        self.assertIn("Hero->SupportsResidentContract(ResidentId)",talk)
        self.assertLess(talk.index("Hero->SupportsResidentContract(ResidentId)"),
                        talk.index("I have a task for you. Select Accept"))
        choice=hero.split("bool AMoonfallHeroPawn::ChooseResidentContextAction(",1)[1].split(
            "// Bounded, persistent faction relationship API.",1)[0]
        self.assertIn("if (Resident->bAmbientVisitor)",choice)
        self.assertIn("if (!SupportsResidentContract(Resident->ResidentId))",choice)
        self.assertIn("This resident has no contract.",choice)
        self.assertLess(choice.index("if (!SupportsResidentContract("),
                        choice.index("Resident->OfferResidentQuest(this)"))
        self.assertIn("const bool bNoContract = Resident->bAmbientVisitor ||",hud)
        self.assertIn("!Hero->SupportsResidentContract(Resident->ResidentId)",hud)
        self.assertIn('const FString QuestAction = bNoContract ? TEXT("No contract")',hud)
        self.assertIn('const FString ContractStatus = bNoContract ? TEXT("None")',hud)
        self.assertIn('bNoContract ? TEXT("Talk to hear this resident',hud)
        # The save format and author-defined world geometry are untouched.
        self.assertIn('bool IsNamedMoonfallResident(FName Id)',hero)

    def test_ambient_visitors_do_not_show_false_quest_offers(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        talk=hero.split("bool AMoonfallHeroPawn::ChooseResidentContextAction(int32 Choice)",1)[1].split(
            "// Bounded, persistent faction relationship API.",1)[0]
        self.assertIn("if (Resident->bAmbientVisitor)",talk)
        self.assertIn("traveler has no contract",talk)
        self.assertLess(talk.index("if (Resident->bAmbientVisitor)"),
                        talk.index("const int32 State = GetResidentQuestState("))
        self.assertIn('TEXT("No contract")',hud)
        self.assertIn('TEXT("Road encounter  |  Talk to hear their story")',hud)
        self.assertIn("Resident->bAmbientVisitor ? 2",hud)
        self.assertIn("MoonfallLivingSimulation::Severe(Weather)",npc)
        self.assertIn("const bool bSheltering=",npc)

    def test_dungeon_gap_landmark_footprints_are_safe(self):
        arena=(SRC/"MoonfallArena.cpp").read_text()
        policy=(SRC/"MoonfallRoadLandmarks.h").read_text()
        self.assertIn('#include "MoonfallRoadLandmarks.h"',arena)
        self.assertIn("MoonfallRoadLandmarks::Placements",arena)
        self.assertIn("static_assert(AllLandmarksSafe()",policy)
        self.assertIn("ClearDungeonFootprint(p.X)",policy)
        self.assertIn("for (int i=0; i<5; ++i)",policy)
        # Every requested region silhouette remains outside 900-unit lanes,
        # accounting for the largest 270-unit-wide landmark primitive.
        import re
        pieces=re.findall(r'\{(-?\d+)\.f,(-?\d+)\.f,([.\d]+)f\}',policy)
        self.assertEqual(len(pieces),8)
        for x,y,scale in pieces:
            x,y=int(x),int(y)
            self.assertGreater(float(scale),0)
            self.assertLess(y,-4000)
            self.assertGreater(y,-7000)
            self.assertTrue(all(abs(x-(i-2)*1350)>=600 for i in range(5)),
                            f"Landmark {x} overlaps a dungeon lane")

    def test_ambient_encounters_require_public_road_and_unlocked_combat(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        for guard in ("Hero->bCharacterStatsOpen", "Hero->bSpellBookOpen",
                      "Hero->FocusedResident.IsValid() || bCombatNearby",
                      "Ground.GetComponent()!=LivingArena->DungeonRoad",
                      "Ground.ImpactNormal.Z<0.7f", "Hero->IsNearStartingSanctuary()"):
            self.assertIn(guard,pulse)
        self.assertLess(pulse.index("Ground.GetComponent()!=LivingArena->DungeonRoad"),
                        pulse.index("World->SpawnActorDeferred<AMoonfallEnemy>("))
        self.assertIn("Spawned->SetLifeSpan(0.f);",pulse)
        self.assertIn("ActiveAmbientExpiresAt=Now+100.f;",pulse)

if __name__=="__main__":unittest.main()
