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
// Rank zero is untrained; ten earned ranks retain existing thresholds 1–5.
inline constexpr int MaxRank=10;
inline constexpr std::array<int,MaxRank+1> RankThresholds{{0,100,300,650,1100,1800,2600,3600,4800,6200,8000}};
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
// Independent versioned record for later native persistence integration.
// A legacy absent record is accepted only with empty payloads. Reject
// malformed or future records rather than resetting earned progress.
inline constexpr int RecordVersion=1;
template<class XPArray,class OrbArray>
bool TryRestoreRecord(int Version,const XPArray& XP,const OrbArray& Orbs,State& Out) {
 if(Version==0){
  if(!XP.empty()||!Orbs.empty())return false;
  Out=State{};return true;
 }
 if(Version!=RecordVersion||XP.size()!=static_cast<std::size_t>(SkillCount)
    ||Orbs.size()!=static_cast<std::size_t>(RegionCount))return false;
 State Next{};
 for(std::size_t I=0;I<Next.XP.size();++I){
  if(XP[I]<0||XP[I]>MaxXP)return false;
  Next.XP[I]=static_cast<int>(XP[I]);
 }
 for(std::size_t I=0;I<Next.Collected.size();++I)
  Next.Collected[I]=static_cast<std::uint64_t>(Orbs[I]);
 Out=Next;return true;
}
// Read-only unlock eligibility. Gameplay integration must still validate
// movement, collision, input accessibility and combat restrictions.
enum class Unlock : int { Vault, AdvancedClimb, PerfectGuard, HeavyFollowup, ArcaneChain, EfficientCraft };
constexpr bool HasUnlock(const State& S,Unlock U) {
 if(!Valid(S))return false;
 switch(U){
  case Unlock::Vault:return RankFor(S,Skill::Agility)>=1;
  case Unlock::AdvancedClimb:return RankFor(S,Skill::Agility)>=5;
  case Unlock::PerfectGuard:return RankFor(S,Skill::Defense)>=2;
  case Unlock::HeavyFollowup:return RankFor(S,Skill::Strength)>=3;
  case Unlock::ArcaneChain:return RankFor(S,Skill::Arcane)>=3;
  case Unlock::EfficientCraft:return RankFor(S,Skill::Survival)>=2;
 }
 return false;
}
// Bounded preview movement bonuses; never compound across jumps.
constexpr int JumpBonusBps(const State& S) {
 const int R=RankFor(S,Skill::Agility);
 constexpr std::array<int,MaxRank+1> Bonus{{0,600,1200,1800,2100,2500,2800,3100,3400,3700,4000}};
 return R<0||R>MaxRank?0:Bonus[static_cast<std::size_t>(R)];
}
// Additive basis-point bonuses relative to existing gameplay calculations.
// This is independent of the gear secondary stat also named Mastery.
constexpr int PassiveBonusBps(const State& S,Skill K) {
 const int R=RankFor(S,K);
 if(R<0)return 0;
 switch(K){
  case Skill::Swordsmanship:return R*100;
  case Skill::Arcane:return R*100;
  case Skill::Agility:return JumpBonusBps(S);
  case Skill::Defense:return R*150;
  case Skill::Strength:return R*200;
  case Skill::Survival:return R*150;
  default:return 0;
 }
}
enum class Challenge : int { Standard, Elite, Boss };
constexpr int VictoryMultiplier(Challenge C) {
 switch(C){
  case Challenge::Standard:return 1;
  case Challenge::Elite:return 2;
  case Challenge::Boss:return 4;
 }
 return 0;
}
constexpr bool VictoryAction(Action A){
 return A==Action::MeleeVictory||A==Action::SpellVictory||A==Action::HeavyVictory;
}
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
inline bool TryAward(const State& Current,Action A,bool Confirmed,State& Out,Challenge C=Challenge::Standard) {
 if(!Confirmed||!Valid(Current))return false;
 const int Factor=VictoryMultiplier(C);
 if(Factor==0||(!VictoryAction(A)&&C!=Challenge::Standard))return false;
 const int I=static_cast<int>(SkillFor(A)),Amount=Reward(A)*Factor;
 if(I<0||I>=SkillCount||Amount<=0)return false;
 State Next=Current;
 auto& XP=Next.XP[static_cast<std::size_t>(I)];
 if(XP==MaxXP)return false;
 XP=SaturatingXP(XP,Amount);
 Out=Next;return true;
}
struct Placement {int Region,Index,X,Y;Orb Kind;};
inline constexpr int MainlandRegions=6;
inline constexpr int RouteRegions=7;
inline constexpr int RouteOrbsPerRegion=12;
inline constexpr int OrbsPerTrail=4;
inline constexpr int TrailsPerRegion=RouteOrbsPerRegion/OrbsPerTrail;
inline constexpr int WorldRouteCount=RouteRegions*RouteOrbsPerRegion;
inline constexpr int RouteCompletionAgilityXP=80;
inline constexpr int RouteCompletionOtherXP=20;
inline constexpr int ProvinceCompletionAgilityXP=300;
inline constexpr int ProvinceCompletionOtherXP=60;
// Preserve the original 28 entries and region/index pairs exactly.
// Additional positions are appended, never shift saved collectible identities.
constexpr Placement RoutePlacement(int I) {
 if(I<0||I>=WorldRouteCount)return {-1,-1,0,0,Orb::Agility};
 const int Group=I/(RouteRegions*OrbsPerTrail);
 const int Region=(I%(RouteRegions*OrbsPerTrail))/OrbsPerTrail;
 const int Local=I%OrbsPerTrail,Index=Group*OrbsPerTrail+Local;
 const Orb Kind=Local==3?Orb::Hidden:Orb::Agility;
 if(Region==6){
  constexpr int Xs[]={-550,550,-900,900,-650,650,-650,650,-400,400,-400,400};
  constexpr int Ys[]={2650,2650,2800,3850,3200,3200,4300,4300,2950,2950,3750,3750};
  return {Region,Index,Xs[Index],Ys[Index],Kind};
 }
 const int Center=(Region*5-12)*1350-675;
 constexpr int XOffsets[]={-2000,-850,850,2000,-2800,-1350,1350,2800,-2400,-400,400,2400};
 constexpr int YOffsets[]={-2600,-2225,-2225,-2675,-2300,-2750,-2750,-2300,-2750,-2350,-2650,-2750};
 return {Region,Index,Center+XOffsets[Index],YOffsets[Index],Kind};
}
constexpr bool ValidOrbID(int Region,int Index) {
 return Region>=0&&Region<RegionCount&&Index>=0&&Index<OrbsPerRegion;
}
constexpr bool IsCollected(const State& S,int Region,int Index) {
 return ValidOrbID(Region,Index)&&
  (S.Collected[static_cast<std::size_t>(Region)]&(std::uint64_t{1}<<Index))!=0;
}
constexpr bool IsRouteID(int Region,int Index){
 return Region>=0&&Region<RouteRegions&&Index>=0&&Index<RouteOrbsPerRegion;
}
constexpr int RegionOrbCount(const State& S,int Region){
 if(!Valid(S)||Region<0||Region>=RouteRegions)return -1;
 int Count=0;
 for(int Index=0;Index<RouteOrbsPerRegion;++Index)if(IsCollected(S,Region,Index))++Count;
 return Count;
}
constexpr bool ProvinceCompleted(const State& S,int Region){
 return RegionOrbCount(S,Region)==RouteOrbsPerRegion;
}
constexpr bool TrailCompleted(const State& S,int Region,int Trail){
 if(!Valid(S)||Region<0||Region>=RouteRegions||Trail<0||Trail>=TrailsPerRegion)return false;
 const auto Mask=std::uint64_t{0xf}<<(Trail*OrbsPerTrail);
 return (S.Collected[static_cast<std::size_t>(Region)]&Mask)==Mask;
}
constexpr int CompletedTrails(const State& S,int Region){
 if(!Valid(S)||Region<0||Region>=RouteRegions)return -1;
 int Count=0;
 for(int Trail=0;Trail<TrailsPerRegion;++Trail)if(TrailCompleted(S,Region,Trail))++Count;
 return Count;
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
// A completed four-orb trail earns its one-time bonus in the same transaction.
// Completion is derived from the persisted orb mask; no new save fields.
inline bool TryCollectWithTrailReward(const State& Current,int Region,int Index,Orb Kind,bool Confirmed,State& Out){
 if(IsRouteID(Region,Index)&&Kind!=(Index%OrbsPerTrail==3?Orb::Hidden:Orb::Agility))return false;
 State Next{};
 if(!TryCollect(Current,Region,Index,Kind,Confirmed,Next))return false;
 if(IsRouteID(Region,Index)&&TrailCompleted(Next,Region,Index/OrbsPerTrail)){
  for(int K=0;K<SkillCount;++K){
   auto& XP=Next.XP[static_cast<std::size_t>(K)];
   XP=SaturatingXP(XP,K==static_cast<int>(Skill::Agility)?RouteCompletionAgilityXP:RouteCompletionOtherXP);
  }
 }
 if(IsRouteID(Region,Index)&&ProvinceCompleted(Next,Region)){
  for(int K=0;K<SkillCount;++K){
   auto& XP=Next.XP[static_cast<std::size_t>(K)];
   XP=SaturatingXP(XP,K==static_cast<int>(Skill::Agility)?ProvinceCompletionAgilityXP:ProvinceCompletionOtherXP);
  }
 }
 Out=Next;return true;
}
}
