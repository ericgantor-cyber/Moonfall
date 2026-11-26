#pragma once
#include "MoonfallEquipmentSlots.h"
namespace MoonfallLootLuck {
constexpr int Clamp(int V,int Lo,int Hi){return V<Lo?Lo:V>Hi?Hi:V;}
inline constexpr int MaxLuck=40;
constexpr int Luck(const MoonfallEquipmentSlots::FLoadout& Owned){
 int Total=0;
 for(int Slot=0;Slot<MoonfallEquipmentSlots::SlotCount;++Slot){
  const auto Item=MoonfallEquipmentSlots::Decode(Owned[Slot]);
  if(Item.Slot==Slot)Total+=Item.Rarity+Item.Tier/5;
 }
 return Clamp(Total,0,MaxLuck);
}
// One independent roll per item; no pity, streak state or save mutation.
// Keep the original six UE5 rarity IDs, level gates and elite/boss floors.
// At zero luck each threshold is exactly ten times the legacy 0..99 odds.
constexpr int Rarity(int Roll,int Level,bool Elite,bool Boss,int LuckStat){
 if(Level<1||Level>100||Roll<0||Roll>999)return 0;
 const int L=Clamp(LuckStat,0,MaxLuck),Bias=Clamp(Level/5,0,12);
 if(Boss){
  if(Level>=60&&Roll<120+L/2)return 5;
  if(Level>=30&&Roll<400+L)return 4;
  return 3;
 }
 if(Elite){
  if(Level>=50&&Roll<30+L/2)return 5;
  if(Level>=20&&Roll<120+L)return 4;
  return Roll<270+2*L?3:2;
 }
 if(Level>=60&&Roll<10+L/2)return 5;
 if(Level>=40&&Roll<30+L)return 4;
 if(Roll<10*(3+Bias/3)+L)return 3;
 if(Roll<10*(18+Bias)+2*L)return 2;
 return Roll<10*(53+Bias)+3*L?1:0;
}
}
