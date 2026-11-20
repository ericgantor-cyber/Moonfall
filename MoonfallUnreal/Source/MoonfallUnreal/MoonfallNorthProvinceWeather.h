#pragma once
// Portable province detection for the optional Northern Expanse greybox.
// Coordinates are centimeters in the continent actor's local XY plane.
// Prevents changing province repeatedly on Voronoi boundaries.
#include "MoonfallNorthContinentLayout.h"
#include <cstdint>
namespace MoonfallNorth {
inline constexpr std::int64_t MaxWeatherProvinceDistanceCm=11500;
inline constexpr std::int64_t ProvinceSwitchMarginCm=1000;
constexpr std::int64_t ProvinceDistSquared(int Xcm,int Ycm,int Province) {
 if(!ValidZone(Province)) return -1;
 const std::int64_t DX=static_cast<std::int64_t>(Xcm)-ContinentCenters[Province].Xcm;
 const std::int64_t DY=static_cast<std::int64_t>(Ycm)-ContinentCenters[Province].Ycm;
 return DX*DX+DY*DY;
}
constexpr int ChooseWeatherProvince(int Xcm,int Ycm,int PreviousProvince) {
 const std::int64_t Limit=MaxWeatherProvinceDistanceCm*MaxWeatherProvinceDistanceCm;
 std::int64_t ClosestDist=Limit;
 int Closest=-1;
 for(int Zone=0; Zone<ZoneCount; ++Zone) {
  const std::int64_t Dist=ProvinceDistSquared(Xcm,Ycm,Zone);
  if(Dist<ClosestDist) {
   ClosestDist=Dist;
   Closest=Zone;
  }
 }
 if(Closest<0 || !ValidZone(PreviousProvince) || Closest==PreviousProvince)
  return Closest;
 const std::int64_t PriorDist=ProvinceDistSquared(Xcm,Ycm,PreviousProvince);
 // Stick to the previous province while still close to it; change only if
 // the new center is meaningfully closer in distance (not just a few cm).
 const std::int64_t Margin=ProvinceSwitchMarginCm;
 const std::int64_t StickyLimit=MaxWeatherProvinceDistanceCm+Margin;
 if(PriorDist<StickyLimit*StickyLimit) {
  // At most 1km of extra squared distance for the previous region; this
  // is a deliberate buffer on a coarse prototype grid, not a map boundary.
  if(PriorDist-ClosestDist <= 2*Margin*MaxWeatherProvinceDistanceCm)
   return PreviousProvince;
 }
 return Closest;
}
static_assert(ChooseWeatherProvince(0,0,-1)==0);
static_assert(ChooseWeatherProvince(-14000,13000,-1)==1);
static_assert(ChooseWeatherProvince(0,52000,-1)==8);
static_assert(ChooseWeatherProvince(999999,999999,0)==-1);
static_assert(ChooseWeatherProvince(-7000,13000,1)==1,
 "Keep climate steady near the boundary");
} // namespace MoonfallNorth
