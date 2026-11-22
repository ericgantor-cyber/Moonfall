#pragma once
// Bounded deterministic positions for optional low-cost cube precipitation.
// No randomness, game damage, fog, new actors or engine dependencies.
#include "MoonfallNorthClimate.h"
namespace MoonfallNorth {
inline constexpr int PrecipitationInstanceCount=18;
inline constexpr int PrecipitationSteps=30;
struct FPrecipitationPoint {
 int Xcm, Ycm, Zcm;
 int HalfWidthCm, HalfDepthCm, HalfHeightCm;
};
constexpr int PositiveMod(int A,int M) {
 return ((A%M)+M)%M;
}
constexpr FPrecipitationPoint PrecipitationAt(int Province,int Slot,int Step) {
 if (!ValidZone(Province) || Slot<0 || Slot>=PrecipitationInstanceCount
     || Step<0) return {};
 const EWeather Weather=Climates[Province].Type;
 const int Speed=(Weather==EWeather::WarmRain || Weather==EWeather::MountainStorm)
   ? 48 : 19;
 const int Wind=Climates[Province].SuggestedWind;
 const int Phase=Step%PrecipitationSteps;
 const int X=PositiveMod(Slot*151+Phase*(Wind/5),800)-400;
 const int Y=PositiveMod(Slot*113+Phase*(Wind/8),800)-400;
 const int Z=120+PositiveMod(Slot*131-Phase*Speed,500);
 // Thin streaks for rainfall; light flakes for ice, ash and magic dust.
 const bool Rain=(Weather==EWeather::WarmRain || Weather==EWeather::MountainStorm);
 return {X,Y,Z,Rain?2:4,Rain?2:4,Rain?14:4};
}
constexpr bool ValidPrecipitation() {
 for(int P=0;P<ZoneCount;++P)
  for(int S=0;S<PrecipitationInstanceCount;++S)
   for(int T=0;T<PrecipitationSteps;++T) {
    const auto V=PrecipitationAt(P,S,T);
    if(V.Xcm< -400 || V.Xcm>400 || V.Ycm< -400 || V.Ycm>400
       || V.Zcm<120 || V.Zcm>=620
       || V.HalfWidthCm<1 || V.HalfHeightCm>14) return false;
   }
 return PrecipitationAt(-1,0,0).Zcm==0
  && PrecipitationAt(0,PrecipitationInstanceCount,0).Zcm==0
  && PrecipitationAt(0,0,-1).Zcm==0;
}
static_assert(ValidPrecipitation(),
 "Northern precipitation preview must remain small and bounded");
} // namespace MoonfallNorth
