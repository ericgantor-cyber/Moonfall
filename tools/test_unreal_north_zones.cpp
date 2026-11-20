// Portable C++17 regression tests for proposed Northern Expanse provinces.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthZones.h"
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
 assert(!N::Linked(-1,0) && !N::Linked(0,9));
 std::cout<<"Moonfall northern 9-zone catalog connectivity passed\n";
}
