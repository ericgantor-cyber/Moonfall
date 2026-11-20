#pragma once
// Native-free layout for an optional, editor-placeable nine-province
// connected greybox. Local XY positions (cm), not actual final world terrain.
// No changes to the existing six-zone world or saves.
#include "MoonfallNorthZones.h"
namespace MoonfallNorth {
struct FZonePoint { int Xcm; int Ycm; };
inline constexpr FZonePoint ContinentCenters[ZoneCount] = {
 {0,0}, {-14000,13000}, {0,13000},
 {0,26000}, {-14000,26000}, {-14000,39000},
 {0,39000}, {-14000,52000}, {0,52000}
};
inline constexpr int ProvincePadHalfSizeCm=1500;
inline constexpr int RoadHalfWidthCm=400;
constexpr int RoadEdgeCount() {
 int Edges=0;
 for(int A=0; A<ZoneCount; ++A)
  for(int B=A+1; B<ZoneCount; ++B)
   if(Linked(A,B)) ++Edges;
 return Edges;
}
constexpr bool ValidContinentCenters() {
 for(int A=0; A<ZoneCount; ++A) {
  for(int B=A+1; B<ZoneCount; ++B) {
   const std::int64_t DX=static_cast<std::int64_t>(ContinentCenters[A].Xcm)-ContinentCenters[B].Xcm;
   const std::int64_t DY=static_cast<std::int64_t>(ContinentCenters[A].Ycm)-ContinentCenters[B].Ycm;
   const std::int64_t DistSq=DX*DX+DY*DY;
   if(DistSq < 10000LL*10000LL) return false;
   if(Linked(A,B) && DistSq > 20000LL*20000LL) return false;
  }
 }
 return FullyConnected() && RoadEdgeCount()>8 && RoadEdgeCount()<=20
  && RoadHalfWidthCm>=350 && ProvincePadHalfSizeCm>=1200;
}
static_assert(ValidContinentCenters(),
 "Nine regional pads must be distinct and roads must be bounded");
} // namespace MoonfallNorth
