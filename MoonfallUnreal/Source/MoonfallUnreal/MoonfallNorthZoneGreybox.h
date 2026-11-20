#pragma once
// Native-free, deterministic starting geometry for all nine Northern Expanse
// provinces. Plans are local centimenter coordinates, not world locations.
// Each instance represents ONE zone. Landscapes and actual travel remain
// a separate UE5 World Partition authoring/integration step.
#include "MoonfallNorthZones.h"
namespace MoonfallNorth {
enum class EZonePiece : unsigned char {
 HubFloor, NorthRoad, DungeonFloor, SouthRoad, EntranceFloor
};
struct FZonePiece {
 EZonePiece Kind;
 int Xcm, Ycm, HalfXcm, HalfYcm;
};
struct FZoneGreybox {
 bool Valid;
 int Province;
 int Count;
 FZonePiece Pieces[5];
};
// A generous 24m-wide plaza, two 8m-wide road sections, and a dungeon
// forecourt. All pieces touch without requiring teleportation.
constexpr FZoneGreybox PlanZoneGreybox(int Province) {
 FZoneGreybox Plan{};
 if (!ValidZone(Province)) return Plan;
 Plan.Valid=true;
 Plan.Province=Province;
 Plan.Count=5;
 Plan.Pieces[0]={EZonePiece::HubFloor,0,0,1200,1200};
 Plan.Pieces[1]={EZonePiece::NorthRoad,0,2200,400,1000};
 Plan.Pieces[2]={EZonePiece::DungeonFloor,0,4200,1200,1000};
 Plan.Pieces[3]={EZonePiece::SouthRoad,0,-2200,400,1000};
 Plan.Pieces[4]={EZonePiece::EntranceFloor,0,-3800,1200,600};
 return Plan;
}
constexpr bool ValidZoneGreybox(const FZoneGreybox& Plan) {
 if (!Plan.Valid || !ValidZone(Plan.Province) || Plan.Count!=5) return false;
 for (int I=0; I<Plan.Count; ++I)
  if (Plan.Pieces[I].HalfXcm<400 || Plan.Pieces[I].HalfYcm<400)
   return false;
 const auto& P=Plan.Pieces;
 return P[0].Kind==EZonePiece::HubFloor
  && P[1].Kind==EZonePiece::NorthRoad
  && P[2].Kind==EZonePiece::DungeonFloor
  && P[3].Kind==EZonePiece::SouthRoad
  && P[4].Kind==EZonePiece::EntranceFloor
  && P[0].Xcm==0 && P[1].Xcm==0 && P[2].Xcm==0
  && P[3].Xcm==0 && P[4].Xcm==0
  && P[0].Ycm+P[0].HalfYcm==P[1].Ycm-P[1].HalfYcm
  && P[1].Ycm+P[1].HalfYcm==P[2].Ycm-P[2].HalfYcm
  && P[3].Ycm+P[3].HalfYcm==P[0].Ycm-P[0].HalfYcm
  && P[4].Ycm+P[4].HalfYcm==P[3].Ycm-P[3].HalfYcm;
}
static_assert(ValidZoneGreybox(PlanZoneGreybox(0)),
 "Northern landing hub needs connected paths");
static_assert(ValidZoneGreybox(PlanZoneGreybox(8)),
 "Endgame province requires open roads");
static_assert(!ValidZoneGreybox(PlanZoneGreybox(9)),
 "Out-of-range province cannot create geometry");
} // namespace MoonfallNorth
