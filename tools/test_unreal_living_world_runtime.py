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
        self.assertIn("Scanned<MoonfallLivingSimulation::ResidentBudget",mode)
        self.assertIn("NPC->AdvanceLivingRoutine(Now,HeroLocation);",mode)
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
        self.assertLess(pulse.index("Active->Destroy();"),pulse.index("ActiveAmbientEncounter.Reset();"))
        self.assertIn("float ActiveAmbientExpiresAt = -1.f;",header)
        self.assertIn("!conversationActive&&!monsterBattleActive",policy)
        self.assertIn("Spawned->SetLifeSpan(0.f);",pulse)
        self.assertNotIn("Spawned->SetLifeSpan(100.f);",pulse)
        self.assertIn("ActiveAmbientExpiresAt=Now+100.f;",pulse)

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
