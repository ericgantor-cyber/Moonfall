// Portable C++17 regression: no Unreal installation required.
#include <cassert>
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthSeaPassage.h"
using namespace MoonfallNorth;

int main() {
 constexpr ESeaPort Ports[] = { ESeaPort::Mainland, ESeaPort::Shiverwake };
 for (const auto Start : Ports) {
  for (const auto Finish : Ports) {
   const bool Valid = Start != Finish;
   assert(ValidSeaRoute(Start,Finish) == Valid);
   for (unsigned Flags = 0; Flags < 32u; ++Flags) {
    const bool Enabled = (Flags & 1u) != 0u;
    const bool Paired = (Flags & 2u) != 0u;
    const bool Alive = (Flags & 4u) != 0u;
    const bool Menus = (Flags & 8u) != 0u;
    const bool Landing = (Flags & 16u) != 0u;
    assert(MayBoard(Start, Finish, Enabled, Paired, Alive, Menus, Landing)
           == (Valid && Enabled && Paired && Alive && !Menus && Landing));
   }
  }
 }
 assert(!MayBoard(static_cast<ESeaPort>(9), ESeaPort::Mainland,
                  true, true, true, false, true));
 assert(!MayBoard(ESeaPort::Mainland, static_cast<ESeaPort>(9),
                  true, true, true, false, true));
 assert(IsFirstIslandLanding(ESeaPort::Mainland,ESeaPort::Shiverwake,false));
 assert(!IsFirstIslandLanding(ESeaPort::Mainland,ESeaPort::Shiverwake,true));
 assert(!IsFirstIslandLanding(ESeaPort::Shiverwake,ESeaPort::Mainland,false));
 assert(!IsFirstIslandLanding(ESeaPort::Mainland,ESeaPort::Mainland,false));
 // Existing province topology remains untouched: landfall routes into
 // the same graph used by Morvella, Z and Nyra.
 assert(ShortestRoute(0,MorvellaProvince).Length > 1);
 assert(ShortestRoute(0,ZProvince).Length > 1);
 assert(ShortestRoute(0,NyraProvince).Length > 1);
}
