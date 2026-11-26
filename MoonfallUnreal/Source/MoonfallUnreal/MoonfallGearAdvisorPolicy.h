#pragma once
#include <array>
#include "MoonfallEquipmentSlots.h"
namespace MoonfallGearAdvisor {
using FEquipped = MoonfallEquipmentSlots::FLegacy;
using FExpandedEquipped = MoonfallEquipmentSlots::FLoadout;
// Retain the legacy predicate for companions, existing loot and HUD callers.
constexpr bool Valid(int Code,int Slot) {
 return MoonfallEquipmentSlots::Legacy(Slot)&&MoonfallEquipmentSlots::Valid(Code,Slot);
}
template<std::size_t N> constexpr bool ValidSlot(int Code,int Slot) {
 return Slot>=0&&Slot<static_cast<int>(N)&&MoonfallEquipmentSlots::Valid(Code,Slot);
}
constexpr int BaseScore(int Code) {
 return (Code%1000)*1000+((Code/1000)%10)*100+(Code/100000>0?10:0);
}
template<std::size_t N>
constexpr int Score(int Code,int Slot,const std::array<int,N>& Equipped) {
 if(!ValidSlot<N>(Code,Slot))return 0;
 const int Affix=Code/100000;
 int Matching=0;
 for(int Other=0;Other<static_cast<int>(N);++Other)
  if(Affix>0&&Other!=Slot&&ValidSlot<N>(Equipped[Other],Other)
     &&Equipped[Other]/100000==Affix)++Matching;
 return BaseScore(Code)+Matching*40;
}
template<std::size_t N>
constexpr int Improvement(int Candidate,const std::array<int,N>& Equipped) {
 if(Candidate<=0)return 0;
 const int Slot=(Candidate/10000)%10;
 if(!ValidSlot<N>(Candidate,Slot)||Candidate==Equipped[Slot])return 0;
 return Score(Candidate,Slot,Equipped)-Score(Equipped[Slot],Slot,Equipped);
}
constexpr int Rank(int Code){return (Code%1000)*10+(Code/1000)%10;}
struct FChoice {int Code=0;int Index=-1;};
template<std::size_t N> struct TPlan {
 std::array<int,N> Codes{};
 std::array<int,N> Indices{};
 int Gain=0;
 int Changes=0;
};
using FPlan=TPlan<3>;
template<std::size_t N>
constexpr int TotalScore(const std::array<int,N>& Codes) {
 int Total=0;
 for(int Slot=0;Slot<static_cast<int>(N);++Slot)Total+=Score(Codes[Slot],Slot,Codes);
 return Total;
}
template<std::size_t N> class TPlanner {
 static_assert(N>0&&N<=MoonfallEquipmentSlots::SlotCount);
 static constexpr int Slots=static_cast<int>(N),Radix=Slots+1,States=Radix*Radix*Radix;
 std::array<int,N> Worn{};
 std::array<std::array<FChoice,4>,N> Candidates{};
 bool WeaponsOnly=false;
 struct FState {int Value=-1;int Changes=0;unsigned Path=0;};
 static bool Better(const FState& A,const FState& B) {
  return A.Value>B.Value||(A.Value==B.Value&&(A.Changes<B.Changes
      ||(A.Changes==B.Changes&&A.Path<B.Path)));
 }
public:
 explicit TPlanner(const std::array<int,N>& Equipped,bool bWeaponsOnly=false):WeaponsOnly(bWeaponsOnly) {
  for(int Slot=0;Slot<Slots;++Slot){
   Worn[Slot]=ValidSlot<N>(Equipped[Slot],Slot)?Equipped[Slot]:0;
   Candidates[Slot][Worn[Slot]/100000]={Worn[Slot],-1};
  }
 }
 void Add(int Code,int Index) {
  if(Code<=0||Index<0)return;
  const int Slot=(Code/10000)%10;
  if(!ValidSlot<N>(Code,Slot)||(WeaponsOnly&&Slot!=0)||Rank(Code)<Rank(Worn[Slot]))return;
  auto& Choice=Candidates[Slot][Code/100000];
  if(Choice.Code==0||Rank(Code)>Rank(Choice.Code))Choice={Code,Index};
 }
 TPlan<N> Finish() const {
  // DP tracks counts of Force/Ward/Focus, not every affix permutation.
  // A matching pair contributes 40 to each piece, so adding the next
  // matching item contributes 80 times the previous matching count.
  // Nine slots: at most 9*1000*4 transitions, two fixed state buffers.
  std::array<FState,States> Current{},Next{};
  Current[0]={0,0,0};
  constexpr int Stride[4]={0,1,Radix,Radix*Radix};
  for(int Slot=0;Slot<Slots;++Slot){
   Next.fill(FState{});
   for(int State=0;State<States;++State){
    if(Current[State].Value<0)continue;
    for(int Affix=0;Affix<4;++Affix){
     const auto& Pick=Candidates[Slot][Affix];
     if(Pick.Code==0&&(Worn[Slot]!=0||Affix!=0))continue;
     const int Count=Affix==0?0:(State/Stride[Affix])%Radix;
     const int Target=State+Stride[Affix];
     const FState Candidate{Current[State].Value+BaseScore(Pick.Code)+80*Count,
       Current[State].Changes+(Pick.Code!=Worn[Slot]?1:0),
       (Current[State].Path<<2)|static_cast<unsigned>(Affix)};
     if(Better(Candidate,Next[Target]))Next[Target]=Candidate;
    }
   }
   Current.swap(Next);
  }
  FState Best{};
  for(const auto& State:Current)if(State.Value>=0&&Better(State,Best))Best=State;
  TPlan<N> Plan{};
  Plan.Codes=Worn;Plan.Indices.fill(-1);
  const int Baseline=TotalScore(Worn);
  if(Best.Value<=Baseline)return Plan;
  Plan.Gain=Best.Value-Baseline;Plan.Changes=Best.Changes;
  for(int Slot=0;Slot<Slots;++Slot){
   const unsigned Affix=(Best.Path>>(2*(Slots-1-Slot)))&3u;
   const auto& Pick=Candidates[Slot][Affix];
   Plan.Codes[Slot]=Pick.Code;Plan.Indices[Slot]=Pick.Index;
  }
  return Plan;
 }
};
using FPlanner=TPlanner<3>;
using FExpandedPlanner=TPlanner<MoonfallEquipmentSlots::SlotCount>;
}
