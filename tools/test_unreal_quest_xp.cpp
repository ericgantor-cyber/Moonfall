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
constexpr bool SameText(const char* A, const char* B) {
 while (*A && *B) {
  if (*A != *B) return false;
  ++A; ++B;
 }
 return *A == *B;
}
static_assert(SameText(MoonfallQuestXP::RankName(10), "APPRENTICE"));
static_assert(SameText(MoonfallQuestXP::RankName(15), "ADVENTURER"));
static_assert(SameText(MoonfallQuestXP::RankName(20), "VETERAN"));
static_assert(SameText(MoonfallQuestXP::RankName(25), "MASTER"));
static_assert(SameText(MoonfallQuestXP::RankName(0), "INVALID"));
static_assert(MoonfallQuestXP::Preview(10,1,90,100).RewardXP == 42);
static_assert(MoonfallQuestXP::Preview(10,1,90,100).FinalLevel == 2);
static_assert(MoonfallQuestXP::Preview(10,1,90,100).LevelsGained == 1);
static_assert(MoonfallQuestXP::Preview(10,1,90,100).RemainingXP == 32);
static_assert(MoonfallQuestXP::Preview(10,1,90,100).NextLevelXP == 155);
static_assert(MoonfallQuestXP::Preview(25,1,0,100).RemainingXP == 72);
static_assert(MoonfallQuestXP::Preview(25,2,145,155).FinalLevel == 3);
static_assert(MoonfallQuestXP::Preview(25,2,145,155).RemainingXP == 64);
static_assert(MoonfallQuestXP::Preview(25,99,5480,5490).FinalLevel == 100);
static_assert(MoonfallQuestXP::Preview(25,99,5480,5490).RemainingXP == 0);
static_assert(MoonfallQuestXP::Preview(25,100,0,0).RewardXP == 0);
static_assert(MoonfallQuestXP::Preview(25,0,0,100).RewardXP == 0);
int main() {
 for (int level=1;level<100;++level) {
  for (int cost=1;cost<=30;++cost) {
   const int xp=Reward(cost,level);
   assert(xp>0 && xp<=300);
   if(cost>1)assert(xp>Reward(cost-1,level));
   if(level>1)assert(xp>Reward(cost,level-1));
   const int next = 100 + (level-1)*55;
   for (int initial : {0, next-1, next, next+1, 999999, 1000000}) {
    const auto got=MoonfallQuestXP::Preview(cost,level,initial,next);
    int expectedLevel=level, expectedXP=initial + xp;
    if(expectedXP>1000000) expectedXP=1000000;
    int expectedNext=next;
    while(expectedXP>=expectedNext && expectedLevel<100) {
     expectedXP-=expectedNext;
     ++expectedLevel;
     expectedNext=100+(expectedLevel-1)*55;
    }
    if(expectedLevel>=100)expectedXP=0;
    assert(got.RewardXP==xp);
    assert(got.FinalLevel==expectedLevel);
    assert(got.LevelsGained==expectedLevel-level);
    assert(got.RemainingXP==expectedXP);
    assert(got.NextLevelXP==(expectedLevel>=100?0:expectedNext));
   }
  }
 }
 std::cout<<"Resident quest XP policy passed\n";
}
