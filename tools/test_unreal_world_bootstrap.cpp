// Portable Windows/Linux regression tests for Moonfall world startup.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldBootstrap.h"
#include <cassert>
namespace M=MoonfallWorldBootstrap;
int main() {
 static_assert(M::LegacyZoneCount==6 && M::TotalDungeons==30);
 static_assert(M::MaxLayoutsPerBatch==8);
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
  if(N>=0 && N<=22) assert(Batch==8);
  if(N>=23 && N<=29) assert(Batch==30-N);
  assert(M::BatchCount(N,4)==(N<0 || N>=30?0:(30-N<4?30-N:4)));
 }
 // Designer values cannot skip every batch or exceed eight layouts.
 for (int Size=-10;Size<=16;++Size) {
  const int Limit=M::ClampBatchSize(Size);
  assert(Limit>=1 && Limit<=8);
  assert(M::ShouldContinueBatch(0,Size,999.0,2.5));
  assert(!M::ShouldContinueBatch(Limit,Size,0.0,2.5));
 }
 assert(M::ClampBatchTimeBudgetMs(-1.0)==2.5);
 assert(M::ClampBatchTimeBudgetMs(0.01)==0.5);
 assert(M::ClampBatchTimeBudgetMs(1000.0)==8.0);
 assert(M::ShouldContinueBatch(7,8,2.49,2.5));
 assert(!M::ShouldContinueBatch(7,8,2.5,2.5));
 assert(!M::ShouldContinueBatch(1,8,-1.0,2.5));
 assert(M::ShouldContinueBatch(0,1,500.0,0.5));
 const M::FPolicy ArenaOnly{true,false};
 const M::FPolicy DungeonsOnly{false,true};
 assert(M::ShouldSpawnArena(ArenaOnly,false));
 assert(!M::ShouldGenerateDungeons(ArenaOnly));
 assert(!M::ShouldSpawnArena(DungeonsOnly,false));
 assert(M::ShouldGenerateDungeons(DungeonsOnly));
 return 0;
}
