#pragma once
#include <array>
namespace MoonfallEquipmentScoring {
// Both legacy and expanded comparisons use identical rank and set weights.
// Callers supply only validated owned equipped codes (or zero).
template<class Loadout,class Validator>
constexpr int Score(int Code,int Slot,const Loadout& Equipped,Validator Valid,int ActiveSlots) {
 if(ActiveSlots<0||ActiveSlots>static_cast<int>(Equipped.size())
     ||Slot<0||Slot>=ActiveSlots||!Valid(Code,Slot))return 0;
 const int Affix=Code/100000;
 int Matching=0;
 for(int Other=0;Other<ActiveSlots;++Other)
  if(Affix>0&&Other!=Slot&&Valid(Equipped[Other],Other)
      &&Equipped[Other]/100000==Affix)++Matching;
 return (Code%1000)*1000+((Code/1000)%10)*100+(Affix>0?10:0)+Matching*40;
}
template<class Loadout,class Validator>
constexpr int Improvement(int Candidate,const Loadout& Equipped,Validator Valid,int ActiveSlots) {
 if(Candidate<=0||ActiveSlots<0||ActiveSlots>static_cast<int>(Equipped.size()))return 0;
 const int Slot=(Candidate/10000)%10;
 if(Slot<0||Slot>=ActiveSlots||!Valid(Candidate,Slot)
     ||Candidate==Equipped[Slot])return 0;
 return Score(Candidate,Slot,Equipped,Valid,ActiveSlots)-Score(Equipped[Slot],Slot,Equipped,Valid,ActiveSlots);
}
}
namespace MoonfallGearAdvisor {
using FEquipped = std::array<int, 3>;
// Identical encoding and scoring weights to the original advisor. Worn
// ownership is checked once at the Unreal boundary, not per candidate.
constexpr bool Valid(int Code, int Slot) {
 if (Code <= 0 || Slot < 0 || Slot > 2) return false;
 const int Affix = Code / 100000, EncodedSlot = (Code / 10000) % 10;
 const int Rarity = (Code / 1000) % 10, Tier = Code % 1000;
 return EncodedSlot == Slot && Affix >= 0 && Affix <= 3 && Rarity >= 0
     && Rarity <= 5 && Tier >= 1 && Tier <= 25 && (Rarity != 0 || Affix == 0)
     && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
}
constexpr int Score(int Code, int Slot, const FEquipped& Equipped) {
 return MoonfallEquipmentScoring::Score(Code,Slot,Equipped,Valid,3);
}
constexpr int Improvement(int Candidate, const FEquipped& Equipped) {
 return MoonfallEquipmentScoring::Improvement(Candidate,Equipped,Valid,3);
}

 // Keep tier and rarity progression primary; affix changes may improve sets.
constexpr int Rank(int Code) { return (Code % 1000) * 10 + (Code / 1000) % 10; }
struct FChoice { int Code=0; int Index=-1; };
struct FPlan {
 FEquipped Codes{};
 std::array<int,3> Indices{{-1,-1,-1}};
 int Gain=0;
 int Changes=0;
};
constexpr int TotalScore(const FEquipped& Codes) {
 return Score(Codes[0],0,Codes)+Score(Codes[1],1,Codes)+Score(Codes[2],2,Codes);
}
class FPlanner {
 FEquipped Worn{};
 std::array<std::array<FChoice,4>,3> Candidates{};
 bool WeaponsOnly=false;
public:
 explicit FPlanner(const FEquipped& Equipped, bool bWeaponsOnly=false):WeaponsOnly(bWeaponsOnly) {
  for(int Slot=0;Slot<3;++Slot) {
   Worn[Slot]=Valid(Equipped[Slot],Slot)?Equipped[Slot]:0;
   Candidates[Slot][Worn[Slot]/100000]={Worn[Slot],-1};
  }
 }
 void Add(int Code,int Index) {
  if(Code<=0||Index<0)return;
  const int Slot=(Code/10000)%10;
  if(!Valid(Code,Slot)||(WeaponsOnly&&Slot!=0)||Rank(Code)<Rank(Worn[Slot]))return;
  auto& Choice=Candidates[Slot][Code/100000];
  if(Choice.Code==0||Rank(Code)>Rank(Choice.Code))Choice={Code,Index};
 }
 FPlan Finish() const {
  FPlan Best{Worn,{{-1,-1,-1}},0,0};
  const int Baseline=TotalScore(Worn);
  // At most 4^3=64 combinations: each affix bucket retains its best owned item.
  for(int A=0;A<4;++A)for(int B=0;B<4;++B)for(int C=0;C<4;++C){
   const FChoice Picks[3]={Candidates[0][A],Candidates[1][B],Candidates[2][C]};
   FPlan P;
   bool ValidPlan=true;
   for(int Slot=0;Slot<3;++Slot){
    const auto& Pick=Picks[Slot];
    if(Pick.Code==0&&Worn[Slot]!=0){ValidPlan=false;break;}
    P.Codes[Slot]=Pick.Code;
    P.Indices[Slot]=Pick.Index;
    if(Pick.Code!=Worn[Slot])++P.Changes;
   }
   if(!ValidPlan)continue;
   P.Gain=TotalScore(P.Codes)-Baseline;
   if(P.Gain>Best.Gain||(P.Gain==Best.Gain&&P.Changes<Best.Changes))Best=P;
  }
  return Best;
 }
};
}

