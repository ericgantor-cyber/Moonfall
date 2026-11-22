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
 // A destroyed or unloaded authored actor must not reserve a dungeon slot.
 for (int Valid=0;Valid<=1;++Valid) {
  for (int Destroying=0;Destroying<=1;++Destroying) {
   assert(M::ShouldRetainAuthoredSlot(Valid!=0,Destroying!=0)
     ==(Valid!=0 && Destroying==0));
  }
 }
 // Every failed primary slot gets at most one late retry.
 assert(M::ShouldQueueRetry(false,false));
 assert(!M::ShouldQueueRetry(false,true));
 assert(!M::ShouldQueueRetry(true,false));
 assert(!M::ShouldQueueRetry(true,true));
 assert(M::HasPendingDungeonWork(0,0,0));
 assert(M::HasPendingDungeonWork(29,0,0));
 assert(M::HasPendingDungeonWork(30,0,1));
 assert(M::HasPendingDungeonWork(30,2,3));
 assert(!M::HasPendingDungeonWork(30,3,3));
 assert(!M::HasPendingDungeonWork(30,0,0));
 // A final scan counts generated dungeon actors the same as authored ones.
 // A successful retry may not be queued a second time if it later vanishes.
 for (int WasCounted=0;WasCounted<=1;++WasCounted) {
  for (int Valid=0;Valid<=1;++Valid) {
   for (int Destroying=0;Destroying<=1;++Destroying) {
    const bool Vanished=WasCounted && !(Valid && !Destroying);
    assert(Vanished == (WasCounted &&
      !M::ShouldRetainAuthoredSlot(Valid!=0,Destroying!=0)));
    for (int AlreadyRetried=0;AlreadyRetried<=1;++AlreadyRetried) {
     const bool QueueFinalRecovery=Vanished && !AlreadyRetried;
     assert(!QueueFinalRecovery || (WasCounted && !AlreadyRetried));
    }
   }
  }
 }
 // A one-time final audit runs only after primary and queued work finishes.
 assert(!M::ShouldAuditDungeonSlots(false,0,0,0));
 assert(!M::ShouldAuditDungeonSlots(false,29,0,0));
 assert(!M::ShouldAuditDungeonSlots(false,30,0,1));
 assert(M::ShouldAuditDungeonSlots(false,30,1,1));
 assert(M::ShouldAuditDungeonSlots(false,30,0,0));
 assert(!M::ShouldAuditDungeonSlots(true,30,1,1));
 for (int Failed=0;Failed<=M::TotalDungeons;++Failed) {
  for (int Retried=0;Retried<=Failed;++Retried) {
   assert(M::ShouldAuditDungeonSlots(false,30,Retried,Failed)
     ==(Retried==Failed));
   assert(!M::ShouldAuditDungeonSlots(true,30,Retried,Failed));
  }
 }
 // A failed actor may be retried once, but cannot create an infinite queue.
 for (int InitialFailures=0;InitialFailures<=M::TotalDungeons;++InitialFailures) {
  int NextRetry=0;
  int Attempts=0;
  while (M::HasPendingDungeonWork(30,NextRetry,InitialFailures)) {
   ++NextRetry;
   ++Attempts;
   assert(Attempts<=M::TotalDungeons);
   assert(!M::ShouldQueueRetry(false,true));
  }
  assert(Attempts==InitialFailures);
 }
 const M::FPolicy ArenaOnly{true,false};
 const M::FPolicy DungeonsOnly{false,true};
 assert(M::ShouldSpawnArena(ArenaOnly,false));
 assert(!M::ShouldGenerateDungeons(ArenaOnly));
 assert(!M::ShouldSpawnArena(DungeonsOnly,false));
 assert(M::ShouldGenerateDungeons(DungeonsOnly));
 return 0;
}
