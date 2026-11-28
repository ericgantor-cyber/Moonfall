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
 constexpr Coins EnemyReward(int Level,bool Elite,bool Boss) {
  return Level<1||Level>100?0:(Boss?1200:Elite?350:100)+Coins(Level-1)*15;
 }
}
