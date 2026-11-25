#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallGearAdvisorPolicy.h"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef NDEBUG
#error Tests require assertions
#endif
namespace G = MoonfallGearAdvisor;
int main() {
 const G::FEquipped Empty = {0,0,0};
 assert(G::Improvement(1, Empty) == 1000);
 assert(G::Improvement(0, Empty) == 0);
 assert(G::Improvement(-1, Empty) == 0);
 assert(G::Improvement(std::numeric_limits<int>::max(), Empty) == 0);
 const G::FEquipped Gear = {101005,111005,121005};
 assert(G::Improvement(Gear[0], Gear) == 0);
 assert(G::Improvement(101006, Gear) == 1000);
 assert(G::Improvement(201005, Gear) == -80);
 assert(G::Improvement(100005, Gear) == 0);
 assert(G::Improvement(301026, Gear) == 0);
 assert(G::Improvement(131005, Gear) == 0);
 const G::FEquipped Wrong = {111005,0,0};
 assert(G::Improvement(101005, Wrong) == 5110);
 int Cases = 0;
 for(int Slot=0;Slot<3;++Slot) for(int Tier=1;Tier<=25;++Tier)
  for(int Rarity=0;Rarity<=5;++Rarity) for(int Affix=0;Affix<=3;++Affix) {
   const int Code=Affix*100000+Slot*10000+Rarity*1000+Tier;
   const bool Valid=Rarity!=0||Affix==0;
   assert(G::Valid(Code,Slot)==Valid);
   assert(G::Improvement(Code,Empty)==(Valid?Tier*1000+Rarity*100+(Affix>0?10:0):0));
   ++Cases;
  }
 std::cout << "Gear advisor: " << Cases << " combinations plus boundary and synergy tests passed\n";
}
