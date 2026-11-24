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
#include <cassert>
#include <limits>
using namespace MoonfallLivingSimulation;
int main(){
 assert(RegionFromWorldX(-16875.f)==0);
 assert(RegionFromWorldX(0.f)==3);
 assert(RegionFromWorldX(16875.f)==5);
 assert(RegionFromWorldX(20250.f)==-1);
 assert(RegionFromWorldX(std::numeric_limits<float>::quiet_NaN())==-1);
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
 }
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"sim.cpp";exe=Path(d)/"sim"
            source.write_text(code)
            subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror","-I",str(SRC),str(source),"-o",str(exe)],capture_output=True,text=True,check=True)
            subprocess.run([str(exe)],check=True)
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

if __name__=="__main__":unittest.main()
