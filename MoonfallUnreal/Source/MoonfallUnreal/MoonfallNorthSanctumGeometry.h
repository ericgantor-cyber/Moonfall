#pragma once
// Deterministic, bounded local geometry construction plan for three sanctums.
// This is portable authoring data for a FUTURE UE5 mesh builder, not spawned
// collision, navmesh, enemy actors, world placement or an actual playable level.
#include "MoonfallNorthSanctumLayouts.h"
namespace MoonfallNorth {
enum class EFeature : unsigned char { ApproachFloor, ArenaFloor, SideWallLeft, SideWallRight, RearWall, BossMarker, EntranceMarker };
struct FPrimitive {
 EFeature Feature;
 int CenterXcm, CenterYcm;
 int HalfXcm, HalfYcm;
 bool Solid;  // Floor and walls become collision; markers never block traversal.
};
struct FSanctumPlan {
 bool Valid;
 int Count;
 FPrimitive Parts[7];
};
// Local Y+ points from the approach entrance to arena center.
// X=0 is the continuous central accessible route. Walls are *outside*
// the clear arena and the south/front edge has no closing wall.
constexpr FSanctumPlan PlanSanctum(EDivineSite Site) {
 FSanctumPlan Plan{};
 const int Index=static_cast<int>(Site);
 if (Index<0 || Index>=3) return Plan;
 const FSanctumLayout Layout=SanctumLayouts[Index];
 if (!ValidSanctumLayout(Layout)) return Plan;
 const int Radius=Layout.ArenaRadiusCm;
 const int EntranceHalf=Layout.EntranceWidthCm/2;
 const int ArenaY=Layout.ApproachLengthCm+Radius;
 Plan.Valid=true;
 Plan.Count=7;
 Plan.Parts[0]={EFeature::ApproachFloor,0,Layout.ApproachLengthCm/2,
   EntranceHalf,Layout.ApproachLengthCm/2,true};
 Plan.Parts[1]={EFeature::ArenaFloor,0,ArenaY,Radius,Radius,true};
 Plan.Parts[2]={EFeature::SideWallLeft,-Radius-100,ArenaY,100,Radius,true};
 Plan.Parts[3]={EFeature::SideWallRight,Radius+100,ArenaY,100,Radius,true};
 Plan.Parts[4]={EFeature::RearWall,0,ArenaY+Radius+100,Radius+200,100,true};
 Plan.Parts[5]={EFeature::BossMarker,0,ArenaY+Radius/3,0,0,false};
 Plan.Parts[6]={EFeature::EntranceMarker,0,0,0,0,false};
 return Plan;
}
constexpr bool SafeSanctumPlan(const FSanctumPlan& Plan) {
 if (!Plan.Valid || Plan.Count!=7) return false;
 const FPrimitive& Approach=Plan.Parts[0];
 const FPrimitive& Arena=Plan.Parts[1];
 if (Approach.Feature!=EFeature::ApproachFloor ||
     Arena.Feature!=EFeature::ArenaFloor || Approach.CenterXcm!=0 ||
     Arena.CenterXcm!=0 || Approach.HalfXcm<200 || Arena.HalfXcm<1000)
  return false;
 // Two floors touch exactly at the forward arena boundary.
 if (Approach.CenterYcm+Approach.HalfYcm !=
     Arena.CenterYcm-Arena.HalfYcm) return false;
 for (int I=2; I<5; ++I)
  if (!Plan.Parts[I].Solid) return false;
 for (int I=5; I<7; ++I)
  if (Plan.Parts[I].Solid) return false;
 return Plan.Parts[5].CenterXcm==0 && Plan.Parts[6].CenterXcm==0;
}
static_assert(SafeSanctumPlan(PlanSanctum(EDivineSite::Morvella)),
 "Morvella requires open connected floors");
static_assert(SafeSanctumPlan(PlanSanctum(EDivineSite::Z)),
 "Z requires open connected floors");
static_assert(SafeSanctumPlan(PlanSanctum(EDivineSite::Nyra)),
 "Nyra requires open connected floors");
} // namespace MoonfallNorth
