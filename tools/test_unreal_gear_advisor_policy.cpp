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
 // The native migration boundary must accept old mirrors, preserve nine
 // enabled slots, and reject any mismatch without modifying the output.
 L::FLoadout MirrorResult=Full;
 assert(L::TryRestoreMirroredLoadout(LegacyCodes,3,nullptr,0,false,3,OwnAll,MirrorResult));
 assert(MirrorResult==Migrated);
 assert(L::TryRestoreMirroredLoadout(LegacyCodes,3,nullptr,0,false,9,OwnAll,MirrorResult));
 assert(MirrorResult==Migrated);
 assert(L::TryRestoreMirroredLoadout(Full.data(),3,Full.data(),9,true,9,OwnAll,MirrorResult));
 assert(MirrorResult==Full);
 assert(!L::TryRestoreMirroredLoadout(Full.data(),3,Full.data(),9,true,3,OwnAll,MirrorResult));
 assert(MirrorResult==Full);
 assert(!L::TryRestoreMirroredLoadout(Full.data(),3,Full.data(),8,true,9,OwnAll,MirrorResult));
 assert(!L::TryRestoreMirroredLoadout(Full.data(),3,Full.data(),9,true,4,OwnAll,MirrorResult));
 assert(!L::TryRestoreMirroredLoadout(Full.data(),3,Full.data(),9,true,9,
     [&](int Code){return Code!=Full[8];},MirrorResult));
 assert(MirrorResult==Full);
 auto WrongMirror=Full;WrongMirror[0]+=1;
 assert(!L::TryRestoreMirroredLoadout(Full.data(),3,WrongMirror.data(),9,true,9,OwnAll,MirrorResult));
 assert(MirrorResult==Full);
 assert(L::TryRestoreMirroredLoadout(LegacyCodes,3,Migrated.data(),9,true,3,OwnAll,MirrorResult));
 assert(MirrorResult==Migrated);
 // V19 codec preserves old banks while adding eight ring slots.
 L::FFullLoadout Full24{};
 int ValidFullCases=0;
 int RingCount=0;
 for(int Slot=0;Slot<L::FullSlotCount;++Slot){
  Full24[Slot]=L::EncodeFullItem(Slot,1,1,5);
  if(L::IsRingSlot(Slot)){
   ++RingCount;
   assert(L::RingSlot(L::RingIndex(Slot))==Slot);
  }else assert(L::RingIndex(Slot)==-1);
  for(int Trait=0;Trait<=3;++Trait)
   for(int Rarity=0;Rarity<=5;++Rarity)
    for(int Tier=1;Tier<=25;++Tier){
     const int Code=L::EncodeFullItem(Slot,Trait,Rarity,Tier);
     if(Trait!=0&&Rarity==0){assert(Code==0);continue;}
     ++ValidFullCases;
     const auto Decoded=L::DecodeFullItem(Code);
     assert(Decoded.Slot==Slot&&Decoded.Trait==Trait&&Decoded.Rarity==Rarity&&Decoded.Tier==Tier);
     assert(L::ValidFullItem(Code,Slot));
     assert(!L::ValidFullItem(Code,(Slot+1)%L::FullSlotCount));
     const int Payload=Trait*100000+Rarity*1000+Tier;
     if(Slot<9)assert(Code==Slot*10000+Payload);
     else if(Slot<16)assert(Code==1000000+(Slot-9)*10000+Payload);
     else assert(Code==2000000+(Slot-16)*10000+Payload);
    }
 }
 assert(ValidFullCases==12600);
 assert(RingCount==10&&L::RingSlot(-1)==-1&&L::RingSlot(10)==-1);
 assert(L::RingIndex(-1)==-1&&L::RingIndex(24)==-1);
 for(int Code:{-1,0,90001,100001,1070001,1090001,1400001,2000000,
      2080001,2090001,2400001,3000001,
      std::numeric_limits<int>::max(),std::numeric_limits<int>::min()})
  assert(!L::DecodeFullItem(Code).IsValid());
 assert(L::EncodeFullItem(-1,0,0,1)==0&&L::EncodeFullItem(24,0,0,1)==0);
 assert(L::EncodeFullItem(23,4,1,1)==0&&L::EncodeFullItem(23,0,6,1)==0);
 assert(L::EncodeFullItem(23,0,0,26)==0);
 L::FFullLoadout FullRestore{};
 assert(L::TryRestoreFullLoadout(Full24.data(),3,nullptr,0,16,OwnAll,FullRestore));
 for(int Slot=0;Slot<24;++Slot)assert(FullRestore[Slot]==(Slot<3?Full24[Slot]:0));
 assert(L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),9,17,OwnAll,FullRestore));
 for(int Slot=0;Slot<24;++Slot)assert(FullRestore[Slot]==(Slot<9?Full24[Slot]:0));
 assert(L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),16,18,OwnAll,FullRestore));
 for(int Slot=0;Slot<24;++Slot)assert(FullRestore[Slot]==(Slot<16?Full24[Slot]:0));
 assert(L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),24,19,OwnAll,FullRestore));
 assert(FullRestore==Full24);
 auto BadFull=Full24;BadFull[23]=Full24[22];
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,BadFull.data(),24,19,OwnAll,FullRestore));
 BadFull=Full24;BadFull[0]+=1;
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,BadFull.data(),24,19,OwnAll,FullRestore));
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),24,19,
     [&](int Code){return Code!=Full24[23];},FullRestore));
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),9,18,OwnAll,FullRestore));
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),16,17,OwnAll,FullRestore));
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),16,19,OwnAll,FullRestore));
 assert(!L::TryRestoreFullLoadout(Full24.data(),3,Full24.data(),24,20,OwnAll,FullRestore));
 assert(FullRestore==Full24);
 // Full-layout crafting operations must preserve slot banks and reject
 // malformed/inactive gear. All valid slots are exercised across rarity caps.
 for(int Slot=0;Slot<24;++Slot)
 for(int Rarity=0;Rarity<=5;++Rarity){
  const int Code=L::EncodeFullItem(Slot,0,Rarity,24);
  const int Up=L::FullUpgradeCode(Code,24),Asc=L::FullAscendCode(Code,24);
  const int Reforge=L::FullReforgeCode(Code,2,24);
  assert(L::DecodeFullItem(Up).Slot==Slot&&L::DecodeFullItem(Up).Tier==25);
  assert(L::FullUpgradeCode(Up,24)==0);
  assert(Asc==(Rarity<5?L::EncodeFullItem(Slot,0,Rarity+1,24):0));
  assert(Reforge==(Rarity>=2?L::EncodeFullItem(Slot,2,Rarity,24):0));
  assert(L::FullReforgeCode(Reforge,2,24)==0);
  assert(L::FullUpgradeCode(Code,9)==(Slot<9?Up:0));
 }
 const int Ring10=L::EncodeFullItem(23,1,5,10);
 L::FFullLoadout RingWorn{};RingWorn[23]=Ring10;
 assert(!L::CanRemoveFullCopy(Ring10,RingWorn,24,std::array<int,1>{Ring10}));
 assert(L::CanRemoveFullCopy(Ring10,RingWorn,24,std::array<int,2>{Ring10,Ring10}));
 assert(!L::CanRemoveFullCopy(Ring10,RingWorn,16,std::array<int,2>{Ring10,Ring10}));
 assert(L::FullUpgradeCode(Ring10,23)==0);
 // Ring compatibility, explicit assignment, physical copy limits, swaps
 // and V19 restoration: the same code may occupy two fingers only when two
 // matching inventory copies actually exist.
 const int CommonRing=L::EncodeFullItem(8,0,0,1);
 const int HighRing=L::EncodeFullItem(13,2,5,25);
 const std::array<int,3> RingBag{{CommonRing,CommonRing,HighRing}};
 L::FFullLoadout Flexible{},RingResult{};
 for(int I=0;I<10;++I){
  const int Target=L::RingSlot(I);
  assert(L::CanWearFullItem(CommonRing,Target,24));
  assert(L::CanWearFullItem(Ring10,Target,24));
  assert(!L::CanWearFullItem(1,Target,24));
 }
 assert(!L::CanWearFullItem(CommonRing,0,24));
 assert(!L::CanWearFullItem(Ring10,8,9));
 assert(!L::TryAssignFullRing(CommonRing,23,Flexible,24,true,RingBag,RingResult));
 assert(L::TryAssignFullRing(CommonRing,23,Flexible,24,false,RingBag,RingResult));
 Flexible=RingResult;
 assert(L::TryAssignFullRing(CommonRing,16,Flexible,24,false,RingBag,RingResult));
 Flexible=RingResult;
 assert(L::FullWornCopies(CommonRing,Flexible,24)==2);
 assert(!L::TryAssignFullRing(CommonRing,17,Flexible,24,false,RingBag,RingResult));
 assert(RingResult==Flexible);
 assert(!L::CanRemoveFullCopy(CommonRing,Flexible,24,RingBag));
 assert(L::CanRemoveFullCopy(CommonRing,Flexible,24,std::array<int,3>{CommonRing,CommonRing,CommonRing}));
 assert(L::TryAssignFullRing(HighRing,17,Flexible,24,false,RingBag,RingResult));
 Flexible=RingResult;
 assert(!L::TryMoveFullRing(17,0,Flexible,24,false,RingBag,RingResult));
 assert(!L::TryMoveFullRing(17,23,Flexible,24,true,RingBag,RingResult));
 assert(L::TryMoveFullRing(17,23,Flexible,24,false,RingBag,RingResult));
 assert(RingResult[23]==HighRing&&RingResult[17]==CommonRing);
 Flexible=RingResult;
 assert(L::TryRestoreFlexibleFullLoadout(Flexible.data(),3,Flexible.data(),24,19,RingBag,RingResult));
 assert(RingResult==Flexible);
 assert(!L::TryRestoreFlexibleFullLoadout(Flexible.data(),3,Flexible.data(),24,19,
      std::array<int,2>{CommonRing,HighRing},RingResult));
 assert(RingResult==Flexible);
 auto BadRings=Flexible;BadRings[0]=CommonRing;
 assert(!L::TryRestoreFlexibleFullLoadout(BadRings.data(),3,BadRings.data(),24,19,RingBag,RingResult));
 assert(L::FullTotalScore(Flexible)>0);
 L::FFullPlanner PreserveFlexible(Flexible);
 PreserveFlexible.Add(L::EncodeFullItem(23,3,5,25),0);
 assert(PreserveFlexible.Finish().Codes==Flexible&&PreserveFlexible.Finish().Changes==0);
 // Worst case: all 24 matching traits, with all four candidate buckets.
 // High counts exceed the old decimal DP; first and last slots exercise
 // the full 48-bit decision path. Equal items retain the earliest index.
 L::FFullPlanner AllSlots(L::FFullLoadout{});
 L::FFullLoadout Expected24{};
 for(int Slot=0;Slot<24;++Slot){
  Expected24[Slot]=L::EncodeFullItem(Slot,3,5,25);
  for(int Trait=0;Trait<4;++Trait)
   AllSlots.Add(L::EncodeFullItem(Slot,Trait,5,25),Slot*4+Trait);
  AllSlots.Add(Expected24[Slot],100+Slot);
 }
 const auto AllPlan=AllSlots.Finish();
 assert(AllPlan.Changes==24&&AllPlan.Gain==L::FullTotalScore(AllPlan.Codes));
 // All same-affix plans have equal score: deterministic path picks trait 1.
 for(int Slot=0;Slot<24;++Slot){
  assert(AllPlan.Codes[Slot]==L::EncodeFullItem(Slot,1,5,25));
  assert(AllPlan.Indices[Slot]==Slot*4+1);
 }
 L::FFullPlanner Locked(AllPlan.Codes,true);
 Locked.Add(L::EncodeFullItem(23,3,5,25),0);
 assert(Locked.Finish().Changes==0);
 L::FFullPlanner InvalidCount(L::FFullLoadout{},false,23);
 InvalidCount.Add(Ring10,0);
 assert(InvalidCount.Finish().Changes==0);
 // Independent brute-force reference over seven varied slots, including
 // bank boundaries and the final ring. Other slots remain fixed.
 const std::array<int,7> Varied{{0,8,9,13,15,16,23}};
 const auto IndependentTotal=[](const L::FFullLoadout& Gear){
  int Score=0;
  for(int S=0;S<24;++S){
   const auto D=L::DecodeFullItem(Gear[S]);
   if(D.Slot!=S)continue;
   Score+=D.Tier*1000+D.Rarity*100+(D.Trait?10:0);
   for(int Other=S+1;Other<24;++Other){
    const auto E=L::DecodeFullItem(Gear[Other]);
    if(D.Trait&&E.Slot==Other&&E.Trait==D.Trait)Score+=80;
   }
  }
  return Score;
 };
 for(int Trial=0;Trial<12;++Trial){
  L::FFullLoadout Worn{};
  for(int S=0;S<24;++S)Worn[S]=L::EncodeFullItem(S,(S+Trial)%4,2,5);
  L::FFullPlanner Planner24(Worn);
  std::array<std::array<int,3>,7> Choices{};
  for(int I=0;I<7;++I){
   const int S=Varied[I];
   Choices[I]={Worn[S],L::EncodeFullItem(S,(S+Trial+1)%4,2,5),
                       L::EncodeFullItem(S,(S+Trial+2)%4,2,5+Trial%2)};
   Planner24.Add(Choices[I][1],I*2);Planner24.Add(Choices[I][2],I*2+1);
  }
  const int Baseline=IndependentTotal(Worn);
  int Best=Baseline,Fewest=0;
  for(int N=0;N<2187;++N){
   auto Gear=Worn;int Digits=N,Changes=0;
   for(int I=0;I<7;++I){Gear[Varied[I]]=Choices[I][Digits%3];Digits/=3;
    Changes+=Gear[Varied[I]]!=Worn[Varied[I]];}
   const int Score=IndependentTotal(Gear);
   if(Score>Best||(Score==Best&&Changes<Fewest)){Best=Score;Fewest=Changes;}
  }
  const auto Plan24=Planner24.Finish();
  assert(IndependentTotal(Plan24.Codes)==Best&&Plan24.Gain==Best-Baseline&&Plan24.Changes==Fewest);
 }
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
 // Exhaust all valid encoding combinations for every inventory operation.
 using Op=L::EInventoryOperation;
 for(int Slot=0;Slot<L::SlotCount;++Slot)
  for(int Tier=1;Tier<=25;++Tier)
   for(int Rarity=0;Rarity<=5;++Rarity)
    for(int Affix=0;Affix<=3;++Affix){
     const int Code=Affix*100000+Slot*10000+Rarity*1000+Tier;
     for(int Active:{L::LegacySlotCount,L::SlotCount}){
      const bool Valid=Slot<Active&&(Rarity!=0||Affix==0);
      assert(L::CanOperate(Code,Active,Op::Equip)==Valid);
      assert(L::CanOperate(Code,Active,Op::Salvage)==Valid);
      assert(L::CanOperate(Code,Active,Op::Upgrade)==(Valid&&Tier<25));
      assert(L::CanOperate(Code,Active,Op::Reforge)==(Valid&&Rarity>=2));
      assert(L::CanOperate(Code,Active,Op::Ascend)==(Valid&&Rarity<5));
     }
    }
 for(int Code:{0,-1,90001,106001,100026,std::numeric_limits<int>::max()})
  for(Op Action:{Op::Equip,Op::Salvage,Op::Upgrade,Op::Reforge,Op::Ascend})
   assert(!L::CanOperate(Code,L::SlotCount,Action));
 assert(!L::CanOperate(101005,4,Op::Equip));
 // A last worn copy is never removable, including all six future slots.
 for(int Slot=0;Slot<L::SlotCount;++Slot){
  const int Code=Full[Slot];
  const std::array<int,1> Single={Code};
  const std::array<int,3> Duplicate={Code,1,Code};
  const std::array<int,2> Unrelated={1,2};
  assert(!L::CanRemoveSelectedCopy(Code,Full,L::SlotCount,Single));
  assert(!L::CanRemoveSelectedCopy(Code,Full,L::SlotCount,Unrelated));
  assert(L::CanRemoveSelectedCopy(Code,Full,L::SlotCount,Duplicate));
  assert(L::CanRemoveSelectedCopy(Code,L::FLoadout{},L::SlotCount,Single));
  assert(L::CanRemoveSelectedCopy(Code,Full,L::LegacySlotCount,Duplicate)==(Slot<L::LegacySlotCount));
 }
 auto WrongWorn=Full;WrongWorn[0]=111005;WrongWorn[1]=0;
 assert(!L::CanRemoveSelectedCopy(111005,WrongWorn,L::SlotCount,std::array<int,1>{111005}));
 assert(!L::CanRemoveSelectedCopy(0,Full,L::SlotCount,std::array<int,2>{0,0}));
 assert(!L::CanRemoveSelectedCopy(101005,Full,4,std::array<int,2>{101005,101005}));
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
 // Fixed seed, independent reference decoder and score. No engine required.
 // Concentrate cases around valid encodings as well as corrupt integer values.
 unsigned FuzzSeed=0x4d4f4f4eu;
 auto NextFuzz=[&FuzzSeed](){FuzzSeed=FuzzSeed*1664525u+1013904223u;return FuzzSeed;};
 const auto RefValid=[](int Code,int Slot){
  if(Code<=0||Slot<0||Slot>=9)return false;
  const int Affix=Code/100000,EncodedSlot=(Code/10000)%10;
  const int Rarity=(Code/1000)%10,Tier=Code%1000;
  return Affix<=3&&EncodedSlot==Slot&&Rarity<=5&&Tier>=1&&Tier<=25
      &&(Affix==0||Rarity>0);
 };
 const auto RefScore=[&RefValid](int Code,int Slot,const L::FLoadout& Worn,int Active){
  if(Slot<0||Slot>=Active||!RefValid(Code,Slot))return 0;
  const int Affix=Code/100000;
  int Matching=0;
  for(int Other=0;Other<Active;++Other)
   if(Other!=Slot&&Affix>0&&RefValid(Worn[Other],Other)&&Worn[Other]/100000==Affix)
    ++Matching;
  return (Code%1000)*1000+((Code/1000)%10)*100+(Affix>0?10:0)+40*Matching;
 };
 auto ValidRandom=[&NextFuzz](int Slot){
  const int Rarity=1+static_cast<int>(NextFuzz()%5u);
  const int Affix=static_cast<int>((NextFuzz()>>12)%4u);
  const int Tier=1+static_cast<int>((NextFuzz()>>8)%25u);
  return Affix*100000+Slot*10000+Rarity*1000+Tier;
 };
 for(int Trial=0;Trial<4096;++Trial){
  const int Slot=static_cast<int>((NextFuzz()>>8)%9u);
  const int Active=Trial%2==0?3:9;
  int Candidate=ValidRandom(Slot);
  switch(Trial%8){
   case 0:Candidate=std::numeric_limits<int>::min();break;
   case 1:Candidate=std::numeric_limits<int>::max();break;
   case 2:Candidate=static_cast<int>(NextFuzz()%1000000u)-500000;break;
   case 3:Candidate=0;break;
   case 4:Candidate+=static_cast<int>(NextFuzz()%2000u)-1000;break;
   default:break;
  }
  L::FLoadout Worn{};
  for(int S=0;S<9;++S)if((NextFuzz()>>8)%3u)Worn[S]=ValidRandom(S);
  const int EncodedSlot=Candidate>0?(Candidate/10000)%10:-1;
  const bool Eligible=EncodedSlot>=0&&EncodedSlot<Active&&RefValid(Candidate,EncodedSlot);
  const int Expected=Eligible?RefScore(Candidate,EncodedSlot,Worn,Active)
     -RefScore(Worn[EncodedSlot],EncodedSlot,Worn,Active):0;
  assert(L::Improvement(Candidate,Worn,Active)==Expected);
  assert(L::CanOperate(Candidate,Active,Op::Equip)==Eligible);
  assert(L::CanOperate(Candidate,Active,Op::Salvage)==Eligible);
  assert(L::CanOperate(Candidate,Active,Op::Upgrade)==(Eligible&&Candidate%1000<25));
  assert(L::CanOperate(Candidate,Active,Op::Reforge)==(Eligible&&(Candidate/1000)%10>=2));
  assert(L::CanOperate(Candidate,Active,Op::Ascend)==(Eligible&&(Candidate/1000)%10<5));
  // A malformed or unowned slot must never partially replace the output.
  const int Changed=static_cast<int>((NextFuzz()>>8)%static_cast<unsigned>(Active));
  auto Record=Worn;
  Record[Changed]=Candidate;
  const bool RejectOwnership=Trial%5==0;
  const auto Owns=[Candidate,RejectOwnership](int Code){return !RejectOwnership||Code!=Candidate;};
  bool ExpectedRestore=true;
  L::FLoadout RestoredExpected{};
  for(int S=0;S<Active;++S){
   const int Code=Record[S];
   if(Code!=0&&(!RefValid(Code,S)||!Owns(Code)))ExpectedRestore=false;
   RestoredExpected[S]=Code;
  }
  L::FLoadout Output=Full;
  assert(L::TryRestoreLoadout(Record.data(),Active,Active==9,Owns,Output)==ExpectedRestore);
  assert(Output==(ExpectedRestore?RestoredExpected:Full));
 }
 // Differential planner tests: brute force only in CI, not in game runtime.
 const auto RefTotal=[&RefScore](const L::FLoadout& Worn){
  int Score=0;
  for(int S=0;S<9;++S)Score+=RefScore(Worn[S],S,Worn,9);
  return Score;
 };
 for(int Trial=0;Trial<24;++Trial){
  L::FLoadout Worn{};
  std::array<std::array<int,3>,9> Options{};
  for(int S=0;S<9;++S){
   Worn[S]=Trial%4==0?0:ValidRandom(S);
   Options[S]={Worn[S],ValidRandom(S),ValidRandom(S)};
  }
  const bool CombatOnly=Trial%3==0;
  L::FPlanner PlannerUnderTest(Worn,CombatOnly);
  for(int S=0;S<9;++S)
   for(int OptionIndex=1;OptionIndex<3;++OptionIndex)
    PlannerUnderTest.Add(Options[S][OptionIndex],S*3+OptionIndex);
  const int Baseline=RefTotal(Worn);
  int Best=Baseline,Fewest=0;
  for(int Combination=0;Combination<19683;++Combination){
   int Digits=Combination,Changes=0;
   bool Allowed=true;
   L::FLoadout Candidate{};
   for(int S=0;S<9;++S){
    const int Choice=Digits%3;Digits/=3;
    const int Code=Options[S][Choice];
    const int Rank=(Code%1000)*10+(Code/1000)%10;
    const int PriorRank=(Worn[S]%1000)*10+(Worn[S]/1000)%10;
    if(Rank<PriorRank||(CombatOnly&&S!=0&&Choice!=0)){Allowed=false;break;}
    Candidate[S]=Code;
    if(Code!=Worn[S])++Changes;
   }
   if(!Allowed)continue;
   const int Score=RefTotal(Candidate);
   if(Score>Best||(Score==Best&&Changes<Fewest)){Best=Score;Fewest=Changes;}
  }
  const auto Result=PlannerUnderTest.Finish();
  assert(RefTotal(Result.Codes)==Best);
  assert(Result.Gain==Best-Baseline&&Result.Changes==Fewest);
  for(int S=0;S<9;++S){
   if(Result.Codes[S]==Worn[S])continue;
   const int Index=Result.Indices[S];
   assert(Index==S*3+1||Index==S*3+2);
   assert(Result.Codes[S]==Options[S][Index%3]);
  }
 }
 std::cout << "Deterministic stress: 4096 corrupt/valid item and save cases; 24 independent exhaustive planner comparisons passed\n";
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
