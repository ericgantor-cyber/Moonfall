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
 // Strict saved records must restore atomically; never erase invalid gear.
 L::FLoadout Saved{};
 const int LegacyCodes[]={101005,111005,121005};
 const auto OwnAll=[](int){return true;};
 assert(L::TryRestoreLoadout(LegacyCodes,3,false,OwnAll,Saved)&&Saved==Migrated);
 const L::FLoadout Full={101005,111005,121005,131005,141005,151005,161005,171005,181005};
 assert(L::TryRestoreLoadout(Full.data(),9,true,OwnAll,Saved)&&Saved==Full);
 assert(!L::TryRestoreLoadout(Full.data(),9,false,OwnAll,Saved)&&Saved==Full);
 assert(!L::TryRestoreLoadout(LegacyCodes,3,true,OwnAll,Saved)&&Saved==Full);
 assert(!L::TryRestoreLoadout(nullptr,9,true,OwnAll,Saved)&&Saved==Full);
 assert(!L::TryRestoreLoadout(Full.data(),10,true,OwnAll,Saved)&&Saved==Full);
 assert(!L::TryRestoreLoadout(Full.data(),9,true,[](int Code){return Code!=181005;},Saved)&&Saved==Full);
 auto WrongSlot=Full;WrongSlot[8]=171005;
 assert(!L::TryRestoreLoadout(WrongSlot.data(),9,true,OwnAll,Saved)&&Saved==Full);
 WrongSlot=Full;WrongSlot[8]=-1;
 assert(!L::TryRestoreLoadout(WrongSlot.data(),9,true,OwnAll,Saved)&&Saved==Full);
 assert(L::TryRestoreLoadout(LegacyCodes,3,false,OwnAll,Saved)&&Saved==Migrated);
 // Shared single-item comparison covers all nine slots without changing legacy weights.
 const auto LegacyExpanded=L::ExpandLegacy(Gear);
 for(int Slot=0;Slot<3;++Slot)
  for(int Affix=0;Affix<=3;++Affix)
   for(int Rarity=0;Rarity<=5;++Rarity)
    for(int Tier=1;Tier<=25;++Tier){
     const int Code=Affix*100000+Slot*10000+Rarity*1000+Tier;
     assert(L::Improvement(Code,LegacyExpanded)==G::Improvement(Code,Gear));
    }
 for(int Slot=0;Slot<L::SlotCount;++Slot){
  assert(L::Score(Full[Slot],Slot,Full)==5430);
  assert(L::Improvement(Full[Slot],Full)==0);
  assert(L::Improvement(Full[Slot]+1,Full)==1000);
  assert(L::Improvement(Full[Slot]+100000,Full)==-320);
  assert(L::Score(Full[Slot],(Slot+1)%L::SlotCount,Full)==0);
 }
 L::FLoadout EmptyNine{};
 assert(L::Improvement(80001,EmptyNine)==1000);
 assert(L::Improvement(90001,EmptyNine)==0);
 assert(L::Improvement(-1,EmptyNine)==0);
 assert(L::Improvement(std::numeric_limits<int>::max(),EmptyNine)==0);
 auto InvalidMatching=Full;InvalidMatching[7]=Full[6];
 assert(L::Score(Full[8],8,InvalidMatching)==5390);
 // The native advisor must ignore inactive slots and their set bonuses.
 for(int Slot=0;Slot<3;++Slot){
  const int Code=Full[Slot];
  assert(L::Score(Code,Slot,Full,L::LegacySlotCount)==G::Score(Code,Slot,Gear));
  assert(L::Improvement(Code+100000,Full,L::LegacySlotCount)==G::Improvement(Code+100000,Gear));
 }
 for(int Slot=3;Slot<L::SlotCount;++Slot){
  assert(L::Score(Full[Slot],Slot,Full,L::LegacySlotCount)==0);
  assert(L::Improvement(Full[Slot]+1,Full,L::LegacySlotCount)==0);
 }
 assert(L::Improvement(101006,Full,0)==0);
 assert(L::Improvement(101006,Full,-1)==0);
 assert(L::Improvement(101006,Full,4)==0);
 assert(L::Improvement(101006,Full,10)==0);
 assert(L::Improvement(181006,Full,L::SlotCount)==1000);
 assert(L::Improvement(181014,Full,L::SlotCount)==9000);
 // Cross-check exact optimization against all 4^9 combinations in tests only.
 L::FLoadout NineWorn{};
 int Choices[L::SlotCount][4]{};
 for(int Slot=0;Slot<L::SlotCount;++Slot){
  for(int Affix=0;Affix<4;++Affix)
   Choices[Slot][Affix]=Affix*100000+Slot*10000+1000+5+((Slot+Affix)%3==0?1:0);
  NineWorn[Slot]=Slot*10000+1005;
 }
 L::FPlanner Nine(NineWorn);
 for(int Slot=0;Slot<L::SlotCount;++Slot)
  for(int Affix=0;Affix<4;++Affix)Nine.Add(Choices[Slot][Affix],Slot*4+Affix);
 Nine.Add(181004,100);Nine.Add(191025,101);
 const auto NinePlan=Nine.Finish();
 int BestScore=-1, BestChanges=L::SlotCount+1;
 for(unsigned Path=0;Path<(1u<<(2*L::SlotCount));++Path){
  L::FLoadout Codes{};
  unsigned Bits=Path;
  int Changes=0;
  for(int Slot=L::SlotCount-1;Slot>=0;--Slot){
   Codes[Slot]=Choices[Slot][Bits&3u];Bits>>=2;
   if(Codes[Slot]!=NineWorn[Slot])++Changes;
  }
  const int Score=L::TotalScore(Codes);
  if(Score>BestScore||(Score==BestScore&&Changes<BestChanges)){
   BestScore=Score;BestChanges=Changes;
  }
 }
 assert(L::TotalScore(NinePlan.Codes)==BestScore);
 assert(NinePlan.Gain==BestScore-L::TotalScore(NineWorn)&&NinePlan.Changes==BestChanges);
 for(int Slot=0;Slot<L::SlotCount;++Slot){
  assert(L::Valid(NinePlan.Codes[Slot],Slot));
  if(NinePlan.Codes[Slot]!=NineWorn[Slot])
   assert(NinePlan.Indices[Slot]==Slot*4+NinePlan.Codes[Slot]/100000);
 }
 L::FPlanner Restricted(NineWorn,true);
 Restricted.Add(101006,0);Restricted.Add(181025,1);
 const auto RestrictedPlan=Restricted.Finish();
 assert(RestrictedPlan.Changes==1&&RestrictedPlan.Codes[0]==101006);
 for(int Slot=1;Slot<L::SlotCount;++Slot)assert(RestrictedPlan.Codes[Slot]==NineWorn[Slot]);
 L::FPlanner Blank(L::FLoadout{});
 Blank.Add(80001,9);Blank.Add(80001,10);
 const auto BlankPlan=Blank.Finish();
 assert(BlankPlan.Changes==1&&BlankPlan.Codes[8]==80001&&BlankPlan.Indices[8]==9);
 L::FPlanner Unchanged(NineWorn);
 Unchanged.Add(181004,0);
 assert(Unchanged.Finish().Changes==0);
 // Legacy mode must produce exactly the old planner's results before activation.
 unsigned Seed=731u;
 auto NextRandom=[&Seed](){Seed=Seed*1664525u+1013904223u;return Seed>>16;};
 for(int Trial=0;Trial<128;++Trial){
  G::FEquipped Old{};
  for(int Slot=0;Slot<3;++Slot)
   if(NextRandom()%2)Old[Slot]=Slot*10000+1000+1+static_cast<int>(NextRandom()%5);
  const bool CombatOnly=Trial%2!=0;
  G::FPlanner Legacy(Old,CombatOnly);
  L::FPlanner Shared(L::ExpandLegacy(Old),CombatOnly,L::LegacySlotCount);
  for(int I=0;I<36;++I){
   const int Slot=static_cast<int>(NextRandom()%3),Affix=static_cast<int>(NextRandom()%4);
   const int Code=Affix*100000+Slot*10000+1000+1+static_cast<int>(NextRandom()%7);
   Legacy.Add(Code,I);Shared.Add(Code,I);
  }
  Shared.Add(181025,99);
  const auto OldPlan=Legacy.Finish();
  const auto SharedPlan=Shared.Finish();
  assert(OldPlan.Gain==SharedPlan.Gain&&OldPlan.Changes==SharedPlan.Changes);
  for(int Slot=0;Slot<3;++Slot){
   assert(OldPlan.Codes[Slot]==SharedPlan.Codes[Slot]);
   assert(OldPlan.Indices[Slot]==SharedPlan.Indices[Slot]);
  }
  for(int Slot=3;Slot<L::SlotCount;++Slot)
   assert(SharedPlan.Codes[Slot]==0&&SharedPlan.Indices[Slot]==-1);
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
