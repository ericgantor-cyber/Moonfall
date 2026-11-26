#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallGearAdvisorPolicy.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallLootLuck.h"
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
 // Verify the nine-slot planner against a small exhaustive oracle.
 G::FExpandedEquipped Nine{};
 G::FExpandedPlanner Expanded(Nine);
 for(int Slot=0;Slot<9;++Slot){
  Nine[Slot]=100000+Slot*10000+1005;
  Expanded.Add(Nine[Slot],Slot);
  Expanded.Add(200000+Slot*10000+1005,9+Slot);
 }
 const auto Full=Expanded.Finish();
 assert(Full.Changes==9&&Full.Codes==Nine&&Full.Gain==G::TotalScore(Nine));
 G::FExpandedPlanner MixedNine(Nine);
 for(int Slot=0;Slot<9;++Slot)MixedNine.Add(200000+Slot*10000+1006,Slot);
 const auto Up=MixedNine.Finish();
 int Oracle=0;
 for(unsigned Mask=0;Mask<512;++Mask){
  auto Candidate=Nine;
  for(int Slot=0;Slot<9;++Slot)if(Mask&(1u<<Slot))Candidate[Slot]=200000+Slot*10000+1006;
  const int Gain=G::TotalScore(Candidate)-G::TotalScore(Nine);
  if(Gain>Oracle)Oracle=Gain;
 }
 assert(Up.Gain==Oracle&&Up.Changes==9);
 G::FExpandedPlanner CombatNine(Nine,true);
 CombatNine.Add(Nine[8]+1,8);CombatNine.Add(Nine[0]+1,0);
 const auto CombatPlan=CombatNine.Finish();
 assert(CombatPlan.Changes==1&&CombatPlan.Codes[8]==Nine[8]);
 assert(G::Improvement(Nine[8]+1,Nine)==1000);
 namespace L=MoonfallLootLuck;
 assert(L::Luck(G::FExpandedEquipped{})==0);
 G::FExpandedEquipped MaxLuckGear{};
 for(int Slot=0;Slot<9;++Slot)MaxLuckGear[Slot]=Slot*10000+5025;
 assert(L::Luck(MaxLuckGear)==40);
 MaxLuckGear.fill(999999);
 assert(L::Luck(MaxLuckGear)==0);
 for(int Level=1;Level<=100;++Level)for(int Rank=0;Rank<3;++Rank)
  for(int Roll=0;Roll<1000;++Roll){
   const bool Elite=Rank==1,Boss=Rank==2;
   const int Old=Roll/10,Bias=Level/5<12?Level/5:12;
   const int Expected=Boss?(Level>=60&&Old<12?5:Level>=30&&Old<40?4:3):
    Elite?(Level>=50&&Old<3?5:Level>=20&&Old<12?4:Old<27?3:2):
    (Level>=60&&Old<1?5:Level>=40&&Old<3?4:Old<3+Bias/3?3:Old<18+Bias?2:Old<53+Bias?1:0);
   const int Base=L::Rarity(Roll,Level,Elite,Boss,0);
   const int Boost=L::Rarity(Roll,Level,Elite,Boss,40);
   assert(Base==Expected&&Boost>=Base&&Boost<=5);
   assert(L::Rarity(Roll,Level,Elite,Boss,-1)==Base);
   assert(L::Rarity(Roll,Level,Elite,Boss,1000)==Boost);
  }
 assert(L::Rarity(999,100,false,false,40)==0);
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
