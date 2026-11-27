#pragma once
#include "MoonfallNorthContinentLayout.h"
namespace MoonfallRegionalAtlas {
 struct FRect { float X,Y,W,H; };
 constexpr FRect Node(int Id,float W,float H) {
  if(!MoonfallNorth::ValidZone(Id)||W<276||H<216) return {};
  const auto P=MoonfallNorth::ContinentCenters[Id];
  const float Gap=(H-102.f)/5.f;
  return {W*(P.Xcm<0?.25f:.71f)-W*.22f,54.f+Gap*(4.f-float(P.Ycm)/13000.f),W*.44f,Gap-2.f<37.f?Gap-2.f:37.f};
 }
 constexpr int Hit(float X,float Y,float W,float H) {
  for(int I=0;I<MoonfallNorth::ZoneCount;++I) {
   const auto R=Node(I,W,H);
   if(R.W>0&&R.H>0&&X>=R.X&&X<R.X+R.W&&Y>=R.Y&&Y<R.Y+R.H) return I;
  }
  return -1;
 }
 constexpr int Step(int Current,int Direction,unsigned Available) {
  if(Direction!=1&&Direction!=-1) return -1;
  if(Current<-1||Current>=MoonfallNorth::ZoneCount) Current=-1;
  for(int I=0;I<=MoonfallNorth::ZoneCount;++I) {
   Current=(Current+1+Direction+10)%10-1;
   if(Current<0||(Available&(1u<<Current))) return Current;
  }
  return -1;
 }
}
