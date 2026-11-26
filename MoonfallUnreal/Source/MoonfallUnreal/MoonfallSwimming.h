#pragma once
#include <cmath>
namespace MoonfallSwimming {
constexpr float MaxAir=100.f, Drain=4.f, Refill=25.f, SpeedMultiplier=0.62f;
constexpr float Clamp(float V,float Lo,float Hi){return V<Lo?Lo:V>Hi?Hi:V;}
inline bool Submerged(double HeadZ,double SurfaceZ){
 return std::isfinite(HeadZ)&&std::isfinite(SurfaceZ)&&HeadZ<SurfaceZ-2.0;
}
struct FBreath {float Air=MaxAir;float DrownElapsed=0.f;};
inline bool Advance(FBreath& State,float Delta,bool InWater,bool Underwater,bool Alive){
 if(!Alive||!InWater){State={};return false;}
 if(!std::isfinite(Delta)||Delta<=0.f)return false;
 const float Step=Clamp(Delta,0.f,0.25f);
 if(!std::isfinite(State.Air)||!std::isfinite(State.DrownElapsed))State={};
 State.Air=Clamp(State.Air,0.f,MaxAir);
 if(!Underwater){
  State.Air=Clamp(State.Air+Refill*Step,0.f,MaxAir);
  State.DrownElapsed=0.f;
  return false;
 }
 const float Prior=State.Air;
 State.Air=Clamp(Prior-Drain*Step,0.f,MaxAir);
 if(State.Air>0.f){State.DrownElapsed=0.f;return false;}
 State.DrownElapsed=Clamp(State.DrownElapsed,0.f,1.f)+Clamp(Step-Prior/Drain,0.f,Step);
 if(State.DrownElapsed<1.f)return false;
 State.DrownElapsed-=1.f;
 return true;
}
}
