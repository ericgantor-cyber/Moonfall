// Portable C++17 regression tests for proposed Northern Expanse provinces.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthZones.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthMorvella.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthDivine.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthDivineRoutes.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthSanctumLayouts.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthSanctumGeometry.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthZoneGreybox.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthContinentLayout.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthLandmarks.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthClimate.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthWeatherPreview.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthPrecipitation.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthWeatherRange.h"
#include <cassert>
#include <cstring>
#include <iostream>
namespace N=MoonfallNorth;
static_assert(N::FullyConnected() && N::SymmetricLinks());
static_assert(N::ZoneCount == 9 && N::FirstFutureRegionId == 6);
int main() {
 for(int A=0; A<N::ZoneCount; ++A) {
  const auto& Zone=N::Zones[A];
  assert(std::strlen(Zone.Id)>5 && std::strlen(Zone.Name)>7);
  assert(std::strlen(Zone.Hub)>5 && std::strlen(Zone.Dungeon)>5);
  assert(std::strlen(Zone.WorldThreat)>10 && std::strlen(Zone.Boss)>7);
  assert(Zone.SuggestedLevel>=25 && Zone.SuggestedLevel<=50);
  assert(Zone.NeighborMask!=0u && !N::Linked(A,A));
  for(int B=0; B<N::ZoneCount; ++B) {
   assert(N::Linked(A,B)==N::Linked(B,A));
   if(A!=B) {
    assert(std::strcmp(Zone.Id,N::Zones[B].Id)!=0);
    assert(std::strcmp(Zone.Name,N::Zones[B].Name)!=0);
    assert(std::strcmp(Zone.Dungeon,N::Zones[B].Dungeon)!=0);
   }
  }
 }
 // All 81 routes, including same-zone and reversed travel, must be safe.
 for (int Start=0; Start<N::ZoneCount; ++Start) {
  for (int Goal=0; Goal<N::ZoneCount; ++Goal) {
   const auto Route=N::ShortestRoute(Start,Goal);
   const auto Reverse=N::ShortestRoute(Goal,Start);
   assert(Route.Length>=1 && Route.Length<=N::ZoneCount);
   assert(Route.Stops[0]==Start);
   assert(Route.Stops[Route.Length-1]==Goal);
   assert(Reverse.Length==Route.Length);
   for (int Step=1; Step<Route.Length; ++Step)
    assert(N::Linked(Route.Stops[Step-1],Route.Stops[Step]));
   // Multiple equally short routes may be chosen depending on BFS order.
   // Reverse travel must have equal distance, not necessarily identical stops.
  }
 }
 assert(N::ShortestRoute(-1,0).Length==0);
 assert(N::ShortestRoute(0,N::ZoneCount).Length==0);
 assert(N::MorvellaHasNorthernHome());
 assert(N::MorvellaProvince==3);
 assert(std::strcmp(N::Zones[N::MorvellaProvince].Name,"Gravewind Barrens")==0);
 assert(std::strcmp(N::MorvellaSanctum,"Moonlit Necropolis")==0);
 assert(std::strcmp(N::MorvellaEncounterId,"death_goddess_crimson_reaper")==0);
 // Exhaustively verify 512 discovered-province masks, with/without victory.
 for (std::uint32_t Mask=0u; Mask<=N::AllProvinceBits; ++Mask) {
  const auto State=N::MorvellaAccess(Mask,false);
  const bool HasGravewind=(Mask & (1u<<N::MorvellaProvince))!=0u;
  const bool Reachable=N::HasDiscoveredRoute(
   N::NorthernLandingProvince,N::MorvellaProvince,Mask);
  assert(State==(HasGravewind
    ? (Reachable?N::EMorvellaAccess::Available:N::EMorvellaAccess::RouteBlocked)
    : N::EMorvellaAccess::Undiscovered));
  assert(N::MorvellaAccess(Mask,true)==N::EMorvellaAccess::Defeated);
 }
 assert(N::MorvellaAccess(1u<<N::ZoneCount,false)==N::EMorvellaAccess::Invalid);
 assert(!N::HasDiscoveredRoute(0,N::MorvellaProvince,1u<<N::ZoneCount));
 assert(N::DivineHomesValid());
 assert(std::strcmp(N::Zones[N::ZProvince].Name,"Starveil Vale")==0);
 assert(std::strcmp(N::Zones[N::NyraProvince].Name,"Nightspire Dominion")==0);
 assert(std::strcmp(N::ZEncounterId,"z_void_reaper")==0);
 assert(std::strcmp(N::NyraEncounterId,"nyra_eclipse_queen")==0);
 assert(N::ZProvince!=N::NyraProvince && N::ZProvince!=N::MorvellaProvince);
 for(std::uint32_t Mask=0; Mask<=N::AllProvinceBits; ++Mask) {
  for(int God=0; God<2; ++God) {
   const int Province=God==0?N::ZProvince:N::NyraProvince;
   const auto Status=N::NorthernDivineAccess(Province,Mask,false);
   const bool Found=(Mask & (1u<<Province))!=0u;
   const bool Reachable=N::HasDiscoveredRoute(0,Province,Mask);
   assert(Status==(Found
    ? (Reachable?N::EDivineAccess::Available:N::EDivineAccess::RouteBlocked)
    : N::EDivineAccess::Undiscovered));
   assert(N::NorthernDivineAccess(Province,Mask,true)==N::EDivineAccess::Defeated);
  }
 }
 assert(N::NorthernDivineAccess(-1,0u,false)==N::EDivineAccess::Invalid);
 assert(N::NorthernDivineAccess(9,0u,false)==N::EDivineAccess::Invalid);
 assert(N::ZNorthernAccess(1u<<N::ZoneCount,false)==N::EDivineAccess::Invalid);
 // Every potential site route must match discovered-only reachability.
 const N::EDivineSite Sites[] = {
  N::EDivineSite::Morvella, N::EDivineSite::Z, N::EDivineSite::Nyra
 };
 for (const auto Site : Sites) {
  const auto Entry=N::DivineSite(Site);
  assert(Entry.EncounterId != nullptr && Entry.Sanctum != nullptr);
  assert(N::ValidZone(Entry.Province));
  for (std::uint32_t Mask=0u; Mask<=N::AllProvinceBits; ++Mask) {
   const auto Route=N::RouteToDivineSite(Site,Mask);
   const bool Reachable=N::HasDiscoveredRoute(0,Entry.Province,Mask);
   assert((Route.Length>0)==Reachable);
   if (!Reachable) continue;
   assert(Route.Length<=N::ZoneCount && Route.Stops[0]==0);
   assert(Route.Stops[Route.Length-1]==Entry.Province);
   for (int Step=0; Step<Route.Length; ++Step) {
    const int Province=Route.Stops[Step];
    assert(N::ValidZone(Province));
    assert((Mask & (1u<<Province))!=0u);
    if (Step>0) assert(N::Linked(Route.Stops[Step-1],Province));
   }
  }
 }
 assert(N::DiscoveredRoute(-1,0,N::AllProvinceBits).Length==0);
 assert(N::DiscoveredRoute(0,9,N::AllProvinceBits).Length==0);
 assert(N::DiscoveredRoute(0,3,1u<<N::ZoneCount).Length==0);
 // All three proposed sanctum footprints have safe entrance and return paths.
 assert(N::ValidAllSanctumLayouts());
 for (const auto& Layout : N::SanctumLayouts) {
  assert(N::ValidSanctumLayout(Layout));
  assert(Layout.EntranceWidthCm>=400);
  assert(Layout.ArenaRadiusCm>=1000);
  assert(Layout.HasSafeReturnPath);
  assert(N::DivineSite(Layout.Site).Province==Layout.Province);
  auto Broken=Layout;
  Broken.EntranceWidthCm=250;
  assert(!N::ValidSanctumLayout(Broken));
  Broken=Layout;
  Broken.HasSafeReturnPath=false;
  assert(!N::ValidSanctumLayout(Broken));
  Broken=Layout;
  Broken.Province=0;
  assert(!N::ValidSanctumLayout(Broken));
 }
 // Portable geometry authoring plans: uninterrupted approach, open exit,
 // wall collision only, and no blocking markers or boss duplication.
 for (const auto Site : Sites) {
  const auto Plan=N::PlanSanctum(Site);
  assert(N::SafeSanctumPlan(Plan));
  assert(Plan.Count==7);
  assert(Plan.Parts[0].Solid && Plan.Parts[1].Solid);
  assert(Plan.Parts[0].CenterYcm+Plan.Parts[0].HalfYcm==
    Plan.Parts[1].CenterYcm-Plan.Parts[1].HalfYcm);
  assert(Plan.Parts[2].CenterXcm<0 && Plan.Parts[3].CenterXcm>0);
  assert(!Plan.Parts[5].Solid && !Plan.Parts[6].Solid);
  auto Broken=Plan;
  Broken.Parts[0].CenterYcm-=100;
  assert(!N::SafeSanctumPlan(Broken));
  Broken=Plan;
  Broken.Parts[6].Solid=true;
  assert(!N::SafeSanctumPlan(Broken));
 }
 // Greybox province terrain: nine hubs, dungeon approaches and safe exits.
 for (int Province=0; Province<N::ZoneCount; ++Province) {
  const auto Plan=N::PlanZoneGreybox(Province);
  assert(N::ValidZoneGreybox(Plan));
  assert(Plan.Province==Province && Plan.Count==5);
  assert(Plan.Pieces[0].HalfXcm>=1200);
  assert(Plan.Pieces[1].HalfXcm>=400);
  auto Broken=Plan;
  Broken.Pieces[1].Ycm+=1;
  assert(!N::ValidZoneGreybox(Broken));
  Broken=Plan;
  Broken.Pieces[3].HalfXcm=100;
  assert(!N::ValidZoneGreybox(Broken));
 }
 assert(!N::PlanZoneGreybox(-1).Valid);
 assert(!N::PlanZoneGreybox(9).Valid);
 // One contiguous nine-province greybox layout; every road maps to a
 // unique reciprocal adjacency and no province pad overlaps another.
 assert(N::ValidContinentCenters());
 int Edges=0;
 for (int A=0; A<N::ZoneCount; ++A) {
  for(int B=A+1; B<N::ZoneCount; ++B) {
   const std::int64_t DX=static_cast<std::int64_t>(N::ContinentCenters[A].Xcm)-N::ContinentCenters[B].Xcm;
   const std::int64_t DY=static_cast<std::int64_t>(N::ContinentCenters[A].Ycm)-N::ContinentCenters[B].Ycm;
   const std::int64_t DistanceSquared=DX*DX+DY*DY;
   assert(DistanceSquared >= 10000LL*10000LL);
   if (N::Linked(A,B)) {
    ++Edges;
    assert(DistanceSquared <= 20000LL*20000LL);
   }
  }
 }
 assert(Edges==N::RoadEdgeCount());
 assert(Edges>=N::ZoneCount-1 && Edges<=20);
 // Nine distinct province landmark pairs; no collision or road blockage
 // is implemented by the decorative geometry component.
 assert(N::ValidLandmarks());
 assert(N::LandmarkCount==18);
 for(int P=0; P<N::ZoneCount; ++P) {
  for(int I=0; I<N::LandmarksPerProvince; ++I) {
   const auto& L=N::Landmarks[P*N::LandmarksPerProvince+I];
   assert(L.Province==P);
   assert(L.HeightCm>=300 && L.HeightCm<=2000);
   const int AbsX=L.OffsetXcm<0?-L.OffsetXcm:L.OffsetXcm;
   const int AbsY=L.OffsetYcm<0?-L.OffsetYcm:L.OffsetYcm;
   assert(AbsX>=N::ProvincePadHalfSizeCm+L.HalfWidthCm);
   assert(AbsY>=N::ProvincePadHalfSizeCm+L.HalfWidthCm);
  }
 }
 // All nine climate labels are explicit and cannot reduce designed visibility
 // below 80%; no visual color cue may be the only weather indicator.
 assert(N::ValidClimates());
 for (int I=0; I<N::ZoneCount; ++I) {
  const auto* Climate=N::ClimateFor(I);
  assert(Climate!=nullptr);
  assert(std::strlen(Climate->Name)>=8);
  assert(Climate->HasTextCue);
  assert(Climate->VisibilityFloor>=80 && Climate->VisibilityFloor<=100);
  assert(Climate->SuggestedWind>=0 && Climate->SuggestedWind<=100);
  assert(Climate->Red>=125 && Climate->Green>=125 && Climate->Blue>=125);
  for(int J=I+1; J<N::ZoneCount; ++J)
   assert(std::strcmp(Climate->Name,N::Climates[J].Name)!=0);
 }
 assert(N::ClimateFor(-1)==nullptr);
 assert(N::ClimateFor(N::ZoneCount)==nullptr);
 // Check both full ambience cycles for every province and fail-closed indices.
 assert(N::ValidWeatherPreview());
 for(int Province=0; Province<N::ZoneCount; ++Province) {
  for(int Step=0; Step<2*N::WeatherPreviewSteps; ++Step) {
   const int Intensity=N::PreviewIntensityPercent(Province,Step);
   assert(Intensity>=15 && Intensity<=75);
   assert(N::PreviewBrightness(Province,Step)>=200);
   assert(N::PreviewBrightness(Province,Step)<=575);
   assert(Intensity==N::PreviewIntensityPercent(
     Province,Step+N::WeatherPreviewSteps));
  }
 }
 assert(N::PreviewIntensityPercent(-1,0)==0);
 assert(N::PreviewIntensityPercent(N::ZoneCount,0)==0);
 assert(N::PreviewBrightness(-1,0)==0);
 assert(N::PreviewIntensityPercent(0,-1)==0);
 // Exhaustive precipitation bounds for 9 provinces, 18 slots, 30 steps.
 assert(N::ValidPrecipitation());
 for(int Province=0; Province<N::ZoneCount; ++Province)
  for(int Slot=0;Slot<N::PrecipitationInstanceCount;++Slot)
   for(int Step=0;Step<N::PrecipitationSteps;++Step) {
    const auto P=N::PrecipitationAt(Province,Slot,Step);
    assert(P.Xcm>=-400 && P.Xcm<=400);
    assert(P.Ycm>=-400 && P.Ycm<=400);
    assert(P.Zcm>=120 && P.Zcm<620);
    assert(P.HalfWidthCm>0 && P.HalfHeightCm>0 && P.HalfHeightCm<=14);
    assert(P.Zcm==N::PrecipitationAt(Province,Slot,Step+N::PrecipitationSteps).Zcm);
   }
 assert(N::PrecipitationAt(-1,0,0).Zcm==0);
 assert(N::PrecipitationAt(0,N::PrecipitationInstanceCount,0).Zcm==0);
 assert(N::PrecipitationAt(0,0,-1).Zcm==0);
 // Weather stays active inside the exit buffer, preventing timer thrashing
 // around the border while still shutting off outside the wider radius.
 for(int Radius=800;Radius<=8000;Radius+=200) {
  assert(N::WithinWeatherRange(false,
    static_cast<float>(Radius)*Radius,static_cast<float>(Radius)));
  assert(N::WithinWeatherRange(true,
    static_cast<float>(Radius+200)*(Radius+200),static_cast<float>(Radius)));
  assert(!N::WithinWeatherRange(false,
    static_cast<float>(Radius+1)*(Radius+1),static_cast<float>(Radius)));
  assert(!N::WithinWeatherRange(true,
    static_cast<float>(Radius+251)*(Radius+251),static_cast<float>(Radius)));
 }
 assert(!N::WithinWeatherRange(false,-1.f,2500.f));
 assert(!N::WithinWeatherRange(false,0.f,799.f));
 assert(!N::WithinWeatherRange(false,0.f,8001.f));
 assert(!N::Linked(-1,0) && !N::Linked(0,9));
 std::cout<<"Moonfall northern 9-zone catalog connectivity passed\n";
}
