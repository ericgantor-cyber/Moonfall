#pragma once
// Portable Moonfall UE5 world bootstrap policy. Defaults match existing
// game behavior. Explicit world overrides make expansion maps testable.
namespace MoonfallWorldBootstrap {
inline constexpr int LegacyZoneCount=6;
inline constexpr int DungeonsPerZone=5;
inline constexpr int TotalDungeons=LegacyZoneCount*DungeonsPerZone;
inline constexpr int MaxLayoutsPerBatch=8;
inline constexpr double MinBatchTimeBudgetMs=0.5;
inline constexpr double DefaultBatchTimeBudgetMs=2.5;
inline constexpr double MaxBatchTimeBudgetMs=8.0;
// Portable, deterministic bounds for designer settings and frame-time gates.
constexpr int ClampBatchSize(int Requested) {
 return Requested<1?1:(Requested>MaxLayoutsPerBatch?MaxLayoutsPerBatch:Requested);
}
constexpr double ClampBatchTimeBudgetMs(double Requested) {
 return !(Requested>=0.0)?DefaultBatchTimeBudgetMs:
  (Requested<MinBatchTimeBudgetMs?MinBatchTimeBudgetMs:
   (Requested>MaxBatchTimeBudgetMs?MaxBatchTimeBudgetMs:Requested));
}
constexpr bool ShouldContinueBatch(int Processed,int RequestedBatchSize,
 double ElapsedMs,double RequestedBudgetMs) {
 return Processed>=0 && Processed<ClampBatchSize(RequestedBatchSize)
  && (Processed==0 ||
      (ElapsedMs>=0.0 &&
       ElapsedMs<ClampBatchTimeBudgetMs(RequestedBudgetMs)));
}
// Authored slots can disappear after the initial map scan (for example
// when an authored streaming level unloads during the startup timer).
constexpr bool ShouldRetainAuthoredSlot(bool ActorValid,bool ActorDestroying) {
 return ActorValid && !ActorDestroying;
}
// One retry only, after the initial pass has considered all 30 slots.
constexpr bool ShouldQueueRetry(bool Succeeded,bool AlreadyRetried) {
 return !Succeeded && !AlreadyRetried;
}
constexpr bool HasPendingDungeonWork(int NextSlot,int NextRetry,int FailedCount) {
 return NextSlot<TotalDungeons ||
  (NextRetry>=0 && FailedCount>NextRetry);
}
// Audit authored and generated layouts once after initial and retry passes.
constexpr bool ShouldAuditDungeonSlots(bool AlreadyAudited,
 int NextSlot,int NextRetry,int FailedCount) {
 return !AlreadyAudited &&
  !HasPendingDungeonWork(NextSlot,NextRetry,FailedCount);
}
struct FPolicy {
 bool SpawnLegacyArena;
 bool GenerateLegacyDungeons;
};
constexpr FPolicy DefaultPolicy() {return {true,true};}
constexpr FPolicy IsolatedWorldPolicy() {return {false,false};}
constexpr bool ShouldSpawnArena(const FPolicy& Policy,bool HasAuthoredArena) {
 return Policy.SpawnLegacyArena && !HasAuthoredArena;
}
constexpr bool ShouldGenerateDungeons(const FPolicy& Policy) {
 return Policy.GenerateLegacyDungeons;
}
constexpr int BatchCount(int NextSlot,int RequestedBatchSize=MaxLayoutsPerBatch) {
 if(NextSlot<0 || NextSlot>=TotalDungeons) return 0;
 const int Remaining=TotalDungeons-NextSlot;
 const int Limit=ClampBatchSize(RequestedBatchSize);
 return Remaining>Limit?Limit:Remaining;
}
static_assert(TotalDungeons==30);
static_assert(DefaultPolicy().SpawnLegacyArena);
static_assert(DefaultPolicy().GenerateLegacyDungeons);
static_assert(!ShouldSpawnArena(IsolatedWorldPolicy(),false));
static_assert(!ShouldGenerateDungeons(IsolatedWorldPolicy()));
static_assert(BatchCount(0)==8 && BatchCount(22)==8);
static_assert(BatchCount(23)==7 && BatchCount(27)==3);
static_assert(BatchCount(29)==1 && BatchCount(30)==0);
static_assert(BatchCount(0,4)==4 && BatchCount(29,4)==1);
static_assert(ClampBatchSize(0)==1 && ClampBatchSize(100)==8);
static_assert(ShouldContinueBatch(0,8,999.0,2.5));
static_assert(!ShouldContinueBatch(1,8,3.0,2.5));
static_assert(ShouldRetainAuthoredSlot(true,false));
static_assert(!ShouldRetainAuthoredSlot(false,false));
static_assert(!ShouldRetainAuthoredSlot(true,true));
static_assert(ShouldQueueRetry(false,false));
static_assert(!ShouldQueueRetry(false,true) && !ShouldQueueRetry(true,false));
static_assert(HasPendingDungeonWork(0,0,0));
static_assert(HasPendingDungeonWork(30,0,1));
static_assert(!HasPendingDungeonWork(30,1,1));
static_assert(!ShouldAuditDungeonSlots(false,29,0,0));
static_assert(!ShouldAuditDungeonSlots(false,30,0,1));
static_assert(ShouldAuditDungeonSlots(false,30,1,1));
static_assert(!ShouldAuditDungeonSlots(true,30,1,1));
} // namespace MoonfallWorldBootstrap
