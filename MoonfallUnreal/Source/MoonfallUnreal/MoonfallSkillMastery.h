#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

// Pure progression rules. The gameplay layer must supply confirmed,
// deduplicated action completions, never raw input or frame ticks.
namespace MoonfallSkillMastery {
enum class Skill : int { Swordsmanship, Arcane, Agility, Defense, Strength, Survival, Count };
enum class Action : int { MeleeVictory, SpellVictory, TraversalChallenge, SuccessfulDefense, HeavyVictory, CompletedCraft };
enum class Orb : int { Agility, Hidden };
inline constexpr int SkillCount=static_cast<int>(Skill::Count);
inline constexpr int RegionCount=8;
inline constexpr int OrbsPerRegion=64;
inline constexpr std::array<int,6> RankThresholds{{0,100,300,650,1100,1800}};
inline constexpr int MaxXP=RankThresholds.back();
struct State {
 std::array<int,SkillCount> XP{};
 std::array<std::uint64_t,RegionCount> Collected{};
};
constexpr bool Valid(const State& S) {
 for(int XP:S.XP) if(XP<0||XP>MaxXP)return false;
 return true;
}
constexpr int Rank(int XP) {
 if(XP<0||XP>MaxXP)return -1;
 int Result=0;
 for(int Threshold:RankThresholds)if(XP>=Threshold)++Result;
 return Result-1;
}
constexpr int XPFor(const State& S,Skill K) {
 const int I=static_cast<int>(K);
 return I>=0&&I<SkillCount&&Valid(S)?S.XP[static_cast<std::size_t>(I)]:-1;
}
constexpr int RankFor(const State& S,Skill K) {return Rank(XPFor(S,K));}
constexpr int Reward(Action A) {
 switch(A){
  case Action::MeleeVictory:return 12;
  case Action::SpellVictory:return 12;
  case Action::TraversalChallenge:return 20;
  case Action::SuccessfulDefense:return 4;
  case Action::HeavyVictory:return 12;
  case Action::CompletedCraft:return 8;
 }
 return 0;
}
constexpr Skill SkillFor(Action A) {
 switch(A){
  case Action::MeleeVictory:return Skill::Swordsmanship;
  case Action::SpellVictory:return Skill::Arcane;
  case Action::TraversalChallenge:return Skill::Agility;
  case Action::SuccessfulDefense:return Skill::Defense;
  case Action::HeavyVictory:return Skill::Strength;
  case Action::CompletedCraft:return Skill::Survival;
 }
 return Skill::Count;
}
constexpr int SaturatingXP(int Current,int Amount) {
 return Amount>=MaxXP-Current?MaxXP:Current+Amount;
}
// Output remains unchanged on every rejected transaction. Rewards are fixed,
// bounded, and separate from character level, gear mastery and talent points.
inline bool TryAward(const State& Current,Action A,bool Confirmed,State& Out) {
 if(!Confirmed||!Valid(Current))return false;
 const int I=static_cast<int>(SkillFor(A)),Amount=Reward(A);
 if(I<0||I>=SkillCount||Amount<=0)return false;
 State Next=Current;
 auto& XP=Next.XP[static_cast<std::size_t>(I)];
 if(XP==MaxXP)return false;
 XP=SaturatingXP(XP,Amount);
 Out=Next;return true;
}
constexpr bool ValidOrbID(int Region,int Index) {
 return Region>=0&&Region<RegionCount&&Index>=0&&Index<OrbsPerRegion;
}
constexpr bool IsCollected(const State& S,int Region,int Index) {
 return ValidOrbID(Region,Index)&&
  (S.Collected[static_cast<std::size_t>(Region)]&(std::uint64_t{1}<<Index))!=0;
}
// Each region/index pair is a unique authored collectible ID, regardless
// of its type. The caller must validate the authored type and contact.
inline bool TryCollect(const State& Current,int Region,int Index,Orb Kind,bool Confirmed,State& Out) {
 if(!Confirmed||!Valid(Current)||!ValidOrbID(Region,Index)||IsCollected(Current,Region,Index))return false;
 if(Kind!=Orb::Agility&&Kind!=Orb::Hidden)return false;
 State Next=Current;
 Next.Collected[static_cast<std::size_t>(Region)]|=std::uint64_t{1}<<Index;
 if(Kind==Orb::Agility){
  auto& XP=Next.XP[static_cast<std::size_t>(Skill::Agility)];XP=SaturatingXP(XP,60);
 }else for(auto& XP:Next.XP)XP=SaturatingXP(XP,25);
 Out=Next;return true;
}
}
