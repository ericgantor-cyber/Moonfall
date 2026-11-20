#pragma once
// Portable Moonfall UE5 world bootstrap policy. Defaults match existing
// game behavior. Explicit world overrides make expansion maps testable.
namespace MoonfallWorldBootstrap {
inline constexpr int LegacyZoneCount=6;
inline constexpr int DungeonsPerZone=5;
inline constexpr int TotalDungeons=LegacyZoneCount*DungeonsPerZone;
inline constexpr int MaxLayoutsPerBatch=2;
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
constexpr int BatchCount(int NextSlot) {
 if(NextSlot<0 || NextSlot>=TotalDungeons) return 0;
 const int Remaining=TotalDungeons-NextSlot;
 return Remaining>MaxLayoutsPerBatch?MaxLayoutsPerBatch:Remaining;
}
static_assert(TotalDungeons==30);
static_assert(DefaultPolicy().SpawnLegacyArena);
static_assert(DefaultPolicy().GenerateLegacyDungeons);
static_assert(!ShouldSpawnArena(IsolatedWorldPolicy(),false));
static_assert(!ShouldGenerateDungeons(IsolatedWorldPolicy()));
static_assert(BatchCount(0)==2 && BatchCount(29)==1 && BatchCount(30)==0);
} // namespace MoonfallWorldBootstrap
