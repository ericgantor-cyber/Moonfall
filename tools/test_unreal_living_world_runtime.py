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
#include "MoonfallLivingWorldPolicy.h"
#include <cassert>
#include <limits>
#include <initializer_list>
using namespace MoonfallLivingSimulation;
int main(){
 assert(MoonfallRoadLandmarks::AllLandmarksSafe());
 assert(MoonfallRoadLandmarks::ArtProfilesSafe());
 static_assert(sizeof(MoonfallRoadLandmarks::ArtProfiles)/sizeof(MoonfallRoadLandmarks::ArtProfiles[0])==6);
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
 // Test the same health boundary in support and tactical withdrawal.
 for(int health=1;health<=100;++health){
  assert(MoonfallWorldPolicy::CanCompanionSupportOffensively(float(health))==(health>25));
  for(int role=0;role<3;++role){
   auto tactic=MoonfallWorldPolicy::SelectCompanionTactic(float(health),3,role,true);
   assert((tactic==MoonfallWorldPolicy::CompanionTactic::Withdraw)==(health<=25));
  }
 }
 assert(!MoonfallWorldPolicy::CanCompanionSupportOffensively(0));
 assert(!MoonfallWorldPolicy::CanCompanionSupportOffensively(101));
 assert(!MoonfallWorldPolicy::CanCompanionSupportOffensively(std::numeric_limits<float>::quiet_NaN()));
 assert(MoonfallWorldPolicy::FreshCompanionThreat(12,10,1,350.0*350.0));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12.01,10,1,0));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(9,10,1,0));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12,10,0,0));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12,10,1,350.0*350.0+1));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12,10,1,-1));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12,10,std::numeric_limits<float>::quiet_NaN(),0));
 assert(!MoonfallWorldPolicy::FreshCompanionThreat(12,10,1,std::numeric_limits<double>::infinity()));
 using MoonfallWorldPolicy::FollowerStepBudget;
 assert(FollowerStepBudget(200,0,0.2)==24);
 assert(FollowerStepBudget(200,24,0.2)==48);
 assert(FollowerStepBudget(200,48,0.2)==60);
 assert(FollowerStepBudget(200,48,0.2,80)==72);
 assert(FollowerStepBudget(200,72,0.2,80)==80);
 assert(FollowerStepBudget(40,80,0.2,80)==20);
 assert(FollowerStepBudget(200,80,0.5,80)==24);
 assert(FollowerStepBudget(200,0,0.2,101)==0);
 assert(FollowerStepBudget(200,0,0.2,0)==0);
 assert(FollowerStepBudget(200,60,0.5)==24);
 assert(FollowerStepBudget(40,60,0.2)==20);
 assert(FollowerStepBudget(5,60,0.2)==5);
 assert(FollowerStepBudget(0,60,0.2)==0);
 assert(FollowerStepBudget(200,60,-1)==24);
 assert(FollowerStepBudget(std::numeric_limits<double>::quiet_NaN(),60,0.2)==0);
 assert(FollowerStepBudget(200,std::numeric_limits<double>::infinity(),0.2)==24);
 for(int remaining=0;remaining<=200;++remaining){
  const double step=FollowerStepBudget(remaining,48,0.2);
  assert(step>=0&&step<=remaining&&step<=60);
 }
 assert(RegionalAllowsSocial(-1));
 for(int danger=0;danger<=100;++danger)assert(RegionalAllowsSocial(danger)==(danger<65));
 assert(RegionalAllowsSocial(101));
 for(int region=0;region<6;++region){
  for(int phase=0;phase<60;++phase){
   const int variant=EncounterDialogueVariant(region,phase);
   assert(variant>=0&&variant<3);
   assert(EncounterDialogueVariant(region,phase+5)==(variant+1)%3);
   assert(EncounterDialogueVariant(region,phase+15)==variant);
  }
 }
 assert(EncounterDialogueVariant(5,std::numeric_limits<int>::max())>=0);
 // Exhaust every valid danger level and boss count for every encounter kind.
 for(int danger=0;danger<=100;++danger){
  for(int bosses=0;bosses<=5;++bosses){
   for(auto event:{EEvent::Merchant,EEvent::Patrol,EEvent::Rescue,EEvent::Dispute,EEvent::Monster}){
    auto expected=event;
    if(danger>=35&&event==EEvent::Dispute)expected=EEvent::Patrol;
    if(danger>=65&&event==EEvent::Merchant)expected=EEvent::Patrol;
    if(danger<35&&bosses==5&&event==EEvent::Monster)expected=EEvent::Merchant;
    assert(ReactiveEncounter(event,EWeather::Clear,danger,bosses)==expected);
    assert(ReactiveEncounter(event,EWeather::Storm,danger,bosses)==(event==EEvent::Patrol?EEvent::Patrol:EEvent::Rescue));
   }
  }
 }
 assert(ReactiveEncounter(EEvent::Dispute,EWeather::Clear,-1,0)==EEvent::Dispute);
 assert(ReactiveEncounter(EEvent::Dispute,EWeather::Clear,101,0)==EEvent::Dispute);
 assert(ReactiveEncounter(EEvent::Dispute,EWeather::Clear,50,6)==EEvent::Dispute);
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
    def test_regional_crowns_and_dungeon_wall_art_are_visual_only(self):
        arena=(SRC/"MoonfallArena.cpp").read_text()
        dungeon=(SRC/"MoonfallDungeonLayout.cpp").read_text()
        header=(SRC/"MoonfallDungeonLayout.h").read_text()
        self.assertIn("MoonfallRoadLandmarks::ArtProfiles[Zone]",arena)
        self.assertIn("Art.CrownHeight*100.f+Art.TipHeight*50.f",arena)
        self.assertIn("bool bEnableWallCrownArt = true;",header)
        for token in ("WallCrownArt->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
                      "WallCrownArt->SetCollisionResponseToAllChannels(ECR_Ignore)",
                      "WallCrownArt->SetGenerateOverlapEvents(false)",
                      "WallCrownArt->SetCastShadow(false)",
                      "WallCrownArt->SetCanEverAffectNavigation(false)",
                      "WallCrownArt->SetCullDistances(8000, 12000)",
                      "WallCrownArt->ClearInstances()",
                      "MoonfallRoadLandmarks::ArtProfiles[ZoneIndex]",
                      "FVector(X,Side*ChamberWidth*0.5f,300.f+Art.CrownHeight*50.f)"):
            self.assertIn(token,dungeon)
        self.assertEqual(dungeon.count("WallCrownArt->AddInstance("),1)

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
        self.assertIn("bInConversation,bNearHero,bSheltering||bRegionalHome,bHoldPost||bRegionalWatch",movement)
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
        self.assertIn("TInlineAllocator<MoonfallLivingSimulation::ResidentBudget>> SocialCandidates",pulse)
        self.assertNotIn("SocialCandidates.Reserve(",pulse)
        self.assertIn("!bCombatNearby && !Hero->bResidentContextOpen && !bTalking",pulse)
        self.assertIn("TArrayView<const TWeakObjectPtr<AMoonfallWorldNPC>> Candidates",helper)
        self.assertEqual(helper.count("Speaker->GetActorLocation()"),1)
        self.assertIn("if (SpeakerLocation.ContainsNaN()) return;",helper)
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
        self.assertIn("if(bRecruited||ReturningCompanion.Get()==NPC)return;",mode)
        self.assertIn("LocalCompanionStance==EMoonfallCompanionStance::Hold",mode)
        self.assertIn("Bond,bGuide,bHold",mode)
        self.assertIn("bHoldCompanion&&!bInConversation&&!bSheltering&&!bHoldPost",npc)
        self.assertIn('LivingIntent=TEXT("Holding Position");return;',npc)

    def test_npc_interaction_rejects_invalid_state_before_visibility_trace(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        body=npc.split("bool AMoonfallWorldNPC::CanHeroInteract(",1)[1].split("\n}",1)[0]
        before_trace=body.split("LineTraceSingleByChannel(",1)[0]
        for guard in ("Hero->GetWorld()!=GetWorld()", "IsActorBeingDestroyed()",
                      "Hero->IsActorBeingDestroyed()", "!FMath::IsFinite(Hero->Health)",
                      "Hero->Health <= 0.f", "Hero->bAtlasOpen", "Hero->bTalentWebOpen",
                      "!FMath::IsFinite(InteractRange)"):
            self.assertIn(guard,before_trace)
        self.assertIn("const float Range = FMath::Clamp(InteractRange, 50.f, 400.f);",body)
        self.assertNotIn(": 240.f",body)

    def test_companion_hud_reads_live_party_without_actor_scan(self):
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        block=hud.split("// One cached party lookup, no actor search or extra world-space labels.",1)[1].split('DrawText(TEXT("ESC  OPTIONS")',1)[0]
        for token in ("!bCompactScreen","GetLocalCompanionForHero(Hero)","Hero->GetCompanionParty().ResidentId",
                      "FMath::IsFinite(Scout->CompanionHealth)","FMath::Clamp(Scout->CompanionHealth,0.f,100.f)",
                      'TEXT("DOWNED")','TEXT("FOLLOW")','TEXT("HOLD")','TEXT("PASSIVE")',
                      "HP<=25.f","(W-20.f)*HP/100.f"):
            self.assertIn(token,block)
        self.assertNotIn("TActorIterator",block)
        self.assertNotIn("SaveWorldProgress",block)
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        lookup=mode.split("const AMoonfallWorldNPC* AMoonfallUnrealGameMode::GetLocalCompanionForHero(",1)[1].split("\n}",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","Hero->Health<=0.f","IsCompanionPartyRestoreReady()",
                      "!Scout->IsActorBeingDestroyed()","Scout->GetWorld()==GetWorld()",
                      "Scout->ResidentId==Hero->GetCompanionParty().ResidentId"):
            self.assertIn(guard,lookup)
        self.assertNotIn("TActorIterator",lookup)

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
        self.assertIn("HasResidentContextPointerSurface(Width,Height)",mouse)
        self.assertIn("Hero->HasResidentContextPointerSurface(Canvas->SizeX,Canvas->SizeY)",panel)
        self.assertLess(mouse.index("HasResidentContextPointerSurface(Width,Height)"),mouse.index("ChooseResidentContextAction(Row)"))
        self.assertIn("GetResidentContextRowPitch(GetResidentContextPanelHeight(ViewportHeight)) >= 32.f",hero)
        self.assertIn("ViewportWidth >= 284.f && ViewportHeight >= 294.f",hero)
        self.assertIn("!FMath::IsFinite(MouseX) || !FMath::IsFinite(MouseY)",mouse)
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

    def test_v27_companion_cover_preserves_defensive_authority(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        cover=mode.split("float AMoonfallUnrealGameMode::ConsumeLocalCompanionCover(",1)[1].split(
            "bool AMoonfallUnrealGameMode::CanReceiveCompanionAttack(",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","LocalCompanionStance!=EMoonfallCompanionStance::Follow",
                      "Scout->GetBondWithHero(Hero)<60","GetFactionReputation(Scout->FactionId)<0",
                      "MoonfallLivingSimulation::Severe","FVector::DotProduct(ToSource,ToScout)<0.5",
                      "MoonfallWorldPolicy::CompanionCover","Now-LastCompanionCoverAt"):
            self.assertIn(guard,cover)
        self.assertEqual(cover.count("LineTraceSingleByChannel("),1)
        self.assertLess(cover.index("LineTraceSingleByChannel("),cover.index("LastCompanionCoverAt=Now"))
        self.assertNotIn("TakeMeleeDamage",cover)
        self.assertNotIn("TakeSpellDamage",cover)
        damage=hero.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)",1)[1].split(
            "void AMoonfallHeroPawn::CycleGraphicsQuality()",1)[0]
        self.assertIn("DamageAfterGuard>0.f&&bHasIncomingDamageSource&&!bIncomingDamageDodgeOnly",damage)
        self.assertLess(damage.index("OnGuardImpact("),damage.index("ConsumeLocalCompanionCover("))
        self.assertLess(damage.index("ConsumeLocalCompanionCover("),damage.index("Health = FMath::Max(0.f, Health - DamageAfterGuard)"))

    def test_v27_companion_strike_uses_recent_player_target_and_cannot_kill(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        enemy=(SRC/"MoonfallEnemy.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        strike=mode.split("void AMoonfallUnrealGameMode::TryLocalCompanionStrike(",1)[1].split(
            "bool AMoonfallUnrealGameMode::SetLocalCompanionStance(",1)[0]
        for condition in ("IsLocalCompanionOwner(Hero)","EMoonfallCompanionStance::Follow",
                          "GetRecentCompanionAssistTarget(Age)","CanCompanionStrike(Age,Now-LastCompanionStrikeAt",
                          "Enemy->bBoss||Enemy->bElite","Scout->GetBondWithHero(Hero)<60",
                          "Hero->bTalentWebOpen","LineTraceSingleByChannel"):
            self.assertIn(condition,strike)
        self.assertNotIn("TActorIterator",strike)
        self.assertNotIn("SetTimer",strike)
        self.assertLess(strike.index("LastCompanionStrikeAt=Now"),strike.index("TryCompanionSupportStrike(StrikeDamage)"))
        self.assertIn("if (bCombatNearby) TryLocalCompanionStrike(Hero,Now);",mode)
        self.assertIn("RecentCompanionAssistTarget=Enemy;RecentCompanionAssistHitAt=Time;",hero)
        self.assertIn("RecentCompanionAssistTarget=Enemy;RecentCompanionAssistHitAt=Now;",hero)
        native=enemy.split("bool AMoonfallEnemy::TryCompanionSupportStrike(float Amount)",1)[1].split(
            "void AMoonfallEnemy::TakeMeleeDamage(",1)[0]
        for condition in ("Health<=Amount","Amount>10.f","bPostureFinisherAvailable","Now<PostureBrokenUntil",
                          "bBoss||bElite","TakeMeleeDamage(Amount)"):
            self.assertIn(condition,native)

    def test_v27_companion_rank_uses_earned_bond_without_new_save_state(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        self.assertIn("CompanionRank(Scout->GetBondWithHero(Hero))",mode)
        self.assertIn("CompanionStrikeDamage(Rank,Role)",mode)
        self.assertIn("TryCompanionSupportStrike(StrikeDamage)",mode)
        self.assertIn("CompanionRank(Resident->GetBondWithHero(Hero))",hud)
        self.assertIn("Bond rank %d / 3",hud)
        self.assertIn("EndGuard();\n  RecentCompanionAssistTarget.Reset()",hero)

    def test_v27_companion_specialization_is_explicit_and_trust_gated(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        role=mode.split("bool AMoonfallUnrealGameMode::SetLocalCompanionRole(",1)[1].split(
            "bool AMoonfallUnrealGameMode::DismissLocalCompanion(",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","IsRecentlyInCombatForAmbientAI()",
                      "CanHeroInteract(Hero)","Rank<2","GetFactionReputation"):
            self.assertIn(guard,role)
        self.assertIn("LocalCompanionRole=Role",role)
        self.assertIn("Mode->SetLocalCompanionRole(this,Next)",hero)
        self.assertIn('TEXT("Balanced | Switch to Protector")',hud)
        self.assertIn('TEXT("Protector | Switch to Striker")',hud)
        self.assertIn('TEXT("Striker | Switch to Balanced")',hud)

    def test_v27_veteran_perks_are_earned_and_preserve_posture_authority(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        policy=(SRC/"MoonfallLivingWorldPolicy.h").read_text()
        self.assertIn("if(rank!=3)return CompanionPerk::None;",policy)
        for perk in ("Fieldcraft","ShieldExpert","Precision"):
            self.assertIn("CompanionPerk::"+perk,policy)
            self.assertIn("CompanionPerk::"+perk,hud)
        self.assertIn("elapsed+TrainingRecovery<CompanionCoverCooldown(rank,role)",policy)
        self.assertIn("Range=CompanionCoverRange(rank,role)",policy)
        self.assertIn("Range=CompanionStrikeRange(rank,role)",policy)
        strike=mode.split("void AMoonfallUnrealGameMode::TryLocalCompanionStrike(",1)[1].split(
            "bool AMoonfallUnrealGameMode::SetLocalCompanionStance(",1)[0]
        self.assertIn("CompanionExtraPosture(Rank,Role)",strike)
        self.assertLess(strike.index("if(!Enemy->TryCompanionSupportStrike(StrikeDamage))return;"),
                        strike.index("Enemy->ApplyPostureDamage(ExtraPosture)"))
        self.assertIn("!Enemy->bPostureFinisherAvailable&&Now>=Enemy->PostureBrokenUntil",strike)
        self.assertIn("Enemy->Health>0.f",strike)
        self.assertIn("!Enemy->bBoss&&!Enemy->bElite",strike)
        self.assertIn("CompanionRank==3",hud)

    def test_v27_companion_talent_page_has_direct_selection_and_safe_back(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        header=(SRC/"MoonfallHeroPawn.h").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        choose=hero.split("bool AMoonfallHeroPawn::ChooseResidentContextAction(",1)[1].split(
            "// Bounded, persistent faction relationship API.",1)[0]
        self.assertIn("bool bResidentCompanionTalents=false",header)
        self.assertIn("if(bResidentCompanionTalents)return 5;",hero)
        self.assertIn("Mode->GetLocalCompanion()==Resident?9:3;",hero)
        self.assertIn("345.f+53.f*(GetResidentContextRowCount()-3)",hero)
        self.assertIn("if(Choice==4){bResidentCompanionTalents=true;ResidentContextSelection=0;return true;}",choose)
        self.assertIn("if(Choice==3){bResidentCompanionTalents=false;bResidentCompanionTraining=false;ResidentContextSelection=0;return true;}",choose)
        self.assertIn("if(Choice<0||Choice>2)return false;",choose)
        self.assertIn("TalentMode->GetLocalCompanion()==Resident",choose)
        self.assertIn("TalentMode->SetLocalCompanionRole(this,static_cast<EMoonfallCompanionRole>(Choice))",choose)
        self.assertIn("bResidentCompanionMenu=false;bResidentCompanionTalents=false;bResidentCompanionTraining=false;",hero)
        self.assertLess(choose.index("if(bResidentCompanionTalents)"),choose.index("OfferResidentQuest(this)"))
        self.assertIn("FString Names[9]",hud)
        self.assertIn('TEXT("COMPANION TALENTS | Select a path")',hud)
        self.assertIn("Rank>=(Path==0?1:2)",hud)
        self.assertIn('TEXT("Perk at 100 trust")',hud)
        self.assertIn("EffectiveCompanionRole(Rank",hud)

    def test_v27_companion_training_budget_prerequisites_and_ui(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        policy=(SRC/"MoonfallLivingWorldPolicy.h").read_text()
        train=mode.split("bool AMoonfallUnrealGameMode::TrainLocalCompanionTalent(",1)[1].split(
            "bool AMoonfallUnrealGameMode::ResetLocalCompanionTalents(",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","IsRecentlyInCombatForAmbientAI()",
                      "CanHeroInteract(Hero)","GetFactionReputation","TrainCompanion(LocalCompanionTraining"):
            self.assertIn(guard,train)
        self.assertIn("if(Next==LocalCompanionTraining)return false;",train)
        self.assertIn("LocalCompanionTraining=Next",train)
        self.assertIn("CompanionNodeCount=9",policy)
        self.assertIn("CompanionPointsSpent(mask)>=CompanionPointBudget(rank)",policy)
        self.assertIn("CompanionNodeRank(mask,node-1)>0",policy)
        self.assertIn("CompanionPointsSpent(mask)>CompanionPointBudget(rank)",policy)
        self.assertIn("TrainingMode->TrainLocalCompanionTalent(this",hero)
        self.assertIn("TrainingMode->ResetLocalCompanionTalents(this)",hero)
        self.assertIn('TEXT("Train active path")',hud)
        self.assertIn('TEXT("Refund all training")',hud)
        self.assertIn("TRAINING | %d / %d points spent",hud)
        self.assertIn("ActiveCompanionTraining(LocalCompanionTraining,Rank,Role,7)",mode)

    def test_v27_companion_profiles_are_saved_validated_and_transactional(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        save=(SRC/"MoonfallPersistentSave.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        self.assertIn("Version = 17",save)
        self.assertIn("TMap<FName,int32> CompanionProfiles",save)
        self.assertIn("Save->CompanionProfiles = CompanionProfiles;",hero)
        self.assertIn("CompanionProfiles.Reset();",hero)
        self.assertIn("if(Save->Version>=14)CompanionProfiles=Save->CompanionProfiles;",hero)
        self.assertIn("Save->CompanionProfiles.Num()>MoonfallQuestJournal::ResidentCount",hero)
        self.assertIn("CompanionProfiles.Num()>MoonfallQuestJournal::ResidentCount",hero)
        transaction=hero.split("bool AMoonfallHeroPawn::CommitCompanionProfile(",1)[1].split(
            "bool AMoonfallHeroPawn::ChangeResidentBond(",1)[0]
        self.assertIn("HasAuthority()",transaction)
        self.assertIn("IsNamedMoonfallResident(ResidentId)",transaction)
        self.assertIn("ValidCompanionProfile(Profile)",transaction)
        self.assertIn("if(!SaveWorldProgress())",transaction)
        self.assertIn("CompanionProfiles.Add(ResidentId,static_cast<int32>(Previous))",transaction)
        recruit=mode.split("bool AMoonfallUnrealGameMode::RecruitLocalCompanion(",1)[1].split(
            "float AMoonfallUnrealGameMode::ConsumeLocalCompanionCover(",1)[0]
        self.assertIn("Hero->GetCompanionProfile(Resident->ResidentId)",recruit)
        self.assertIn("Profile&MoonfallWorldPolicy::CompanionTrainingMask",recruit)
        for method,next_method,mutation in (
            ("SetLocalCompanionRole","TrainLocalCompanionTalent","LocalCompanionRole=Role"),
            ("TrainLocalCompanionTalent","ResetLocalCompanionTalents","LocalCompanionTraining=Next"),
            ("ResetLocalCompanionTalents","DismissLocalCompanion","LocalCompanionTraining=0")):
            section=mode.split("bool AMoonfallUnrealGameMode::"+method+"(",1)[1].split(
                "bool AMoonfallUnrealGameMode::"+next_method+"(",1)[0]
            self.assertLess(section.index("CommitCompanionProfile"),section.index(mutation))

    def test_v27_scout_deeds_are_earned_once_and_mood_compiles(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        deeds=hero.split("bool AMoonfallHeroPawn::ReportScoutDeeds(",1)[1].split(
            "uint32 AMoonfallHeroPawn::GetCompanionProfile(",1)[0]
        for guard in ("HasAuthority()","Resident->CanHeroInteract(this)","IsRecentlyInCombatForAmbientAI()",
                      "GetTotalDefeatedDungeonBosses()","GetResidentQuestState(Resident->ResidentId)==2",
                      "GetFactionReputation(Resident->FactionId)","IsNamedMoonfallResident(Resident->ResidentId)",
                      "Target>Current&&ChangeResidentBond(Resident->ResidentId,Target-Current)"):
            self.assertIn(guard,deeds)
        self.assertIn("(bRecruited&&Choice==5)||(!bRecruited&&Choice==1)",hero)
        self.assertIn('Names[5]=TEXT("Report deeds")',hud)
        self.assertIn("const auto ResolvedMood=MoonfallWorldPolicy::ResolveMood",npc)
        self.assertIn("switch(ResolvedMood)",npc)
        self.assertNotIn("const auto Mood=MoonfallWorldPolicy::ResolveMood",npc)
        guide=mode.split("const bool bGuideSafe=",1)[1].split("NextResidentPulseIndex=",1)[0]
        self.assertIn("!Hero->bTalentWebOpen",guide)
        self.assertNotIn("FocusedResident.IsValid()",guide)

    def test_v27_follower_navigation_commands_and_recovery(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        self.assertIn("PulseFollower,0.2f,true",mode)
        self.assertIn("ClearTimer(FollowerPulseTimer)",mode)
        self.assertIn("if(bRecruited||ReturningCompanion.Get()==NPC)return;",mode)
        self.assertIn("AdvanceFollowerToward(Hero->GetActorLocation(),Now,180.f)",mode)
        self.assertIn("AdvanceFollowerToward(Returning->AuthoredHomeLocation,Now,40.f)",mode)
        self.assertIn("AdvanceFollowerToward(OrderedFollowerDestination,Now,45.f)",mode)
        self.assertIn("NextFollowerPathAt=Now+1.f",npc)
        self.assertIn("FollowerPathGoal=Goal",npc)
        self.assertIn("FVector::DistSquared(Goal,FollowerPathGoal)>FMath::Square(120.0)",npc)
        self.assertIn("if(Distance<=StopDistance&&bSameFloor){bFollowerWaypointValid=false;FollowerBlockedSteps=0;FollowerLastStep=0.0;return true;}",npc)
        self.assertIn("FollowerStepBudget(Remaining,FollowerLastStep,Now-FollowerLastMoveAt,MaxStep)",npc)
        self.assertIn("float MaxStep=60.f",npc)
        self.assertIn("FollowerLastStep=FMath::Min(60.0,ActualMove.Size2D())",npc)
        self.assertIn("!bFollowerWaypointValid&&bSameFloor&&FollowerBlockedSteps>=2",npc)
        self.assertIn("FollowerBlockedSteps>=7?2:FollowerBlockedSteps+1",npc)
        self.assertIn("else MarkBlocked();",npc)
        move=npc.split("bool AMoonfallWorldNPC::AdvanceFollowerToward(",1)[1].split("bool AMoonfallWorldNPC::TrySafeFollowerCatchup(",1)[0]
        self.assertEqual(move.count("LineTraceSingleByChannel("),3)
        self.assertEqual(move.count("FindPathToLocationSynchronously("),1)
        self.assertLess(move.index("if(bDetour)"),move.index("LineTraceSingleByChannel("))
        self.assertIn("ActualMove=GetActorLocation()-Start",npc)
        self.assertIn("FMath::FixedTurn(CurrentYaw,DesiredYaw,45.f)",npc)
        self.assertIn("FMath::IsFinite(CurrentYaw)&&FMath::IsFinite(DesiredYaw)",npc)
        self.assertIn("Velocity.SizeSquared2D()>FMath::Square(120.0)",mode)
        self.assertIn("Player-Forward*200.0+Side*90.0*Sign,Now,85.f",mode)
        self.assertIn("FMath::Abs(Velocity.Z)<100.0&&!bThreat",mode)
        self.assertIn("FindPathToLocationSynchronously",npc)
        self.assertIn("Ground.ImpactNormal.Z<0.65",npc)
        self.assertIn("AddActorWorldOffset(Move,true)",npc)
        self.assertIn("Scout->CompanionHealth-Cover",mode)
        self.assertIn("Scout->bCompanionDowned=true",mode)
        self.assertIn("Scout->CompanionHealth=50.f",mode)
        self.assertIn("Mode->OrderCompanionMove(this",hero)
        self.assertIn("Mode->ReviveLocalCompanion(this)",hero)
        self.assertIn('TEXT("Move ahead")',hud)
        self.assertIn('TEXT("Revive companion")',hud)

    def test_v27_companion_equipment_transfer_is_owned_and_transactional(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        save=(SRC/"MoonfallPersistentSave.h").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        self.assertIn("Version = 17",save)
        self.assertIn("TMap<FName,FMoonfallCompanionGearRecord> CompanionGear",save)
        self.assertIn("Save->CompanionGear = CompanionGear;",hero)
        self.assertIn("Save->CompanionGear.Num()>MoonfallQuestJournal::ResidentCount",hero)
        self.assertIn("if(Save->Version>=15)CompanionGear=Save->CompanionGear;",hero)
        trade=hero.split("bool AMoonfallHeroPawn::TradeCompanionGear(",1)[1].split(
            "uint32 AMoonfallHeroPawn::GetCompanionProfile(",1)[0]
        for guard in ("HasAuthority()","CanHeroInteract(this)","IsRecentlyInCombatForAmbientAI()",
                      "Mode->GetLocalCompanion()!=Resident","Code==Worn[Slot]","InventoryItemCodes.Num()>=10000"):
            self.assertIn(guard,trade)
        self.assertIn("InventoryItemCodes.RemoveAt(Index)",trade)
        self.assertIn("InventoryItemCodes.Add(Old)",trade)
        self.assertIn("if(!SaveWorldProgress())",trade)
        self.assertIn("InventoryItemCodes=PreviousBag",trade)
        self.assertIn("CompanionGear.Add(Id,Previous)",trade)
        self.assertIn("TradeCompanionGear(Resident,Choice%3,Choice>=3)",hero)
        self.assertIn('TEXT("Trade equipment")',hud)
        self.assertIn("GetCompanionGearTier(Scout->ResidentId,0)",mode)
        self.assertIn("GetCompanionGearTier(Scout->ResidentId,1)",mode)

    def test_v27_companion_threat_is_event_driven_and_bounded(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        enemy=(SRC/"MoonfallEnemy.cpp").read_text()
        header=(SRC/"MoonfallUnrealGameMode.h").read_text()
        self.assertIn("TWeakObjectPtr<AMoonfallEnemy> CompanionThreat",header)
        self.assertIn("!bBoss&&!bElite&&!bSanctuaryPractice&&bCanSeeHero",enemy)
        self.assertIn("Mode->ReportCompanionThreat(Hero,this,Now)",enemy)
        report=mode.split("void AMoonfallUnrealGameMode::ReportCompanionThreat(",1)[1].split(
            "void AMoonfallUnrealGameMode::TryLocalCompanionStrike(",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","Enemy->bBoss||Enemy->bElite",
                      "LocalCompanion->bCompanionDowned","CanReportCompanionThreat",
                      "Now-CompanionThreatAt<=2.f","CompanionThreat=Enemy;CompanionThreatAt=Now"):
            self.assertIn(guard,report)
        self.assertNotIn("TActorIterator",report)
        strike=mode.split("void AMoonfallUnrealGameMode::TryLocalCompanionStrike(",1)[1].split(
            "bool AMoonfallUnrealGameMode::SetLocalCompanionStance(",1)[0]
        self.assertLess(strike.index("GetRecentCompanionAssistTarget(Age)"),strike.index("Enemy=CompanionThreat.Get()"))
        self.assertIn("Age<0.f||Age>2.f",strike)
        self.assertIn("if(!CanAssist(Enemy,Age))",strike)
        self.assertIn("Age>2.f||!CanAssist(Enemy,Age)",strike)
        self.assertIn("!FMath::IsFinite(Enemy->Health)",strike)
        self.assertIn("Enemy->bPostureFinisherAvailable||Now<Enemy->PostureBrokenUntil",strike)
        self.assertLess(strike.index("CanCompanionStrike(Age"),strike.index("Enemy=CompanionThreat.Get()"))
        self.assertEqual(strike.count("LineTraceSingleByChannel("),1)
        self.assertIn("Enemy->TryCompanionSupportStrike(StrikeDamage)",strike)
        self.assertIn("&&bFreshThreat)TryLocalCompanionStrike(Hero,Now)",mode)
        self.assertIn("FreshCompanionThreat(Now,CompanionThreatAt,Existing->Health",report)
        self.assertIn("CanCompanionSupportOffensively(Scout->CompanionHealth)",strike)
        self.assertIn("const bool bFreshThreat=IsFreshThreat(CompanionThreat.Get())",mode)
        self.assertIn("if(!bFreshThreat){CompanionThreat.Reset();CompanionThreatAt=-1000.f;}",mode)
        self.assertIn("FMath::Abs(Player.Z-Threat->GetActorLocation().Z)<=100.0",mode)
        self.assertIn("PreviousIntent!=Scout->LivingIntent)ResetFollowerMomentum()",mode)
        self.assertIn("HeroSpeed>=550.0?80.f:HeroSpeed>=300.0?70.f:60.f",mode)
        self.assertIn("Player-Forward*200.0+Side*90.0*Sign,Now,85.f,AdaptiveMaxStep",mode)
        self.assertIn("Scout->FollowerLastMoveAt=-1.f",mode)
        self.assertIn("LocalCompanion.Get()!=Scout||!IsLocalCompanionOwner(Hero)",mode)
        self.assertIn("CompanionThreat.Reset();CompanionThreatAt=-1000.f",mode)

    def test_v27_companion_counterattack_is_committed_and_has_no_hero_fallback(self):
        enemy=(SRC/"MoonfallEnemy.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        policy=(SRC/"MoonfallLivingWorldPolicy.h").read_text()
        self.assertIn("LockedCompanionTarget=Target;bCompanionAttackCommitted=true",enemy)
        self.assertIn("RecentCompanionAttacker.Get()==Scout",enemy)
        self.assertIn("Now-RecentCompanionAttackerAt<=8.f",enemy)
        commit=enemy.split("if(bCompanionAttackCommitted){",1)[1].split("if (bBoss)",1)[0]
        self.assertIn("LockedCompanionTarget.Get();LockedCompanionTarget.Reset()",commit)
        self.assertIn("Mode->ResolveCompanionCounterAttack",commit)
        self.assertIn("return;",commit)
        self.assertNotIn("Hero->ApplyMoonfallDamage",commit)
        receive=mode.split("void AMoonfallUnrealGameMode::ResolveCompanionCounterAttack(",1)[1].split(
            "float AMoonfallUnrealGameMode::GetCompanionAbilityRemaining(",1)[0]
        for guard in ("CanReceiveCompanionAttack(Hero,Target)","Enemy->bBoss||Enemy->bElite",
                      "CanCompanionCounter","LineTraceSingleByChannel","FollowerIncomingDamage",
                      "Target->CompanionHealth-Actual","Target->bCompanionDowned=true"):
            self.assertIn(guard,receive)
        self.assertIn("Enemy->RememberCompanionAttacker(Scout,Now)",mode)
        self.assertIn("CanCompanionCounter",policy)
        eligibility=mode.split("bool AMoonfallUnrealGameMode::CanReceiveCompanionAttack(",1)[1].split("\n}",1)[0]
        for modal in ("bInventoryOpen","bOptionsOpen","bAtlasOpen","bTalentWebOpen","bCharacterStatsOpen","bSpellBookOpen","bResidentContextOpen"):
            self.assertNotIn(modal,eligibility)
        for guard in ("IsLocalCompanionOwner(Hero)","!Scout->bCompanionDowned","FMath::IsFinite(Scout->CompanionHealth)","Hero->Health>0.f"):
            self.assertIn(guard,eligibility)

    def test_v27_companion_tactics_and_active_abilities(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        hud=(SRC/"MoonfallCombatHUD.cpp").read_text()
        policy=(SRC/"MoonfallLivingWorldPolicy.h").read_text()
        ability=mode.split("bool AMoonfallUnrealGameMode::ActivateLocalCompanionAbility(",1)[1].split(
            "void AMoonfallUnrealGameMode::ReportCompanionThreat(",1)[0]
        for guard in ("IsLocalCompanionOwner(Hero)","Scout->bCompanionDowned","Scout->CanHeroInteract(Hero)",
                      "CanActivateCompanionAbility","CompanionAbilityCooldown","GetCompanionGearTier(Scout->ResidentId,2)"):
            self.assertIn(guard,ability)
        self.assertIn("Scout->CompanionHealth+25.f",ability)
        self.assertIn("Now+(Role==1?6.f:8.f)",ability)
        self.assertIn("bPrecisionBurst?8.f:0.f",mode)
        self.assertIn("if(bPrecisionBurst)CompanionAbilityEndsAt=-1000.f",mode)
        self.assertIn("SelectCompanionTactic",mode)
        for label in ("Intercepting","Flanking","Withdrawing"):
            self.assertIn('TEXT("'+label+'")',mode)
        for label in ("Second Wind","Brace","Precision Burst"):
            self.assertIn('TEXT("'+label+'")',hud)
        self.assertIn("Mode->ActivateLocalCompanionAbility(this)",hero)
        self.assertIn("GetCompanionAbilityRemaining()",hud)
        self.assertIn("CompanionAbilityCooldown",policy)

    def test_companion_commands_reject_invalid_hero_before_save(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        for method in ("CommitCompanionParty", "CommitCompanionProfile"):
            body=hero.split("bool AMoonfallHeroPawn::"+method+"(",1)[1].split("\n}",1)[0]
            before_save=body.split("SaveWorldProgress()",1)[0]
            for guard in ("HasAuthority()", "IsActorBeingDestroyed()", "!FMath::IsFinite(Health)", "Health<=0.f"):
                self.assertIn(guard,before_save)
        owner=mode.split("bool AMoonfallUnrealGameMode::IsLocalCompanionOwner(",1)[1].split("\n}",1)[0]
        self.assertIn("!Hero->IsActorBeingDestroyed()",owner)
        self.assertIn("FMath::IsFinite(Hero->Health)",owner)
        dismiss=mode.split("bool AMoonfallUnrealGameMode::DismissLocalCompanion(",1)[1].split("\n}",1)[0]
        self.assertLess(dismiss.index("Hero->CommitCompanionParty(Next)"),dismiss.index("LocalCompanion.Reset()"))

    def test_v27_companion_party_persistence_and_restoration(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        save=(SRC/"MoonfallPersistentSave.h").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        self.assertIn("Version = 17",save)
        self.assertIn("FMoonfallCompanionPartyRecord CompanionParty",save)
        self.assertIn("Save->CompanionParty = CompanionParty;",hero)
        self.assertIn("CompanionParty=Save->Version>=16?Save->CompanionParty:FMoonfallCompanionPartyRecord{}",hero)
        commit=hero.split("bool AMoonfallHeroPawn::CommitCompanionParty(",1)[1].split("void AMoonfallHeroPawn::UpdateCompanionVitals(",1)[0]
        self.assertIn("ValidCompanionParty",commit)
        self.assertIn("if(!SaveWorldProgress()){CompanionParty=Previous;return false;}",commit)
        restore=mode.split("void AMoonfallUnrealGameMode::TryRestoreCompanionParty(",1)[1].split("void AMoonfallUnrealGameMode::SyncCompanionVitals(",1)[0]
        for guard in ("IsCompanionPartyRestoreReady()","LocalCompanion.IsValid()","LivingResidents",
                      "if(Match)return","Match->GetBondWithHero(Hero)<60",
                      "AttachCompanionParty(Hero,Match,Saved.Stance,Saved.Health)"):
            self.assertIn(guard,restore)
        self.assertNotIn("SpawnActor",restore)
        recruit=mode.split("bool AMoonfallUnrealGameMode::RecruitLocalCompanion(",1)[1].split("void AMoonfallUnrealGameMode::AttachCompanionParty(",1)[0]
        duplicate_guard="IsValid(Other)&&Other!=Resident&&!Other->IsActorBeingDestroyed()&&Other->ResidentId==Resident->ResidentId"
        self.assertIn(duplicate_guard,recruit)
        self.assertLess(recruit.index(duplicate_guard),recruit.index("Hero->CommitCompanionParty(Next)"))
        saved_slot_guard="!Hero->IsCompanionPartyRestoreReady()||(!Next.ResidentId.IsNone()&&Next.ResidentId!=Resident->ResidentId)"
        self.assertIn(saved_slot_guard,recruit)
        self.assertLess(recruit.index(saved_slot_guard),recruit.index("Next.Health="))
        self.assertLess(recruit.index(saved_slot_guard),recruit.index("Hero->CommitCompanionParty(Next)"))
        self.assertIn("TryRestoreCompanionParty(Hero);",mode)
        self.assertIn("Resident->bCompanionDowned=Health<=0.f",mode)
        self.assertIn("CompanionAbilityReadyUtc-FDateTime::UtcNow().ToUnixTimestamp()",mode)
        self.assertIn("Next.AbilityReadyUtc=CompanionAbilityReadyUtc",mode)
        self.assertIn("SyncCompanionVitals(Hero);",mode)
        self.assertIn("if(!Hero->CommitCompanionParty(Next))return false;",mode)

    def test_v27_follower_catchup_requires_safe_ground_and_sustained_separation(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        policy=(SRC/"MoonfallLivingWorldPolicy.h").read_text()
        social=(SRC/"MoonfallNpcSocialSessionSubsystem.cpp").read_text()
        recovery=npc.split("bool AMoonfallWorldNPC::TrySafeFollowerCatchup(",1)[1].split(
            "void AMoonfallWorldNPC::WitnessNearbyDanger(",1)[0]
        for guard in ("HasAuthority()","bCompanionDowned","IsMovingOnGround()",
                      "IsRecentlyInCombatForAmbientAI()","ProjectPointToNavigation",
                      "Ground.ImpactNormal.Z<0.7","OverlapAnyTestByObjectType",
                      "LineTraceSingleByChannel(Sight","TeleportTo(Destination",
                      "bFollowerWaypointValid=false"):
            self.assertIn(guard,recovery)
        self.assertLess(recovery.index("OverlapAnyTestByObjectType"),recovery.index("TeleportTo(Destination"))
        self.assertIn("LocalCompanionStance==EMoonfallCompanionStance::Follow&&!bFollowerMoveOrdered",mode)
        self.assertIn("if(!bRecoverySafe)FollowerSeparatedAt=-1.f",mode)
        self.assertIn("CanRecoverFollower(Separation,Now-FollowerSeparatedAt,true)",mode)
        self.assertIn("NextFollowerCatchupAt=Now+5.f",mode)
        self.assertIn("distanceSquared>=1800.0*1800.0",policy)
        self.assertIn("separatedSeconds>=8.0",policy)
        self.assertIn("std::numeric_limits<int64>::max()",social)

    def test_v27_companion_gear_and_recovery_regressions(self):
        hero=(SRC/"MoonfallHeroPawn.cpp").read_text()
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        trade=hero.split("bool AMoonfallHeroPawn::TradeCompanionGear(",1)[1].split(
            "bool AMoonfallHeroPawn::CommitCompanionParty(",1)[0]
        self.assertIn("BestScore=GetItemPower(Old)",trade)
        self.assertIn("Score=GetItemPower(Code)",trade)
        self.assertIn("if(Score>BestScore)",trade)
        self.assertIn("Code==Worn[Slot]",trade)
        self.assertIn("if(!bFreshThreat&&!Hero->IsRecentlyInCombatForAmbientAI())",mode)
        self.assertIn("||Hero->bResidentContextOpen){FollowerSeparatedAt=-1.f;return;}",mode)
        self.assertIn("!FMath::IsFinite(Scout->CompanionHealth)",mode)
        self.assertIn("&& !bFreshThreat".replace(" ",""),mode.replace(" ",""))
        self.assertIn("Distance<=StopDistance&&bSameFloor",npc)
        self.assertIn("FMath::Abs(GetActorLocation().Z-Goal.Z)<=150.0",npc)
        self.assertIn("FMath::Max(0.0,Distance-StopDistance)",npc)

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
        self.assertIn("bGuard && !bRegionalWatch && !bInConversation",movement)
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
        self.assertIn('Disposition==TEXT("Friendly")',talk)
        self.assertIn("&&Bond>-60&&Bond<60",talk)
        self.assertLess(talk.index('Disposition==TEXT("Friendly")'),talk.index("SupportsResidentContract"))
        self.assertIn("RegionalDanger>=65&&RegionalDanger<=100",talk)
        self.assertIn("Reaction.ToString()+RegionalWarning",talk)
        for role in ("Merchant","Guard","Scout"):
            self.assertIn('SocialRole==FName(TEXT("'+role+'"))',talk)
        self.assertNotIn("ChangeFactionReputation(",talk)
        self.assertIn("Mode->GetLocalCompanionForHero(Hero)==this",talk)
        self.assertIn("bActiveCompanion&&Disposition",talk)
        self.assertIn("!bAmbientVisitor&&!bActiveCompanion&&Disposition",talk)
        self.assertLess(talk.index("if(bCompanionDowned)PartyLine"),talk.index("Mode->LocalCompanionStance==EMoonfallCompanionStance::Hold"))
        self.assertLess(talk.index("SupportsResidentContract"),talk.index("Reaction.ToString()+PartyLine"))
        self.assertNotIn("SetLocalCompanionStance(",talk)
        self.assertNotIn("SetLocalCompanionRole(",talk)
        self.assertIn("!Hero->IsRecentlyInCombatForAmbientAI()",talk)
        self.assertIn("Hero->GetTotalDefeatedDungeonBosses()",talk)
        self.assertIn("Hero->GetTotalDiscoveredDungeons()",talk)
        self.assertIn("if(!bCompanionDowned&&Mode->LocalCompanionStance==EMoonfallCompanionStance::Follow",talk)
        self.assertLess(talk.index("if(bActiveCompanion&&Disposition"),talk.index("Hero->GetTotalDefeatedDungeonBosses()"))
        self.assertNotIn("MarkDungeonBossDefeated(",talk)
        self.assertNotIn("DiscoverDungeonEntrance(",talk)

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

    def test_regional_geometry_reserves_instances_and_weather_keeps_zone_identity(self):
        arena=(SRC/"MoonfallArena.cpp").read_text()
        region=arena.split("void AMoonfallArena::CreateRegionalLandmarks()",1)[1].split(
            "void AMoonfallArena::UpdateRegionWeather(",1)[0]
        self.assertLess(region.index("Details->PreAllocateInstancesMemory("),
                        region.index("Details->AddInstance("))
        self.assertLess(region.index("RegionWeatherLights.SetNum(6);"),
                        region.index("RegionWeatherLights[Zone]=Light;"))
        self.assertNotIn("RegionWeatherLights.Add(Light)",region)
        weather=arena.split("void AMoonfallArena::UpdateRegionWeather(",1)[1].split(
            "void AMoonfallArena::BeginPlay()",1)[0]
        self.assertIn("RegionWeatherLights[Zone]",weather)
        self.assertIn("if (!IsValid(Light)) continue;",weather)
        self.assertIn("WeatherFor(Zone,Phase)",weather)
        self.assertIn("LastRegionWeatherPhase=-1;", region)
        self.assertIn("for (int32& WeatherId : AppliedRegionWeather) WeatherId=INDEX_NONE;", region)
        self.assertLess(weather.index("if (AppliedRegionWeather[Zone]==WeatherId) continue;"),
                        weather.index("Light->SetLightColor(Tint);"))
        self.assertLess(weather.index("Light->SetIntensity(Intensity);"),
                        weather.index("AppliedRegionWeather[Zone]=WeatherId;"))
        self.assertIn("const int32 WeatherId=static_cast<int32>(Weather);", weather)
        self.assertLess(arena.index("DungeonRoad->PreAllocateInstancesMemory(30);"),
                        arena.index("DungeonRoad->AddInstance("))

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
        self.assertIn("EncounterDialogueVariant(Region,Phase)",mode)
        for group in ("MerchantGreetings","PatrolGreetings","WayfarerGreetings","DisputeGreetings"):
            self.assertIn(group+"[3]",mode)
            self.assertIn("FText::FromString("+group+"[DialogueVariant])",mode)
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

    def test_ambient_spawn_gate_includes_every_modal_without_freezing_existing_events(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1]
        gate=pulse.split("const bool bModal=",1)[1].split("if (!MoonfallLivingSimulation::MayEncounter(",1)[0]
        for modal in ("bInventoryOpen","bOptionsOpen","bAtlasOpen","bCharacterStatsOpen",
                      "bSpellBookOpen","bResidentContextOpen","bTalentWebOpen"):
            self.assertIn("Hero->"+modal,gate)
        self.assertIn("Hero->FocusedResident.IsValid()",gate)
        self.assertIn("bCombatNearby",gate)
        self.assertLess(pulse.index("ShouldRetireAmbient("),pulse.index("const bool bModal="))
        self.assertLess(pulse.index("MayEncounter("),pulse.index("World->LineTraceSingleByChannel("))

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

    def test_waystation_contract_catalog_matches_spawned_residents(self):
        import re
        arena=(SRC/"MoonfallArena.cpp").read_text()
        journal=(SRC/"MoonfallQuestJournal.h").read_text()
        sites=arena.split("static const FWaystationSite Sites[6] = {",1)[1].split(
            "RegionalWaystationFloors->ClearInstances()",1)[0]
        rows=re.findall(r'\{TEXT\("([^"]+)"\), TEXT\("(waystation_[a-z]+)"\), TEXT\("[^"]+"\),\s*TEXT\("(waystation_[a-z_]+)"\), TEXT\("([^"]+)"\),\s*TEXT\("(waystation_[a-z_]+)"\), TEXT\("([^"]+)"\),[\s\S]*?, (\d+), (\d+)\}',sites)
        self.assertEqual(len(rows),6)
        entries=re.findall(r'\{"(waystation_[a-z_]+)", "([^"]+)", "[^"]+", "([^"]+)", (\d+)\}',journal)
        self.assertEqual(len(entries),12)
        by_id={key:(name,location.upper(),int(race)) for key,name,location,race in entries}
        self.assertEqual(len(by_id),12)
        self.assertEqual(len({row[1] for row in rows}),6)
        for location,_,scout,scout_name,companion,companion_name,scout_race,companion_race in rows:
            self.assertEqual(by_id[scout],(scout_name,location,int(scout_race)))
            self.assertEqual(by_id[companion],(companion_name,location,int(companion_race)))
        self.assertIn("RegionalWaystationRuins->SetCanEverAffectNavigation(false)",arena)
        self.assertIn("FVector(ZoneCenterX, -1800.f, -20.f), FVector(7.4f, 9.6f, 0.4f)",arena)
        self.assertIn("FVector(X, -2500.f, -20.f), FVector(13.5f, 9.f, 0.4f)",arena)
        # The camp floor and road overlap by 230 units and share Z=0.
        self.assertEqual((-2050)-(-2280),230)
        # Camp centers are far from the central arena and dungeon chamber strip.
        for zone in range(6):
            center=(zone*5+2-14.5)*1350
            self.assertGreater(abs(center)-370,1500)

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
        self.assertIn("return NamedMoonfallResidentIds().Contains(Id);",validator)
        catalog=(SRC/"MoonfallQuestJournal.h").read_text()
        for name in ("CrossroadsGuide","FirstlightArchivist","FirstlightWatchkeeper"):
            self.assertIn('{"'+name+'",',catalog)
        builder=hero.split("const TArray<FName>& NamedMoonfallResidentIds() {",1)[1].split(
            "bool IsNamedMoonfallResident(FName Id) {",1)[0]
        self.assertIn("for (const auto& Entry : MoonfallQuestJournal::Residents)",builder)
        self.assertIn("Result.Add(FName(UTF8_TO_TCHAR(Entry.Id)))",builder)
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

    def test_reactive_regional_danger_reuses_existing_scheduler(self):
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        policy=(SRC/"MoonfallLivingSimulation.h").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1].split("void AMoonfallUnrealGameMode::SetDungeonSlotReady",1)[0]
        self.assertIn("StepRegionalPressure(RegionalPressure[Z]",pulse)
        self.assertIn("Hero->HasDefeatedDungeonBoss(Z,D)",pulse)
        self.assertIn("bCombatNearby&&LocalRegion==Z",pulse)
        self.assertLess(pulse.index("StepRegionalPressure"),pulse.index("if (!bHeroReady) return;"))
        self.assertLess(pulse.index("MayEncounter("),pulse.index("ReactiveEncounter("))
        self.assertLess(pulse.index("Ground.GetComponent()!=LivingArena->DungeonRoad"),pulse.index("ReactiveEncounter("))
        self.assertIn("PlannedEvent,CurrentWeather,Danger,RegionalBossVictories[Region]",pulse)
        self.assertNotIn("SetTimer(",pulse)
        self.assertIn('static_assert(StepRegionalPressure(58,true)==60',policy)
        self.assertIn('static_assert(ReactiveEncounter(EEvent::Monster,EWeather::Storm,10,5)==EEvent::Rescue',policy)

    def test_regional_concern_is_not_witness_memory(self):
        npc=(SRC/"MoonfallWorldNPC.cpp").read_text()
        query=npc.split("int32 AMoonfallWorldNPC::GetLocalRegionalDanger()",1)[1].split("\n}",1)[0]
        self.assertIn('SettlementId==FName(TEXT("Firstlight"))',query)
        self.assertIn("AuthoredHomeLocation.Y>-1800.f",query)
        self.assertIn("bAmbientVisitor||!bUsePersonalityRoutines",query)
        routine=npc.split("void AMoonfallWorldNPC::AdvanceLivingRoutine(",1)[1].split("\n}",1)[0]
        self.assertIn("RegionalRoutine(Role,GetLocalRegionalDanger()",routine)
        self.assertIn("!bInConversation&&!bSheltering&&!bHoldPost",routine)
        self.assertIn("RegionalRoutineAllowsGuide(Regional)",routine)
        self.assertNotIn("WitnessNearbyDanger(",routine)
        self.assertNotIn("PublishObservation(",routine)
        self.assertIn("RegionalRoutineStepLimit(Regional)",routine)
        self.assertIn("bSheltering||bRegionalHome,bHoldPost||bRegionalWatch",routine)
        self.assertIn("if(bRegionalWatch){Side=0.f;Front=0.f;}",routine)
        self.assertIn("GetClampedToMaxSize2D(RoutineStep)",routine)
        self.assertIn("if(bGuide||bRegionalHome||bRegionalWatch||bSheltering||bHoldPost)",routine)
        self.assertLess(routine.index("if(bGuide||bRegionalHome||bRegionalWatch||bSheltering||bHoldPost)"),routine.index("LineTraceSingleByChannel"))
        self.assertIn("bReviewingCombatReport && !bRegionalWatch && !bHoldPost",routine)
        self.assertIn("bGuard && !bRegionalWatch && !bInConversation",routine)
        self.assertLess(routine.index("LineTraceSingleByChannel"),routine.index("AddActorWorldOffset"))
        activity=npc.split("FString AMoonfallWorldNPC::GetCurrentActivity() const",1)[1].split("\n}",1)[0]
        self.assertIn("RegionalRoutine(Role,GetLocalRegionalDanger()",activity)
        self.assertIn("bRegionalIntent&&Regional!=MoonfallLivingSimulation::ERegionalRoutine::Normal",activity)
        self.assertIn("Regional!=MoonfallLivingSimulation::ERegionalRoutine::GuardWatch",activity)
        self.assertIn("!LivingIntent.IsEmpty() && !bRegionalIntent",activity)
        self.assertIn('LivingIntent!=TEXT("Seeking Shelter")',activity)
        mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text()
        pulse=mode.split("void AMoonfallUnrealGameMode::PulseLivingWorld()",1)[1]
        self.assertLess(pulse.index("RegionalAllowsSocial(NPC->GetLocalRegionalDanger())"),pulse.index("SocialCandidates.Add(NPC)"))
        self.assertIn('NPC->LivingIntent!=TEXT("Meal Break")',pulse)

if __name__=="__main__":unittest.main()