// Nine-slot migration foundation. Not yet wired into the native inventory or save.
// Preserve the legacy weapon/armor/accessory digits exactly; reserve six new IDs.
namespace MoonfallEquipmentLayout {
enum class ESlot : int { Weapon=0, Chest=1, Trinket=2, Head=3, Shoulders=4, Hands=5, Legs=6, Feet=7, Ring=8 };
inline constexpr int SlotCount=9;
inline constexpr int LegacySlotCount=3;
using FLoadout=std::array<int,SlotCount>;
constexpr bool Valid(int Code,int Slot) {
 if(Code<=0 || Slot<0 || Slot>=SlotCount || (Code/10000)%10!=Slot) return false;
 // Normalize the slot digit only, then reuse the existing tier/rarity/affix validator.
 return MoonfallGearAdvisor::Valid(Code-Slot*10000,0);
}
// Common validation for explicit inventory operations. Reject inactive slots
// before crafting or equipping; operation-specific limits remain independent.
enum class EInventoryOperation { Equip, Salvage, Upgrade, Reforge, Ascend };
constexpr bool CanOperate(int Code,int ActiveSlots,EInventoryOperation Operation) {
 if(ActiveSlots!=LegacySlotCount&&ActiveSlots!=SlotCount)return false;
 if(Code<=0)return false;
 const int Slot=(Code/10000)%10;
 if(Slot>=ActiveSlots||!Valid(Code,Slot))return false;
 const int Tier=Code%1000,Rarity=(Code/1000)%10;
 switch(Operation){
  case EInventoryOperation::Equip:
  case EInventoryOperation::Salvage:return true;
  case EInventoryOperation::Upgrade:return Tier<25;
  case EInventoryOperation::Reforge:return Rarity>=2;
  case EInventoryOperation::Ascend:return Rarity<5;
 }
 return false;
}
// Individual item comparisons preserve the legacy per-piece score semantics.
// Whole-loadout optimization uses TotalScore below, including both sides of a set pair.
constexpr int Score(int Code,int Slot,const FLoadout& Equipped,int ActiveSlots=SlotCount) {
 if(ActiveSlots!=LegacySlotCount&&ActiveSlots!=SlotCount)return 0;
 return MoonfallEquipmentScoring::Score(Code,Slot,Equipped,Valid,ActiveSlots);
}
constexpr int Improvement(int Candidate,const FLoadout& Equipped,int ActiveSlots=SlotCount) {
 if(ActiveSlots!=LegacySlotCount&&ActiveSlots!=SlotCount)return 0;
 return MoonfallEquipmentScoring::Improvement(Candidate,Equipped,Valid,ActiveSlots);
}
constexpr FLoadout ExpandLegacy(const MoonfallGearAdvisor::FEquipped& Old) {
 FLoadout Out{};
 for(int Slot=0;Slot<LegacySlotCount;++Slot)
  if(Valid(Old[Slot],Slot)) Out[Slot]=Old[Slot];
 return Out;
}
constexpr bool ValidLayout(const FLoadout& Codes) {
 for(int Slot=0;Slot<SlotCount;++Slot)
  if(Codes[Slot]!=0&&!Valid(Codes[Slot],Slot))return false;
 return true;
}
static_assert(SlotCount<=10,"Item encoding reserves one decimal slot digit");
// Strict persistence boundary: legacy records have exactly three codes;
// expanded records have nine. Unlike preview expansion, never discard invalid
// saved items. Publish the output only after every slot passes ownership checks.
template<class Owns>
bool TryRestoreLoadout(const int* Codes,int Count,bool Expanded,Owns IsOwned,FLoadout& Out) {
 if(!Codes||Count!=(Expanded?SlotCount:LegacySlotCount))return false;
 FLoadout Candidate{};
 for(int Slot=0;Slot<Count;++Slot){
  const int Code=Codes[Slot];
  if(Code!=0&&(!Valid(Code,Slot)||!IsOwned(Code)))return false;
  Candidate[Slot]=Code;
 }
 Out=Candidate;
 return true;
}

// Exact set-aware optimization by affix counts rather than enumerating 4^9
// loadouts. A matching pair contributes 80 total (40 per equipped item).
struct FPlan {
 FLoadout Codes{};
 std::array<int,SlotCount> Indices{};
 int Gain=0, Changes=0;
 FPlan(){Indices.fill(-1);}
};
constexpr int TotalScore(const FLoadout& Codes) {
 int Score=0, Counts[4]{};
 for(int Slot=0;Slot<SlotCount;++Slot) if(Valid(Codes[Slot],Slot)){
  const int Affix=Codes[Slot]/100000;
  Score+=MoonfallGearAdvisor::Rank(Codes[Slot])*100+(Affix?10+80*Counts[Affix]:0);
  if(Affix)++Counts[Affix];
 }
 return Score;
}
class FPlanner {
 using FChoice=MoonfallGearAdvisor::FChoice;
 FLoadout Worn{};
 std::array<std::array<FChoice,4>,SlotCount> Candidates{};
 bool WeaponsOnly=false;
 int ActiveSlots=SlotCount;
 // Three decimal digits track counts of affixes 1, 2, 3. Each <=9.
 static constexpr int States=1000;
 static constexpr int Weights[4]={0,1,10,100};
 struct FState {int Score=-1, Changes=0; unsigned Path=0;};
 static bool Better(const FState& A,const FState& B){
  return A.Score>B.Score||(A.Score==B.Score&&(A.Changes<B.Changes
      ||(A.Changes==B.Changes&&A.Path<B.Path)));
 }
public:
 explicit FPlanner(const FLoadout& Equipped,bool bWeaponsOnly=false,int EnabledSlots=SlotCount)
     :WeaponsOnly(bWeaponsOnly),ActiveSlots(EnabledSlots==SlotCount?SlotCount:LegacySlotCount){
  for(int Slot=0;Slot<SlotCount;++Slot){
   Worn[Slot]=Slot<ActiveSlots&&Valid(Equipped[Slot],Slot)?Equipped[Slot]:0;
   Candidates[Slot][Worn[Slot]/100000]={Worn[Slot],-1};
  }
 }
 void Add(int Code,int Index){
  if(Code<=0||Index<0)return;
  const int Slot=(Code/10000)%10;
  if(Slot>=ActiveSlots||!Valid(Code,Slot)||(WeaponsOnly&&Slot!=0)
     ||MoonfallGearAdvisor::Rank(Code)<MoonfallGearAdvisor::Rank(Worn[Slot]))return;
  auto& C=Candidates[Slot][Code/100000];
  if(!C.Code||MoonfallGearAdvisor::Rank(Code)>MoonfallGearAdvisor::Rank(C.Code))C={Code,Index};
 }
 FPlan Finish()const{
  std::array<FState,States> Current{},Next{};
  Current[0].Score=0;
  for(int Slot=0;Slot<ActiveSlots;++Slot){
   Next.fill(FState{});
   for(int Key=0;Key<States;++Key){
    const auto& Prior=Current[Key];
    if(Prior.Score<0)continue;
    for(int Affix=0;Affix<4;++Affix){
     const auto& C=Candidates[Slot][Affix];
     if(!C.Code&&(Affix!=0||Worn[Slot]!=0))continue;
     const int Weight=Weights[Affix];
     const int Matching=Affix?(Key/Weight)%10:0;
     const int Dest=Key+Weight;
     FState New{Prior.Score+MoonfallGearAdvisor::Rank(C.Code)*100+(Affix?10+80*Matching:0),
         Prior.Changes+(C.Code!=Worn[Slot]),(Prior.Path<<2)|static_cast<unsigned>(Affix)};
     if(Better(New,Next[Dest]))Next[Dest]=New;
    }
   }
   Current=Next;
  }
  FState Best;
  for(const auto& State:Current)if(State.Score>=0&&Better(State,Best))Best=State;
  FPlan Result;
  Result.Codes=Worn;
  if(Best.Score<=TotalScore(Worn))return Result;
  Result.Gain=Best.Score-TotalScore(Worn);
  Result.Changes=Best.Changes;
  unsigned Path=Best.Path;
  for(int Slot=ActiveSlots-1;Slot>=0;--Slot){
   const auto& C=Candidates[Slot][Path&3u];
   Result.Codes[Slot]=C.Code;Result.Indices[Slot]=C.Index;Path>>=2;
  }
  return Result;
 }
};
static_assert(SlotCount==9,"Affix count encoding and path capacity assume nine slots");
} // namespace MoonfallEquipmentLayout
