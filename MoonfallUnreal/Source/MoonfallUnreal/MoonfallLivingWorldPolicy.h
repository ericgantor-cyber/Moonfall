#pragma once
namespace MoonfallWorldPolicy {
inline int Clamp(int x,int lo,int hi){return x<lo?lo:(x>hi?hi:x);}
inline int Relationship(int current,int delta){return Clamp(Clamp(current,-100,100)+Clamp(delta,-100,100),-100,100);}
inline int RelayConfidence(int confidence){return Clamp(confidence,0,100)*3/4;}
inline bool CanRelay(int confidence,int hops){return hops>=0&&hops<3&&RelayConfidence(confidence)>=20;}
// 0 unknown, 1 available, 2 active, 3 completed. Only explicit host approval advances state.
inline bool CanTransition(int from,int to,bool repeatable,bool cooldownElapsed){
 return (from==0&&to==1)||(from==1&&to==2)||(from==2&&(to==1||to==3))||(from==3&&to==1&&repeatable&&cooldownElapsed);
}
}
