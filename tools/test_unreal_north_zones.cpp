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
 assert(!N::Linked(-1,0) && !N::Linked(0,9));
 std::cout<<"Moonfall northern 9-zone catalog connectivity passed\n";
}
