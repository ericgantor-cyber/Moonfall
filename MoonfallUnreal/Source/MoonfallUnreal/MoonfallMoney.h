#pragma once
#include <cstdint>
namespace MoonfallMoney {
 using Coins=std::int64_t;
 constexpr Coins CopperPerSilver=100, CopperPerGold=10000;
 constexpr Coins MaxCoins=999999999999LL;
 constexpr bool Valid(Coins Value) { return Value>=0&&Value<=MaxCoins; }
 struct FWallet {
  Coins Copper=0;
  constexpr Coins Gold() const {return Copper/CopperPerGold;}
  constexpr Coins Silver() const {return Copper%CopperPerGold/CopperPerSilver;}
  constexpr Coins LooseCopper() const {return Copper%CopperPerSilver;}
  constexpr bool Credit(Coins Amount) {
   if(!Valid(Copper)||Amount<=0||Amount>MaxCoins-Copper)return false;
   Copper+=Amount;return true;
  }
  constexpr bool Debit(Coins Amount) {
   if(!Valid(Copper)||Amount<=0||Amount>Copper)return false;
   Copper-=Amount;return true;
  }
 };
 // Deliberately bounded vendor buyback value for one canonical equipment copy.
 constexpr Coins GearSalePrice(int Tier,int Rarity,int Trait) {
  return Tier<1||Tier>25||Rarity<0||Rarity>5||Trait<0||Trait>3||(Rarity==0&&Trait!=0)
   ?0:Coins(25+Tier*15)*(Rarity+1)+Trait*10;
 }
 constexpr int VendorTier(int Level) {return Level<1||Level>100?0:1+(Level-1)/5;}
 constexpr Coins GearPurchasePrice(int Tier,int Rarity,int Trait) {return 3*GearSalePrice(Tier,Rarity,Trait);}
 // Faction standing changes purchase prices only; even maximum discounts
 // remain well above resale. The quote is snapshotted when entering trade.
 constexpr int ReputationPricePercent(int Standing) {
  return Standing<-100||Standing>100?100:Standing>=80?80:Standing>=40?90:Standing<=-40?120:Standing<0?110:100;
 }
 constexpr Coins FactionPurchasePrice(int Tier,int Rarity,int Trait,int Standing) {
  const Coins Base=GearPurchasePrice(Tier,Rarity,Trait);
  return (Base*ReputationPricePercent(Standing)+99)/100;
 }
 // Authored faction specialties: Ashen weapons, Stone armor, Wanderer rings.
 constexpr int SpecialtySlot(int Faction) {return Faction==1?0:Faction==2?1:Faction==3?8:2;}
 constexpr int SpecialtyRarity(int Level,int Standing) {
  return Level<1||Level>100||Standing<-100||Standing>100?0:Level>=25&&Standing>=40?2:Level>=5?1:0;
 }
 constexpr int SpecialtyTrait(int Faction) {return Faction==1?1:Faction==2?2:3;}
 // One specialty per faction every 30 real-world minutes, including across saves.
 constexpr int StockGroups=4;
 constexpr Coins RestockSeconds=1800, MaxUtc=253402300799LL;
 constexpr bool ValidRestock(Coins Deadline){return Deadline>=0&&Deadline<=MaxUtc;}
 constexpr bool SpecialtyAvailable(Coins Now,Coins Deadline){
  return Now>0&&Now<=MaxUtc-RestockSeconds&&ValidRestock(Deadline)&&Now>=Deadline;
 }
 constexpr Coins NextRestock(Coins Now,Coins Deadline){
  return SpecialtyAvailable(Now,Deadline)?Now+RestockSeconds:0;
 }
 constexpr int TradeMode(int Mode) {return Mode>=0&&Mode<=2?(Mode+1)%3:0;}
 constexpr Coins EnemyReward(int Level,bool Elite,bool Boss) {
  return Level<1||Level>100?0:(Boss?1200:Elite?350:100)+Coins(Level-1)*15;
 }
}
