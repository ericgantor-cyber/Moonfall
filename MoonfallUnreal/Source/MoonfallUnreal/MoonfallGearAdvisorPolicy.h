#pragma once
#include <array>
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
 if (!Valid(Code, Slot)) return 0;
 const int Affix = Code / 100000;
 int Matching = 0;
 for (int Other = 0; Other < 3; ++Other)
  if (Affix > 0 && Other != Slot && Valid(Equipped[Other], Other)
      && Equipped[Other] / 100000 == Affix) ++Matching;
 return (Code % 1000) * 1000 + ((Code / 1000) % 10) * 100
     + (Affix > 0 ? 10 : 0) + Matching * 40;
}
constexpr int Improvement(int Candidate, const FEquipped& Equipped) {
 if (Candidate <= 0) return 0;
 const int Slot = (Candidate / 10000) % 10;
 if (!Valid(Candidate, Slot) || Candidate == Equipped[Slot]) return 0;
 return Score(Candidate, Slot, Equipped) - Score(Equipped[Slot], Slot, Equipped);
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
} // namespace MoonfallEquipmentLayout
