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
