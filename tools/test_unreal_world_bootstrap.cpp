// Portable Windows/Linux regression tests for Moonfall world startup.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldBootstrap.h"
#include <cassert>
namespace M=MoonfallWorldBootstrap;
int main() {
 static_assert(M::LegacyZoneCount==6 && M::TotalDungeons==30);
 const auto Legacy=M::DefaultPolicy();
 const auto Isolated=M::IsolatedWorldPolicy();
 assert(M::ShouldSpawnArena(Legacy,false));
 assert(!M::ShouldSpawnArena(Legacy,true));
 assert(!M::ShouldSpawnArena(Isolated,false));
 assert(M::ShouldGenerateDungeons(Legacy));
 assert(!M::ShouldGenerateDungeons(Isolated));
 for(int N=-3;N<=35;++N) {
  const int Batch=M::BatchCount(N);
  assert(Batch>=0 && Batch<=M::MaxLayoutsPerBatch);
  if(N<0 || N>=30) assert(Batch==0);
  if(N>=0 && N<29) assert(Batch==2);
  if(N==29) assert(Batch==1);
 }
 const M::FPolicy ArenaOnly{true,false};
 const M::FPolicy DungeonsOnly{false,true};
 assert(M::ShouldSpawnArena(ArenaOnly,false));
 assert(!M::ShouldGenerateDungeons(ArenaOnly));
 assert(!M::ShouldSpawnArena(DungeonsOnly,false));
 assert(M::ShouldGenerateDungeons(DungeonsOnly));
 return 0;
}
