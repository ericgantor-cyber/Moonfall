// Compile-time and runtime policy checks, no Unreal Engine dependencies.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestXP.h"
#include <cassert>
#include <iostream>
using MoonfallQuestXP::Reward;
static_assert(Reward(10,1) == 42, "Intro contract must award XP.");
static_assert(Reward(25,1) == 72, "Harder contract grants more XP.");
static_assert(Reward(10,25) == 90, "Higher-level character gains more XP.");
static_assert(Reward(25,50) == 170, "Both axes affect XP.");
static_assert(Reward(25,99) == 268, "Late-game XP stays bounded.");
static_assert(Reward(30,99) == 278, "No out-of-range reward.");
static_assert(Reward(10,100) == 0, "Maximum-level characters do not gain XP.");
static_assert(Reward(0,10) == 0, "Invalid cost cannot earn XP.");
static_assert(Reward(31,10) == 0, "Invalid cost cannot earn XP.");
static_assert(Reward(10,0) == 0, "Invalid level cannot earn XP.");
static_assert(Reward(10,-1) == 0, "Invalid level cannot earn XP.");
int main() {
 for (int level=1;level<100;++level) {
  for (int cost=1;cost<=30;++cost) {
   const int xp=Reward(cost,level);
   assert(xp>0 && xp<=300);
   if(cost>1)assert(xp>Reward(cost-1,level));
   if(level>1)assert(xp>Reward(cost,level-1));
  }
 }
 std::cout<<"Resident quest XP policy passed\n";
}
