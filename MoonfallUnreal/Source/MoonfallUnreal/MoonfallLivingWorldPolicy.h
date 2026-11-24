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
