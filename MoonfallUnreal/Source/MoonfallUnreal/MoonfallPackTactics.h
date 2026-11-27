#pragma once
#include <cmath>
// Deterministic, bounded turn-taking for ordinary enemies in a local pack.
// No registry, shared mutable state, extra timer, or additional actor scan.
namespace MoonfallPackTactics {
constexpr int MaxScannedActors = 64;
constexpr int MaxNearbyCandidates = 32;
constexpr int MaxWindingAllies = 2;
enum class CombatRole : unsigned int { Vanguard=0u, Scout=1u, Ranged=2u, PackLeader=3u };
constexpr CombatRole RoleForEncounterSlot(int Slot,bool bElite) {
 if(bElite)return CombatRole::PackLeader;
 return Slot==0?CombatRole::Scout:(Slot==1?CombatRole::Ranged:CombatRole::Vanguard);
}
constexpr CombatRole RoleForBossMinion(int Slot) {
 return Slot==1?CombatRole::Ranged:CombatRole::Vanguard;
}
constexpr bool IsRangedRole(CombatRole Role) { return Role==CombatRole::Ranged; }
constexpr double RangedMinimumRange=280.0;
constexpr double RangedPreferredRange=390.0;
constexpr double RangedMaximumRange=500.0;
constexpr float RangedAttackRange=520.f;
inline int RangedRepositionMode(double Distance) {
 if(!std::isfinite(Distance)||Distance<0.0)return 0;
 return Distance<RangedMinimumRange?-1:(Distance>RangedMaximumRange?1:0);
}
constexpr bool ShouldMinionYieldToBoss(bool bBossMinion,bool bSameEncounter) {
 return bBossMinion&&bSameEncounter;
}
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
// Leaders retry sooner, but cannot bypass an occupied attack slot.
constexpr float RoleAdjustedSlotDelay(float Delay,bool Leader) {
 return Delay<=0.f?0.f:(Leader?Delay*0.8f:Delay);
}
// A waiting ordinary foe can circle at the edge of melee range, not push
// through the hero or change its committed strike. One step per deferral.
inline float YieldRepositionStep(double Distance,double Range,float Delta,bool Eligible) {
 if(!Eligible||!std::isfinite(Distance)||!std::isfinite(Range)||!std::isfinite(Delta)
    ||Range<=0.0||Distance<=Range*0.65||Distance>Range||Delta<=0.f)return 0.f;
 return std::fmin(18.f,180.f*Delta);
}
static_assert(RoleForEncounterSlot(0,false)==CombatRole::Scout
    && RoleForEncounterSlot(1,false)==CombatRole::Ranged
    && RoleForEncounterSlot(2,false)==CombatRole::Vanguard
    && RoleForEncounterSlot(0,true)==CombatRole::PackLeader,
    "Dungeon encounter roles remain deterministic");
static_assert(RoleForBossMinion(0)==CombatRole::Vanguard
    && RoleForBossMinion(1)==CombatRole::Ranged,
    "Boss adds use one vanguard and one ranged role");
static_assert(IsRangedRole(RoleForEncounterSlot(1,false))
    && RangedAttackRange>RangedMaximumRange,
    "Ranged enemies have a readable reposition and attack envelope");
static_assert(!ShouldMinionYieldToBoss(false,true)
    && !ShouldMinionYieldToBoss(true,false)
    && ShouldMinionYieldToBoss(true,true),
    "Only same-encounter boss minions yield to the boss");
static_assert(!ShouldYieldAttackSlot(0) && ShouldYieldAttackSlot(1)
    && ShouldYieldAttackSlot(2), "Pack attacks yield to active windups");
static_assert(ApproachLaneIndex(8u)==0u && ApproachLaneDistance(7u)==145.f,
    "Eight approach lanes wrap deterministically");
static_assert(AttackSlotDelay(1,0u) >= 0.4f
    && AttackSlotDelay(2,3u) <= 0.656f, "Pack slot delay stays bounded");
} // namespace MoonfallPackTactics
