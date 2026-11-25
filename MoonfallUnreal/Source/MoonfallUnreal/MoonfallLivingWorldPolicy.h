#pragma once
#include <cmath>
namespace MoonfallWorldPolicy {
inline int Clamp(int x,int lo,int hi){return x<lo?lo:(x>hi?hi:x);}
inline int Relationship(int current,int delta){return Clamp(Clamp(current,-100,100)+Clamp(delta,-100,100),-100,100);}
// Ambient news earns modest familiarity, not forgiveness or automatic loyalty.
inline int AmbientTrustDelta(int score,int confidence,double elapsed){
 if(score<0||score>=30||confidence<56||confidence>100||!std::isfinite(elapsed)||elapsed<60.0)return 0;
 return 1;
}
inline int RelayConfidence(int confidence){return Clamp(confidence,0,100)*3/4;}
inline bool CanRelay(int confidence,int hops){return hops>=0&&hops<3&&RelayConfidence(confidence)>=20;}
// Presentation mood only; never authorizes hostile actions or modifies quest state.
enum class Mood { Calm, Uneasy, Afraid, Vigilant, Wary, Resentful, Warm, Loyal };
inline Mood ResolveMood(int bond,int reputation,bool danger,bool credibleReport,bool guard){
 bond=Clamp(bond,-100,100);reputation=Clamp(reputation,-100,100);
 if(danger)return guard?Mood::Vigilant:Mood::Afraid;
 if(reputation<=-50||bond<=-60)return Mood::Resentful;
 if(credibleReport)return guard?Mood::Vigilant:Mood::Uneasy;
 if(reputation<=-15||bond<=-15)return Mood::Wary;
 if(bond>=60)return Mood::Loyal;
 if(bond>=15||reputation>=15)return Mood::Warm;
 return Mood::Calm;
}
// Local scout assistance is not recruitment, combat assistance or long-range following.
inline bool CanLocalGuide(int bond,int reputation,double homeDistanceSquared,double heroDistanceSquared,
                          bool safe,bool enabled){
 return enabled&&safe&&bond>=60&&reputation>=0&&std::isfinite(homeDistanceSquared)
  &&std::isfinite(heroDistanceSquared)&&homeDistanceSquared>=0&&homeDistanceSquared<=450.0*450.0
  &&heroDistanceSquared>240.0*240.0&&heroDistanceSquared<=600.0*600.0;
}
// Progression is derived from earned resident bond; no separate XP or save schema.
// Verified contract completion and distinct dungeon victories unlock non-repeatable trust milestones.
inline int ScoutDeedTrust(int current,int victories,bool contractComplete,int reputation){
 if(current<0||!contractComplete||reputation<0||victories<1||victories>30)return current;
 const int target=victories>=5?100:victories>=3?80:60;
 return current<target?target:current;
}
inline int CompanionRank(int bond){return bond>=100?3:bond>=80?2:bond>=60?1:0;}
inline int EffectiveCompanionRole(int rank,int role){return rank>=2&&rank<=3&&role>=1&&role<=2?role:0;}
// Nine two-rank nodes, three branches. Budget is shared across all paths.
constexpr int CompanionNodeCount=9;
inline int CompanionPointBudget(int rank){return rank>=1&&rank<=3?rank*2:0;}
inline int CompanionNodeRank(unsigned mask,int node){return node>=0&&node<CompanionNodeCount?int((mask>>(2*node))&3u):0;}
inline int CompanionPointsSpent(unsigned mask){
 int spent=0;for(int node=0;node<CompanionNodeCount;++node)spent+=CompanionNodeRank(mask,node);return spent;
}
inline bool ValidCompanionTraining(unsigned mask){
 if(mask>>18)return false;
 for(int node=0;node<CompanionNodeCount;++node){
  const int level=CompanionNodeRank(mask,node);
  if(level>2||(level>0&&node%3!=0&&CompanionNodeRank(mask,node-1)==0))return false;
 }
 return CompanionPointsSpent(mask)<=6;
}
constexpr unsigned CompanionTrainingMask=(1u<<18)-1u;
inline bool ValidCompanionProfile(unsigned profile){
 return (profile>>18)<=2u&&ValidCompanionTraining(profile&CompanionTrainingMask);
}
inline unsigned PackCompanionProfile(unsigned training,int role){
 return ValidCompanionTraining(training)&&role>=0&&role<=2?training|(unsigned(role)<<18):0u;
}
inline bool CanTrainCompanion(unsigned mask,int rank,int role,int node){
 if(!ValidCompanionTraining(mask)||rank<1||rank>3||role<0||role>2||node<0||node>=CompanionNodeCount
     ||node/3!=role||(role>0&&rank<2)||CompanionPointsSpent(mask)>=CompanionPointBudget(rank)
     ||CompanionNodeRank(mask,node)>=2||rank<1+node%3)return false;
 return node%3==0||CompanionNodeRank(mask,node-1)>0;
}
inline unsigned TrainCompanion(unsigned mask,int rank,int role,int node){
 return CanTrainCompanion(mask,rank,role,node)?mask+(1u<<(2*node)):mask;
}
inline int ActiveCompanionTraining(unsigned mask,int rank,int role,int node){
 if(!ValidCompanionTraining(mask)||CompanionPointsSpent(mask)>CompanionPointBudget(rank)
     ||rank<1||rank>3||role<0||role>2||node<0||node>=CompanionNodeCount
     ||node/3!=role||(role>0&&rank<2)||rank<1+node%3)return 0;
 return CompanionNodeRank(mask,node);
}
// Earned veteran perks are derived, not spendable points or persistent flags.
enum class CompanionPerk { None, Fieldcraft, ShieldExpert, Precision };
inline CompanionPerk VeteranPerk(int rank,int role){
 if(rank!=3)return CompanionPerk::None;
 const int Effective=EffectiveCompanionRole(rank,role);
 return Effective==1?CompanionPerk::ShieldExpert:Effective==2?CompanionPerk::Precision:CompanionPerk::Fieldcraft;
}
inline double CompanionCoverCooldown(int rank,int role){return VeteranPerk(rank,role)==CompanionPerk::ShieldExpert?8.0:12.0;}
inline double CompanionCoverRange(int rank,int role){return VeteranPerk(rank,role)==CompanionPerk::Fieldcraft?350.0:300.0;}
inline double CompanionStrikeRange(int rank,int role){return VeteranPerk(rank,role)==CompanionPerk::Fieldcraft?220.0:180.0;}
inline float CompanionExtraPosture(int rank,int role){return VeteranPerk(rank,role)==CompanionPerk::Precision?4.f:0.f;}
inline float CompanionStrikeDamage(int rank,int role=0){
 const float Base=rank==3?10.f:rank==2?8.f:rank==1?6.f:0.f;
 const int Effective=EffectiveCompanionRole(rank,role);
 return Effective==1?Base-2.f:Effective==2?std::fmin(Base+2.f,10.f):Base;
}
// A local companion can mitigate one ordinary directional hit per cooldown.
inline float CompanionCover(float damage,double elapsed,double distanceSquared,bool eligible,int rank=1,int role=0,unsigned training=0){
 const auto Active=[&](int node){return ActiveCompanionTraining(training,rank,role,node);};
 const double Range=CompanionCoverRange(rank,role)+10.0*Active(0)+10.0*Active(2);
 const double TrainingRecovery=Active(3)+Active(5);
 if(!eligible||rank<1||rank>3||!std::isfinite(damage)||damage<=0.f||!std::isfinite(elapsed)||elapsed+TrainingRecovery<CompanionCoverCooldown(rank,role)
    ||!std::isfinite(distanceSquared)||distanceSquared<0.0||distanceSquared>Range*Range)return 0.f;
 const float Fraction=rank==3?0.20f:rank==2?0.18f:0.15f;
 const float Cap=rank==3?16.f:rank==2?14.f:12.f;
 const int Effective=EffectiveCompanionRole(rank,role);
 const float Shift=Effective==1?0.04f:Effective==2?-0.04f:0.f;
 const float CapShift=Effective==1?2.f:Effective==2?-2.f:0.f;
 return std::fmin(damage*(Fraction+Shift+0.01f*Active(4)),Cap+CapShift+Active(1)+Active(5));
}
// Threat notifications are bounded to genuine nearby enemy attack commitments.
inline bool CanReportCompanionThreat(double now,double heroTargetDistanceSquared,double scoutHeroDistanceSquared){
 return std::isfinite(now)&&now>=0&&std::isfinite(heroTargetDistanceSquared)
    &&heroTargetDistanceSquared>=0&&heroTargetDistanceSquared<=350.0*350.0
    &&std::isfinite(scoutHeroDistanceSquared)&&scoutHeroDistanceSquared>=0
    &&scoutHeroDistanceSquared<=500.0*500.0;
}
// Counter attacks are committed to a nearby follower, never retargeted mid-swing.
inline bool CanCompanionCounter(double distanceSquared,double range,double heroDistanceSquared,bool eligible){
 return eligible&&std::isfinite(distanceSquared)&&distanceSquared>=0&&std::isfinite(range)
    &&range>0&&range<=600.0&&distanceSquared<=range*range
    &&std::isfinite(heroDistanceSquared)&&heroDistanceSquared>=0&&heroDistanceSquared<=500.0*500.0;
}
inline float FollowerIncomingDamage(float damage,int armorTier,bool braced){
 if(!std::isfinite(damage)||damage<=0.f)return 0.f;
 const float Armor=1.f-0.01f*Clamp(armorTier,0,25);
 return std::fmin(damage,100.f)*Armor*(braced?0.5f:1.f);
}
enum class CompanionTactic { Follow, Intercept, Flank, Withdraw };
inline CompanionTactic SelectCompanionTactic(float health,int rank,int role,bool threat){
 if(!threat||!std::isfinite(health)||health<=0.f||health>100.f||rank<1)return CompanionTactic::Follow;
 if(health<=25.f)return CompanionTactic::Withdraw;
 const int Effective=EffectiveCompanionRole(rank,role);
 return Effective==1?CompanionTactic::Intercept:Effective==2?CompanionTactic::Flank:CompanionTactic::Follow;
}
// Catch-up is only for healthy followers outside combat after sustained separation.
inline bool CanRecoverFollower(double distanceSquared,double separatedSeconds,bool eligible){
 return eligible&&std::isfinite(distanceSquared)&&distanceSquared>=1800.0*1800.0
    &&std::isfinite(separatedSeconds)&&separatedSeconds>=8.0;
}
inline bool ValidCompanionParty(bool recruited,int stance,float health,long long readyUtc){
 return stance>=0&&stance<=2&&std::isfinite(health)&&health>=0.f&&health<=100.f
     &&readyUtc>=0&&readyUtc<=253402300799LL&&(recruited||(stance==2&&health==100.f));
}
inline double CompanionAbilityCooldown(int accessoryTier){return 30.0-0.2*Clamp(accessoryTier,0,25);}
inline bool CanActivateCompanionAbility(int rank,int role,float health,double now,double ready,bool eligible){
 return eligible&&rank>=2&&rank<=3&&role>=0&&role<=2&&std::isfinite(health)
    &&health>0.f&&health<=100.f&&(role!=0||health<100.f)&&std::isfinite(now)
    &&now>=0&&std::isfinite(ready)&&now>=ready;
}
// Only a recent player hit or verified enemy attack commitment qualifies for assistance.
inline bool CanCompanionStrike(double age,double cooldown,double scoutDistanceSquared,
                               double heroDistanceSquared,bool eligible,int rank=1,int role=0,unsigned training=0){
 const auto Active=[&](int node){return ActiveCompanionTraining(training,rank,role,node);};
 const double RequiredCooldown=(EffectiveCompanionRole(rank,role)==2?6.0:8.0)-0.5*Active(8);
 const double Range=CompanionStrikeRange(rank,role)+10.0*Active(2)+10.0*Active(6);
 return eligible&&std::isfinite(age)&&age>=0&&age<=5.0&&std::isfinite(cooldown)&&cooldown>=RequiredCooldown
  &&std::isfinite(scoutDistanceSquared)&&scoutDistanceSquared>=0&&scoutDistanceSquared<=Range*Range
  &&std::isfinite(heroDistanceSquared)&&heroDistanceSquared>=0&&heroDistanceSquared<=350.0*350.0;
}
// Select only existing authored watch points. Reports never authorize pursuit.
enum class WatchPoint { Unchanged, West, East, North, South };
inline WatchPoint GuardWatchPoint(double dx,double dy,int confidence,bool available){
 if(!available||confidence<56||confidence>100||!std::isfinite(dx)||!std::isfinite(dy))return WatchPoint::Unchanged;
 if(std::fabs(dx)<=10.0&&std::fabs(dy)<=10.0)return WatchPoint::Unchanged;
 if(std::fabs(dx)>=std::fabs(dy))return dx<0?WatchPoint::West:WatchPoint::East;
 return dy<0?WatchPoint::South:WatchPoint::North;
}
// 0 unknown, 1 available, 2 active, 3 completed. Only explicit host approval advances state.
inline bool CanTransition(int from,int to,bool repeatable,bool cooldownElapsed){
 return (from==0&&to==1)||(from==1&&to==2)||(from==2&&(to==1||to==3))||(from==3&&to==1&&repeatable&&cooldownElapsed);
}
}
