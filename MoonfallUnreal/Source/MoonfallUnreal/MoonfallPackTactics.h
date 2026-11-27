#pragma once
// Deterministic, bounded turn-taking for ordinary enemies in a local pack.
// No registry, shared mutable state, extra timer, or additional actor scan.
namespace MoonfallPackTactics {
constexpr int MaxScannedActors = 64;
constexpr int MaxNearbyCandidates = 32;
constexpr int MaxWindingAllies = 2;
constexpr unsigned int ApproachLaneIndex(unsigned int ActorId) {
 return ActorId % 8u;
}
constexpr float ApproachLaneSign(unsigned int ActorId) {
 return ApproachLaneIndex(ActorId) % 2u == 0u ? -1.f : 1.f;
}
constexpr float ApproachLaneDistance(unsigned int ActorId) {
 return 70.f + 25.f * static_cast<float>(ApproachLaneIndex(ActorId) / 2u);
}
constexpr bool ShouldYieldAttackSlot(int WindingAllies) {
 return WindingAllies > 0;
}
constexpr float AttackSlotDelay(int WindingAllies, unsigned int ActorId) {
 if (WindingAllies <= 0) return 0.f;
 const int Count = WindingAllies > MaxWindingAllies ? MaxWindingAllies : WindingAllies;
 return 0.25f + 0.15f * static_cast<float>(Count)
     + 0.035f * static_cast<float>(ActorId % 4u);
}
static_assert(!ShouldYieldAttackSlot(0) && ShouldYieldAttackSlot(1)
    && ShouldYieldAttackSlot(2), "Pack attacks yield to active windups");
static_assert(ApproachLaneIndex(8u)==0u && ApproachLaneDistance(7u)==145.f,
    "Eight approach lanes wrap deterministically");
static_assert(AttackSlotDelay(1,0u) >= 0.4f
    && AttackSlotDelay(2,3u) <= 0.656f, "Pack slot delay stays bounded");
} // namespace MoonfallPackTactics
