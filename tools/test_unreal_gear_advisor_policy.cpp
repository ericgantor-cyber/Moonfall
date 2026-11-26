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
 // Whole-loadout synergy, deterministic equal choices and combat-only filtering.
 const G::FEquipped Mixed={101005,211005,321005};
 G::FPlanner Planner(Mixed);
 Planner.Add(201005,0); Planner.Add(221005,1); Planner.Add(221005,2);
 Planner.Add(301004,3); Planner.Add(131005,4);
 const auto P=Planner.Finish();
 assert(P.Codes[0]==201005&&P.Codes[1]==211005&&P.Codes[2]==221005);
 assert(P.Indices[0]==0&&P.Indices[2]==1&&P.Changes==2&&P.Gain>0);
 G::FPlanner Combat(Mixed,true);
 Combat.Add(201005,0); Combat.Add(221006,1);
 const auto C=Combat.Finish();
 assert(C.Codes[0]==201005&&C.Codes[1]==Mixed[1]&&C.Codes[2]==Mixed[2]);
 G::FPlanner Stable(Gear);
 Stable.Add(Gear[0],0);Stable.Add(201005,1);Stable.Add(201004,2);
 assert(Stable.Finish().Changes==0);
 G::FPlanner New(Empty);
 New.Add(1,0);New.Add(10001,1);New.Add(20001,2);
 assert(New.Finish().Changes==3);
 namespace L=MoonfallEquipmentLayout;
 static_assert(L::SlotCount==9);
 constexpr auto Migrated=L::ExpandLegacy(G::FEquipped{101005,111005,121005});
 static_assert(Migrated[0]==101005&&Migrated[1]==111005&&Migrated[2]==121005);
 static_assert(Migrated[3]==0&&Migrated[8]==0&&L::ValidLayout(Migrated));
 assert(L::ExpandLegacy(G::FEquipped{111005,-1,121005})[0]==0);
 assert(L::ExpandLegacy(G::FEquipped{111005,-1,121005})[1]==0);
 L::FLoadout Expanded=Migrated;
 for(int Slot=3;Slot<L::SlotCount;++Slot) Expanded[Slot]=Slot*10000+1;
 assert(L::ValidLayout(Expanded));
 Expanded[8]=Expanded[7];
 assert(!L::ValidLayout(Expanded));
 assert(!L::Valid(std::numeric_limits<int>::max(),8));
 assert(!L::Valid(90001,9));
 assert(!L::Valid(-1,0));
 for(int Slot=0;Slot<L::SlotCount;++Slot)
  for(int Tier=1;Tier<=25;++Tier)
   for(int Rarity=0;Rarity<=5;++Rarity)
    for(int Affix=0;Affix<=3;++Affix) {
     const int Code=Affix*100000+Slot*10000+Rarity*1000+Tier;
     assert(L::Valid(Code,Slot)==(Rarity!=0||Affix==0));
     assert(!L::Valid(Code,(Slot+1)%L::SlotCount));
     if(Slot<3) assert(L::Valid(Code,Slot)==G::Valid(Code,Slot));
    }
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
