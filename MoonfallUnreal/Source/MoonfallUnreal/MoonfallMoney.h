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
 // Historical V1 formula is immutable; future balance changes use a new revision.
 constexpr int CurrentSalePriceVersion=1;
 constexpr Coins GearSalePriceV1(int Tier,int Rarity,int Trait) {
  return Tier<1||Tier>25||Rarity<0||Rarity>5||Trait<0||Trait>3||(Rarity==0&&Trait!=0)
   ?0:Coins(25+Tier*15)*(Rarity+1)+Trait*10;
 }
 constexpr Coins HistoricalSalePrice(int Version,int Tier,int Rarity,int Trait) {
  return Version==1?GearSalePriceV1(Tier,Rarity,Trait):0;
 }
 constexpr Coins GearSalePrice(int Tier,int Rarity,int Trait) {
  return HistoricalSalePrice(CurrentSalePriceVersion,Tier,Rarity,Trait);
 }
 constexpr bool ValidHistoricalSale(int Version,int Tier,int Rarity,int Trait,Coins Price) {
  return Price>0&&Valid(Price)&&Price==HistoricalSalePrice(Version,Tier,Rarity,Trait);
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
 // Bounded, newest-first session history; each record represents one sold copy.
 struct FBuybackEntry {int Code=0; Coins Price=0; int PriceVersion=CurrentSalePriceVersion;};
 struct FBuybackHistory {
  static constexpr int Capacity=12;
  FBuybackEntry Entries[Capacity]{};
  int Count=0;
  constexpr bool ValidCount() const {return Count>=0&&Count<=Capacity;}
  constexpr FBuybackEntry Get(int Index) const {return ValidCount()&&Index>=0&&Index<Count?Entries[Index]:FBuybackEntry{};}
  constexpr bool Push(int Code,Coins Price,int Version=CurrentSalePriceVersion) {
   if(Code<=0||Price<=0||!Valid(Price)||Version!=1||!ValidCount())return false;
   if(Count<Capacity)++Count;
   for(int I=Count-1;I>0;--I)Entries[I]=Entries[I-1];
   Entries[0]={Code,Price,Version};return true;
  }
  constexpr bool Remove(int Index) {
   if(!ValidCount()||Index<0||Index>=Count)return false;
   for(int I=Index;I+1<Count;++I)Entries[I]=Entries[I+1];
   Entries[--Count]={};return true;
  }
  constexpr int Browse(int Index,int Direction) const {
   if(!ValidCount()||Count==0)return 0;
   if(Index<0||Index>=Count)Index=0;
   return (Index+(Direction<0?-1:Direction>0?1:0)+Count)%Count;
  }
 };
 // Confirm only the identical quote and bag revision the player reviewed.
 struct FTradeIntent {
  int Mode=-1, Code=0, Index=-1;
  Coins Price=0;
  std::uint32_t Revision=0;
  constexpr bool Matches(const FTradeIntent& Other) const {
   return Mode>=0&&Mode<=2&&Code>0&&Index>=0&&Price>0&&Valid(Price)&&Revision!=0
    &&Mode==Other.Mode&&Code==Other.Code&&Index==Other.Index&&Price==Other.Price&&Revision==Other.Revision;
  }
 };
 constexpr int TradeMode(int Mode) {return Mode>=0&&Mode<=2?(Mode+1)%3:0;}
 // Contracts award coins even at the level cap; never block quest progress
 // merely because a valid wallet has reached its currency ceiling.
 constexpr Coins ContractReward(int Cost,int Level) {
  return Cost<1||Cost>30||Level<1||Level>100?0:Coins(75)+Cost*20+Level*5;
 }
 constexpr Coins BoundedRewardCredit(Coins Reward,Coins Balance) {
  if(!Valid(Balance)||Reward<=0||!Valid(Reward))return 0;
  const Coins Room=MaxCoins-Balance;
  return Reward<Room?Reward:Room;
 }
 constexpr Coins ContractCredit(int Cost,int Level,Coins Balance) {
  return BoundedRewardCredit(ContractReward(Cost,Level),Balance);
 }
 // Bag progression: preserve oversized legacy bags but prohibit new additions
 // until they fit. The original 10,000-entry serialization ceiling remains.
 constexpr int BagStart=100,BagMax=500,BagStep=50,VaultMax=1000;
 constexpr bool VaultHasRoom(int Count) {return Count>=0&&Count<VaultMax;}
 // Page browsing is clamped, never wraps across the entire storage bank.
 constexpr int VaultPageSize=25;
 constexpr int VaultPageSelection(int Index,int Count,int Direction) {
  if(Count<=0)return 0;
  const int Last=Count-1;
  const int Current=Index<0?0:Index>Last?Last:Index;
  if(Direction<0)return Current<VaultPageSize?0:Current-VaultPageSize;
  if(Direction>0)return Last-Current<VaultPageSize?Last:Current+VaultPageSize;
  return Current;
 }
 constexpr bool ValidBagCapacity(int Capacity) {
  return Capacity>=BagStart&&Capacity<=BagMax&&(Capacity-BagStart)%BagStep==0;
 }
 constexpr bool BagHasRoom(int Count,int Capacity) {
  return Count>=0&&ValidBagCapacity(Capacity)&&Count<Capacity;
 }
 constexpr Coins BagUpgradePrice(int Capacity) {
  return ValidBagCapacity(Capacity)&&Capacity<BagMax?1000+1500*((Capacity-BagStart)/BagStep):0;
 }
 constexpr Coins EnemyReward(int Level,bool Elite,bool Boss) {
  return Level<1||Level>100?0:(Boss?1200:Elite?350:100)+Coins(Level-1)*15;
 }
}
