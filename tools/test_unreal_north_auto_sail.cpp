// UE-independent C++17 automatic Moonfall sea passage time rules.
#include <cassert>
#include <limits>
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthAutoSail.h"

using namespace MoonfallNorth;

int main() {
 assert(ValidAutoSailDuration(4.f));
 assert(ValidAutoSailDuration(11.f));
 assert(ValidAutoSailDuration(30.f));
 assert(!ValidAutoSailDuration(0.f));
 assert(!ValidAutoSailDuration(-1.f));
 assert(!ValidAutoSailDuration(3.999f));
 assert(!ValidAutoSailDuration(30.001f));
 const float NaN = std::numeric_limits<float>::quiet_NaN();
 assert(!ValidAutoSailDuration(NaN));
 assert(AutoSailProgress(NaN,11.f)==0.f);
 assert(AutoSailProgress(2.f,NaN)==0.f);
 assert(AutoSailProgress(-0.1f,11.f)==0.f);
 assert(AutoSailProgress(0.f,11.f)==0.f);
 assert(AutoSailProgress(5.5f,11.f)==0.5f);
 assert(AutoSailProgress(11.f,11.f)==1.f);
 assert(AutoSailProgress(200.f,11.f)==1.f);
 assert(AutoSailEasedProgress(2.75f,11.f)==0.15625f);
 assert(AutoSailEasedProgress(5.5f,11.f)==0.5f);
 assert(AutoSailEasedProgress(8.25f,11.f)==0.84375f);
 assert(!AutoSailShouldArrive(10.99f,11.f));
 assert(AutoSailShouldArrive(11.f,11.f));
 assert(AutoSailShouldArrive(20.f,11.f));
 assert(!AutoSailShouldArrive(NaN,11.f));
 assert(!AutoSailShouldArrive(11.f,NaN));
 assert(AutoSailPhaseAt(-1.f,11.f)==EAutoSailPhase::Invalid);
 assert(AutoSailPhaseAt(NaN,11.f)==EAutoSailPhase::Invalid);
 assert(AutoSailPhaseAt(0.f,NaN)==EAutoSailPhase::Invalid);
 assert(AutoSailPhaseAt(0.f,11.f)==EAutoSailPhase::Departing);
 assert(AutoSailPhaseAt(2.74f,11.f)==EAutoSailPhase::Departing);
 assert(AutoSailPhaseAt(2.75f,11.f)==EAutoSailPhase::OpenSea);
 assert(AutoSailPhaseAt(8.24f,11.f)==EAutoSailPhase::OpenSea);
 assert(AutoSailPhaseAt(8.25f,11.f)==EAutoSailPhase::Approaching);
 assert(AutoSailPhaseAt(10.99f,11.f)==EAutoSailPhase::Approaching);
 assert(AutoSailPhaseAt(11.f,11.f)==EAutoSailPhase::Arrived);
 assert(AutoSailSecondsRemaining(0.f,11.f)==11);
 assert(AutoSailSecondsRemaining(0.5f,11.f)==11);
 assert(AutoSailSecondsRemaining(1.f,11.f)==10);
 assert(AutoSailSecondsRemaining(5.5f,11.f)==6);
 assert(AutoSailSecondsRemaining(10.99f,11.f)==1);
 assert(AutoSailSecondsRemaining(11.f,11.f)==0);
 assert(AutoSailSecondsRemaining(NaN,11.f)==0);
 assert(AutoSailSecondsRemaining(2.f,NaN)==0);
 int LastRemaining=11;
 for (int Step=0; Step<=110; ++Step) {
  const float Elapsed=Step*0.1f;
  const int Remaining=AutoSailSecondsRemaining(Elapsed,11.f);
  assert(Remaining>=0 && Remaining<=LastRemaining);
  if (Elapsed<11.f) assert(Remaining>=1);
  LastRemaining=Remaining;
 }
 float Last=-1.f;
 for (int Step=0; Step<=220; ++Step) {
  const float Elapsed=Step*0.05f;
  const float Value=AutoSailEasedProgress(Elapsed,11.f);
  assert(Value>=0.f && Value<=1.f);
  assert(Value>=Last);
  Last=Value;
 }
}
