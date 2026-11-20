// UE-independent C++17 regression tests for ferry dry-land placement.
#include <cassert>
#include <limits>
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthDryLandingPolicy.h"

using namespace MoonfallNorth;

int main() {
 const float NaN = std::numeric_limits<float>::quiet_NaN();
 const float Inf = std::numeric_limits<float>::infinity();
 assert(ValidDryLandingSample(true,true,1.f,130.f,0.f,88.f));
 assert(ValidDryLandingSample(true,true,0.7f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(false,true,1.f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,false,1.f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,0.6999f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,-0.5f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.001f,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,0.f,0.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,0.f,500.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,0.f,-1.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,-1000.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,2000.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,NaN,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,NaN,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,NaN,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,0.f,NaN));
 assert(!ValidDryLandingSample(true,true,Inf,130.f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,Inf,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,Inf,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,130.f,0.f,Inf));
 // If capsule center is exactly 400cm above/below the marker, accept.
 assert(ValidDryLandingSample(true,true,1.f,491.f,0.f,88.f));
 assert(ValidDryLandingSample(true,true,1.f,-309.f,0.f,88.f));
 // Nearby out-of-range points must fail.
 assert(!ValidDryLandingSample(true,true,1.f,491.01f,0.f,88.f));
 assert(!ValidDryLandingSample(true,true,1.f,-309.01f,0.f,88.f));
 // The side of a collision object must be rejected even if it blocks Pawn.
 for (int Step=0; Step<70; ++Step) {
  float Slope=Step*0.01f;
  assert(!ValidDryLandingSample(true,true,Slope,130.f,0.f,88.f));
 }
}
