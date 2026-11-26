#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
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
// All nine player equipment slots; companion slots retain their legacy limit.
inline constexpr int ActiveSlotCount=SlotCount;
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
// The caller has already validated the selected bag index. Keep raw worn
// references (even inconsistent ones) protected until the save is repaired.
// Unworn items require no bag scan; worn items stop at the second copy.
template<class Bag>
bool CanRemoveSelectedCopy(int Code,const FLoadout& Worn,int ActiveSlots,const Bag& Inventory) {
 if(!CanOperate(Code,ActiveSlots,EInventoryOperation::Salvage))return false;
 bool Referenced=false;
 for(int Slot=0;Slot<ActiveSlots;++Slot)
  if(Worn[Slot]==Code){Referenced=true;break;}
 if(!Referenced)return true;
 int Copies=0;
 for(const int Candidate:Inventory)
  if(Candidate==Code&&++Copies>=2)return true;
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

// Validate legacy mirrors and expanded records as one transaction. Inactive
// slots must be empty; when enabled, owned expanded gear survives migration.
template<class Owns>
bool TryRestoreMirroredLoadout(const int* Legacy,int LegacyCount,
    const int* Expanded,int ExpandedCount,bool HasExpanded,int ActiveSlots,
    Owns IsOwned,FLoadout& Out) {
 if(ActiveSlots!=LegacySlotCount&&ActiveSlots!=SlotCount)return false;
 FLoadout Mirrors{};
 if(!TryRestoreLoadout(Legacy,LegacyCount,false,IsOwned,Mirrors))return false;
 if(!HasExpanded){Out=Mirrors;return true;}
 FLoadout Candidate{};
 if(!TryRestoreLoadout(Expanded,ExpandedCount,true,IsOwned,Candidate))return false;
 for(int Slot=0;Slot<LegacySlotCount;++Slot)
  if(Candidate[Slot]!=Mirrors[Slot])return false;
 for(int Slot=ActiveSlots;Slot<SlotCount;++Slot)
  if(Candidate[Slot]!=0)return false;
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
// V19 expansion preserves every V1–V18 item code byte-for-byte.
// Bank 0 stores slots 0–8, bank 1 stores slots 9–15, and bank 2 stores
// slots 16–23. Unused digits in each bank remain invalid.
enum class EFullSlot : int {
 MainHand=0, Chest=1, Trinket1=2, Head=3, Shoulders=4, Hands=5,
 Legs=6, Feet=7, Ring1=8, Neck=9, Back=10, Wrists=11, Waist=12,
 Ring2=13, Trinket2=14, OffHand=15,
 Ring3=16, Ring4=17, Ring5=18, Ring6=19, Ring7=20,
 Ring8=21, Ring9=22, Ring10=23
};
inline constexpr int SixteenSlotCount=16;
inline constexpr int FullSlotCount=24;
inline constexpr int RingSlotCount=10;
inline constexpr std::array<int,RingSlotCount> RingSlots{{8,13,16,17,18,19,20,21,22,23}};
constexpr int RingSlot(int Index) {
 return Index>=0&&Index<RingSlotCount?RingSlots[Index]:-1;
}
constexpr bool IsRingSlot(int Slot) {
 return Slot==8||Slot==13||(Slot>=SixteenSlotCount&&Slot<FullSlotCount);
}
constexpr int RingIndex(int Slot) {
 for(int Index=0;Index<RingSlotCount;++Index)
  if(RingSlots[Index]==Slot)return Index;
 return -1;
}
inline constexpr int ExtendedCodeBank=1000000;
using FFullLoadout=std::array<int,FullSlotCount>;
struct FDecodedItem {
 int Slot=-1, Trait=0, Rarity=0, Tier=0;
 constexpr bool IsValid() const {return Slot>=0;}
};
constexpr int EncodeFullItem(int Slot,int Trait,int Rarity,int Tier) {
 if(Slot<0||Slot>=FullSlotCount||Trait<0||Trait>3||Rarity<0||Rarity>5
     ||Tier<1||Tier>25||(Rarity==0&&Trait!=0))return 0;
 const int Bank=Slot<SixteenSlotCount?(Slot<SlotCount?0:1):2;
 const int Start=Bank==0?0:Bank==1?SlotCount:SixteenSlotCount;
 return Bank*ExtendedCodeBank+Trait*100000+
     (Slot-Start)*10000+Rarity*1000+Tier;
}
constexpr FDecodedItem DecodeFullItem(int Code) {
 if(Code<=0||Code>=3*ExtendedCodeBank)return {};
 const int Bank=Code/ExtendedCodeBank;
 const int Payload=Code%ExtendedCodeBank;
 const int Digit=(Payload/10000)%10;
 const int Start=Bank==0?0:Bank==1?SlotCount:SixteenSlotCount;
 const int End=Bank==0?SlotCount:Bank==1?SixteenSlotCount:FullSlotCount;
 const int Slot=Start+Digit;
 if(Slot>=End)return {};
 const int Trait=Payload/100000,Rarity=(Payload/1000)%10,Tier=Payload%1000;
 if(EncodeFullItem(Slot,Trait,Rarity,Tier)!=Code)return {};
 return {Slot,Trait,Rarity,Tier};
}
constexpr bool ValidFullItem(int Code,int Slot) {
 return Slot>=0&&Slot<FullSlotCount&&DecodeFullItem(Code).Slot==Slot;
}
// Future native loader: older records restore exact original slots and
// zero-fill only the newly introduced slots. Invalid data never publishes
// a partial loadout. V17 has nine entries; V18 has sixteen; V19 has 24.
template<class Owns>
bool TryRestoreFullLoadout(const int* Legacy,int LegacyCount,
    const int* Record,int RecordCount,int Version,Owns IsOwned,FFullLoadout& Out) {
 if(Version<1||Version>19)return false;
 FLoadout Mirrors{};
 if(!TryRestoreLoadout(Legacy,LegacyCount,false,IsOwned,Mirrors))return false;
 FFullLoadout Candidate{};
 for(int Slot=0;Slot<LegacySlotCount;++Slot)Candidate[Slot]=Mirrors[Slot];
 if(Version>=17){
  const int ExpectedCount=Version==17?SlotCount:Version==18?SixteenSlotCount:FullSlotCount;
  if(!Record||RecordCount!=ExpectedCount)return false;
  for(int Slot=0;Slot<ExpectedCount;++Slot){
   const int Code=Record[Slot];
   if(Slot<LegacySlotCount&&Code!=Mirrors[Slot])return false;
   if(Code!=0&&(!ValidFullItem(Code,Slot)||!IsOwned(Code)))return false;
   Candidate[Slot]=Code;
  }
 }
 Out=Candidate;
 return true;
}
static_assert(SixteenSlotCount-SlotCount<=10&&FullSlotCount-SixteenSlotCount<=10,
    "Each decimal bank reserves one slot digit");
static_assert(EncodeFullItem(0,1,1,5)==101005,"Legacy ID stability");
static_assert(EncodeFullItem(8,1,1,5)==181005,"V17 ID stability");
static_assert(EncodeFullItem(15,3,5,25)==1365025,"V18 off-hand ID stability");
static_assert(DecodeFullItem(EncodeFullItem(23,3,5,25)).Slot==23,"Tenth ring round trip");
static_assert(RingSlot(0)==8&&RingSlot(9)==23&&RingIndex(13)==1,"Ring order stability");
// Shared 24-slot operation boundary. Legacy runtime can explicitly retain
// its smaller active layout while migrating callers to the full codec.
constexpr bool ValidFullActiveCount(int Count) {
 return Count==LegacySlotCount||Count==SlotCount||Count==SixteenSlotCount||Count==FullSlotCount;
}
constexpr bool CanOperateFull(int Code,int ActiveSlots,EInventoryOperation Operation) {
 const auto Item=DecodeFullItem(Code);
 if(!ValidFullActiveCount(ActiveSlots)||!Item.IsValid()||Item.Slot>=ActiveSlots)return false;
 switch(Operation){
  case EInventoryOperation::Equip:
  case EInventoryOperation::Salvage:return true;
  case EInventoryOperation::Upgrade:return Item.Tier<25;
  case EInventoryOperation::Reforge:return Item.Rarity>=2;
  case EInventoryOperation::Ascend:return Item.Rarity<5;
 }
 return false;
}
// A ring's encoded slot is its stable historical item identity, not a
// restriction on which finger can wear it. Other equipment stays slot-bound.
constexpr bool CanWearFullItem(int Code,int TargetSlot,int ActiveSlots=FullSlotCount) {
 if(!CanOperateFull(Code,ActiveSlots,EInventoryOperation::Equip)
     ||TargetSlot<0||TargetSlot>=ActiveSlots)return false;
 const auto D=DecodeFullItem(Code);
 return D.Slot==TargetSlot||(IsRingSlot(D.Slot)&&IsRingSlot(TargetSlot));
}
constexpr int FullWornCopies(int Code,const FFullLoadout& Worn,int ActiveSlots) {
 if(Code<=0||!ValidFullActiveCount(ActiveSlots))return 0;
 int Count=0;
 for(int Slot=0;Slot<ActiveSlots;++Slot)if(Worn[Slot]==Code)++Count;
 return Count;
}
template<class Bag>
int FullOwnedCopies(int Code,const Bag& Inventory,int Limit=FullSlotCount+1) {
 if(Code<=0||Limit<=0)return 0;
 int Count=0;
 for(int Item:Inventory)if(Item==Code&&++Count>=Limit)break;
 return Count;
}
// Save/equip transaction boundary. Identical codes represent separate bag
// copies, never one item multiplied across several fingers. Validate all
// references and multiplicities before publishing a candidate.
template<class Bag>
bool IsFullLoadoutOwned(const FFullLoadout& Worn,int ActiveSlots,const Bag& Inventory) {
 if(!ValidFullActiveCount(ActiveSlots))return false;
 for(int Slot=0;Slot<FullSlotCount;++Slot){
  const int Code=Worn[Slot];
  if(!Code)continue;
  if(!CanWearFullItem(Code,Slot,ActiveSlots))return false;
  bool Seen=false;
  for(int Prior=0;Prior<Slot;++Prior)if(Worn[Prior]==Code){Seen=true;break;}
  if(Seen)continue;
  const int Required=FullWornCopies(Code,Worn,ActiveSlots);
  if(FullOwnedCopies(Code,Inventory,Required)<Required)return false;
 }
 return true;
}
template<class Bag>
bool TryAssignFullRing(int Code,int TargetSlot,const FFullLoadout& Worn,
    int ActiveSlots,bool InCombat,const Bag& Inventory,FFullLoadout& Out) {
 if(InCombat||!IsRingSlot(TargetSlot)||!IsRingSlot(DecodeFullItem(Code).Slot)
     ||!CanWearFullItem(Code,TargetSlot,ActiveSlots)
     ||!IsFullLoadoutOwned(Worn,ActiveSlots,Inventory)||Worn[TargetSlot]==Code)return false;
 auto Candidate=Worn;
 Candidate[TargetSlot]=Code;
 if(!IsFullLoadoutOwned(Candidate,ActiveSlots,Inventory))return false;
 Out=Candidate;
 return true;
}
template<class Bag>
bool TryMoveFullRing(int From,int To,const FFullLoadout& Worn,int ActiveSlots,
    bool InCombat,const Bag& Inventory,FFullLoadout& Out) {
 if(InCombat||!ValidFullActiveCount(ActiveSlots)||From==To
     ||!IsRingSlot(From)||!IsRingSlot(To)||From>=ActiveSlots||To>=ActiveSlots
     ||!IsFullLoadoutOwned(Worn,ActiveSlots,Inventory)||!Worn[From]
     ||Worn[From]==Worn[To])return false;
 auto Candidate=Worn;
 Candidate[From]=Worn[To];Candidate[To]=Worn[From];
 if(!IsFullLoadoutOwned(Candidate,ActiveSlots,Inventory))return false;
 Out=Candidate;
 return true;
}
// V17/V18 retain their original strict slot semantics. V19 permits ring
// interchange only after bag multiplicities and legacy mirrors are verified.
template<class Bag>
bool TryRestoreFlexibleFullLoadout(const int* Legacy,int LegacyCount,
    const int* Record,int RecordCount,int Version,const Bag& Inventory,FFullLoadout& Out) {
 FFullLoadout Candidate{};
 const auto Owns=[&Inventory](int Code){return FullOwnedCopies(Code,Inventory,1)>0;};
 if(Version<19){
  if(!TryRestoreFullLoadout(Legacy,LegacyCount,Record,RecordCount,Version,Owns,Candidate))return false;
 }else{
  if(Version!=19||!Legacy||LegacyCount!=LegacySlotCount
      ||!Record||RecordCount!=FullSlotCount)return false;
  for(int Slot=0;Slot<FullSlotCount;++Slot){
   if(Slot<LegacySlotCount&&Record[Slot]!=Legacy[Slot])return false;
   Candidate[Slot]=Record[Slot];
  }
 }
 const int Active=Version<17?LegacySlotCount:Version==17?SlotCount:Version==18?SixteenSlotCount:FullSlotCount;
 if(!IsFullLoadoutOwned(Candidate,Active,Inventory))return false;
 Out=Candidate;
 return true;
}
// Crafting never loses the bank/slot identity. Reforge RNG is supplied by
// the caller, keeping this policy deterministic and transaction-independent.
constexpr int FullUpgradeCode(int Code,int ActiveSlots) {
 const auto D=DecodeFullItem(Code);
 return CanOperateFull(Code,ActiveSlots,EInventoryOperation::Upgrade)
  ?EncodeFullItem(D.Slot,D.Trait,D.Rarity,D.Tier+1):0;
}
constexpr int FullAscendCode(int Code,int ActiveSlots) {
 const auto D=DecodeFullItem(Code);
 return CanOperateFull(Code,ActiveSlots,EInventoryOperation::Ascend)
  ?EncodeFullItem(D.Slot,D.Trait,D.Rarity+1,D.Tier):0;
}
constexpr int FullReforgeCode(int Code,int NewTrait,int ActiveSlots) {
 const auto D=DecodeFullItem(Code);
 return CanOperateFull(Code,ActiveSlots,EInventoryOperation::Reforge)
     &&NewTrait>=1&&NewTrait<=3&&NewTrait!=D.Trait
  ?EncodeFullItem(D.Slot,NewTrait,D.Rarity,D.Tier):0;
}
template<class Bag>
bool CanRemoveFullCopy(int Code,const FFullLoadout& Worn,int ActiveSlots,const Bag& Inventory) {
 if(!CanOperateFull(Code,ActiveSlots,EInventoryOperation::Salvage))return false;
 // Count every raw worn reference, including a ring on another finger.
 const int Required=FullWornCopies(Code,Worn,ActiveSlots)+1;
 return FullOwnedCopies(Code,Inventory,Required)>=Required;
}
constexpr int FullRank(int Code) {
 const auto D=DecodeFullItem(Code);
 return D.IsValid()?D.Tier*10+D.Rarity:0;
}
// Planner value only, not combat stat modifiers. Preserve previous rank
// and pair weights; the actual combat bonus thresholds remain separate.
constexpr int FullTotalScore(const FFullLoadout& Codes,int ActiveSlots=FullSlotCount) {
 if(!ValidFullActiveCount(ActiveSlots))return 0;
 int Score=0,Counts[4]{};
 for(int Slot=0;Slot<ActiveSlots;++Slot){
  const auto D=DecodeFullItem(Codes[Slot]);
  if(!CanWearFullItem(Codes[Slot],Slot,ActiveSlots))continue;
  Score+=FullRank(Codes[Slot])*100+(D.Trait?10+80*Counts[D.Trait]:0);
  if(D.Trait)++Counts[D.Trait];
 }
 return Score;
}
constexpr int FullImprovement(int Candidate,const FFullLoadout& Worn,int ActiveSlots=FullSlotCount) {
 if(!CanOperateFull(Candidate,ActiveSlots,EInventoryOperation::Equip))return 0;
 const auto D=DecodeFullItem(Candidate);
 if(Candidate==Worn[D.Slot])return 0;
 auto Next=Worn;
 Next[D.Slot]=Candidate;
 return FullTotalScore(Next,ActiveSlots)-FullTotalScore(Worn,ActiveSlots);
}
struct FFullPlan {
 FFullLoadout Codes{};
 std::array<int,FullSlotCount> Indices{};
 int Gain=0,Changes=0;
 FFullPlan(){Indices.fill(-1);}
};
// Exact dynamic program, one candidate per affix per slot. Counts use
// base 25, not decimal, so 10+ matching pieces cannot alias other traits.
// Buffers live on the heap, and only reachable triangular count states
// are visited. 48 path bits encode all 24 choices without truncation.
class FFullPlanner {
 using FChoice=MoonfallGearAdvisor::FChoice;
 FFullLoadout Worn{};
 std::array<std::array<FChoice,4>,FullSlotCount> Candidates{};
 bool WeaponsOnly=false;
 int ActiveSlots=0;
 bool RequiresFlexibleRingPlanner=false;
 static constexpr int Base=FullSlotCount+1,States=Base*Base*Base;
 static constexpr int Weights[4]={0,1,Base,Base*Base};
 struct FState {int Score=-1,Changes=0;std::uint64_t Path=0;};
 static bool Better(const FState& A,const FState& B){
  return A.Score>B.Score||(A.Score==B.Score&&(A.Changes<B.Changes
      ||(A.Changes==B.Changes&&A.Path<B.Path)));
 }
public:
 explicit FFullPlanner(const FFullLoadout& Equipped,bool CombatOnly=false,int EnabledSlots=FullSlotCount)
     :WeaponsOnly(CombatOnly),ActiveSlots(ValidFullActiveCount(EnabledSlots)?EnabledSlots:0) {
  for(int Slot=0;Slot<ActiveSlots;++Slot){
   Worn[Slot]=CanWearFullItem(Equipped[Slot],Slot,ActiveSlots)?Equipped[Slot]:0;
   // Until inventory-copy-aware optimization is integrated, never let the
   // position-specific planner discard or duplicate a flexible worn ring.
   if(Worn[Slot]&&!ValidFullItem(Worn[Slot],Slot))RequiresFlexibleRingPlanner=true;
   const auto D=DecodeFullItem(Worn[Slot]);
   Candidates[Slot][D.Trait]={Worn[Slot],-1};
  }
 }
 void Add(int Code,int Index){
  if(Index<0||!CanOperateFull(Code,ActiveSlots,EInventoryOperation::Equip))return;
  const auto D=DecodeFullItem(Code);
  if((WeaponsOnly&&D.Slot!=0)||FullRank(Code)<FullRank(Worn[D.Slot]))return;
  auto& Choice=Candidates[D.Slot][D.Trait];
  if(!Choice.Code||FullRank(Code)>FullRank(Choice.Code))Choice={Code,Index};
 }
 FFullPlan Finish()const{
  FFullPlan Result;Result.Codes=Worn;
  if(!ActiveSlots||RequiresFlexibleRingPlanner)return Result;
  std::vector<FState> Current(States),Next(States);
  Current[0].Score=0;
  for(int Slot=0;Slot<ActiveSlots;++Slot){
   std::fill(Next.begin(),Next.end(),FState{});
   for(int A=0;A<=Slot;++A)
   for(int B=0;B<=Slot-A;++B)
   for(int C=0;C<=Slot-A-B;++C){
    const int Key=A+B*Base+C*Base*Base;
    const auto& Prior=Current[Key];
    if(Prior.Score<0)continue;
    for(int Trait=0;Trait<4;++Trait){
     const auto& Choice=Candidates[Slot][Trait];
     if(!Choice.Code&&(Trait!=0||Worn[Slot]!=0))continue;
     const int Weight=Weights[Trait];
     const int Matching=Trait?(Key/Weight)%Base:0;
     const int Dest=Key+Weight;
     const FState New{Prior.Score+FullRank(Choice.Code)*100+(Trait?10+80*Matching:0),
       Prior.Changes+(Choice.Code!=Worn[Slot]),(Prior.Path<<2)|static_cast<std::uint64_t>(Trait)};
     if(Better(New,Next[Dest]))Next[Dest]=New;
    }
   }
   Current.swap(Next);
  }
  FState Best;
  for(const auto& State:Current)if(State.Score>=0&&Better(State,Best))Best=State;
  const int Baseline=FullTotalScore(Worn,ActiveSlots);
  if(Best.Score<=Baseline)return Result;
  Result.Gain=Best.Score-Baseline;Result.Changes=Best.Changes;
  auto Path=Best.Path;
  for(int Slot=ActiveSlots-1;Slot>=0;--Slot){
   const auto& Choice=Candidates[Slot][Path&3u];
   Result.Codes[Slot]=Choice.Code;Result.Indices[Slot]=Choice.Index;Path>>=2;
  }
  return Result;
 }
};
static_assert(2*FullSlotCount<=64,"Full planner path must fit in uint64");
// Optimize interchangeable rings while holding non-ring gear fixed. Each bag
// entry is a physical copy. Keep only the best ten per trait; a count choice
// needs no other candidates because all members share the same synergy.
// Never decrease aggregate ring rank or remove an occupied ring unnecessarily.
template<class Bag>
FFullPlan PlanFlexibleRings(const FFullLoadout& Worn,int ActiveSlots,bool InCombat,const Bag& Inventory) {
 FFullPlan Best;Best.Codes=Worn;
 if(InCombat||!IsFullLoadoutOwned(Worn,ActiveSlots,Inventory))return Best;
 struct FRingChoice {int Code=0,Index=-1,Rank=0;bool Retained=false;};
 std::array<std::array<FRingChoice,RingSlotCount>,4> Pool{};
 std::array<int,4> Sizes{};
 std::array<bool,RingSlotCount> RetentionClaimed{};
 int RingCapacity=0,OldRank=0,OldCount=0;
 for(int R=0;R<RingSlotCount&&RingSlot(R)<ActiveSlots;++R){
  ++RingCapacity;
  const int Code=Worn[RingSlot(R)];
  OldRank+=FullRank(Code);OldCount+=Code!=0;
 }
 const auto BetterChoice=[](const FRingChoice& X,const FRingChoice& Y){
  if(X.Rank!=Y.Rank)return X.Rank>Y.Rank;
  if(X.Retained!=Y.Retained)return X.Retained;
  return X.Index<Y.Index;
 };
 int BagIndex=0;
 for(int Code:Inventory){
  const int Index=BagIndex++;
  const auto D=DecodeFullItem(Code);
  if(!IsRingSlot(D.Slot)||!CanOperateFull(Code,ActiveSlots,EInventoryOperation::Equip))continue;
  FRingChoice Choice{Code,Index,FullRank(Code),false};
  for(int R=0;R<RingCapacity;++R)
   if(!RetentionClaimed[R]&&Worn[RingSlot(R)]==Code){
    RetentionClaimed[R]=true;Choice.Retained=true;break;
   }
  auto& Choices=Pool[D.Trait];int& Size=Sizes[D.Trait];
  int Pos=0;
  while(Pos<Size&&!BetterChoice(Choice,Choices[Pos]))++Pos;
  if(Pos>=RingCapacity)continue;
  if(Size<RingCapacity)++Size;
  for(int I=Size-1;I>Pos;--I)Choices[I]=Choices[I-1];
  Choices[Pos]=Choice;
 }
 const int Baseline=FullTotalScore(Worn,ActiveSlots);
 // At most C(14,4)=1001 count distributions for ten fingers, independent
 // of bag size. Rank prefixes make every fixed-trait count choice optimal.
 for(int A=0;A<=Sizes[0]&&A<=RingCapacity;++A)
 for(int B=0;B<=Sizes[1]&&A+B<=RingCapacity;++B)
 for(int C=0;C<=Sizes[2]&&A+B+C<=RingCapacity;++C)
 for(int D=0;D<=Sizes[3]&&A+B+C+D<=RingCapacity;++D){
  if(A+B+C+D<OldCount)continue;
  const int Counts[4]={A,B,C,D};
  std::array<FRingChoice,RingSlotCount> Picks{};
  std::array<bool,RingSlotCount> Used{};
  int Count=0,Rank=0;
  for(int Trait=0;Trait<4;++Trait)
   for(int I=0;I<Counts[Trait];++I){Picks[Count++]=Pool[Trait][I];Rank+=Pool[Trait][I].Rank;}
  if(Rank<OldRank)continue;
  FFullPlan Candidate;Candidate.Codes=Worn;
  // Keep existing fingers when their selected physical code is still present.
  for(int R=0;R<RingCapacity;++R){
   const int Slot=RingSlot(R);Candidate.Codes[Slot]=0;
   for(int I=0;I<Count;++I)
    if(!Used[I]&&Picks[I].Code==Worn[Slot]){
     Used[I]=true;Candidate.Codes[Slot]=Picks[I].Code;break;
    }
  }
  // Remaining copies fill empty positions in stable finger order.
  for(int R=0;R<RingCapacity;++R){
   const int Slot=RingSlot(R);
   if(Candidate.Codes[Slot])continue;
   for(int I=0;I<Count;++I)
    if(!Used[I]){
     Used[I]=true;Candidate.Codes[Slot]=Picks[I].Code;
     Candidate.Indices[Slot]=Picks[I].Index;break;
    }
  }
  for(int R=0;R<RingCapacity;++R)
   Candidate.Changes+=Candidate.Codes[RingSlot(R)]!=Worn[RingSlot(R)];
  Candidate.Gain=FullTotalScore(Candidate.Codes,ActiveSlots)-Baseline;
  if(Candidate.Gain>Best.Gain||(Candidate.Gain>0&&Candidate.Gain==Best.Gain
      &&Candidate.Changes<Best.Changes))Best=Candidate;
 }
 return Best;
}
} // namespace MoonfallEquipmentLayout
