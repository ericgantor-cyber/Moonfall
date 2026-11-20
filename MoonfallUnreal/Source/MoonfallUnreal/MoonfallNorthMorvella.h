#pragma once
// Native-free access policy for Morvella's future Gravewind Barrens encounter.
// No actor spawns, save writes, UE dependencies, or changes to legacy region IDs.
// Discovered provinces are a FUTURE nine-bit northern mask, not the six-region
// 30-dungeon mask. UI must avoid revealing locked Morvella lore.
#include "MoonfallNorthZones.h"
#include <cstdint>
namespace MoonfallNorth {
inline constexpr std::uint32_t AllProvinceBits = (1u << ZoneCount) - 1u;
inline constexpr int NorthernLandingProvince = 0;
enum class EMorvellaAccess : unsigned char {
 Invalid = 0, Undiscovered, RouteBlocked, Available, Defeated
};
constexpr bool ValidProvinceMask(std::uint32_t Mask) {
 return (Mask & ~AllProvinceBits) == 0u;
}
// Breadth-first traversal through *discovered* provinces only.
// This is connectivity planning. No teleport or boss unlock is implemented.
constexpr bool HasDiscoveredRoute(int Start, int Goal, std::uint32_t Mask) {
 if (!ValidZone(Start) || !ValidZone(Goal) || !ValidProvinceMask(Mask))
  return false;
 if ((Mask & (1u << Start)) == 0u || (Mask & (1u << Goal)) == 0u)
  return false;
 std::uint32_t Reached = 1u << Start;
 for (int Pass=0; Pass<ZoneCount; ++Pass) {
  std::uint32_t Next = Reached;
  for (int Zone=0; Zone<ZoneCount; ++Zone)
   if ((Reached & (1u << Zone)) != 0u)
    Next |= Zones[Zone].NeighborMask & Mask;
  if (Next == Reached) break;
  Reached = Next;
 }
 return (Reached & (1u << Goal)) != 0u;
}
constexpr EMorvellaAccess MorvellaAccess(std::uint32_t Discovered,
 bool Defeated) {
 if (!ValidProvinceMask(Discovered))
  return EMorvellaAccess::Invalid;
 if (Defeated)
  return EMorvellaAccess::Defeated;
 if ((Discovered & (1u << MorvellaProvince)) == 0u)
  return EMorvellaAccess::Undiscovered;
 return HasDiscoveredRoute(NorthernLandingProvince, MorvellaProvince, Discovered)
  ? EMorvellaAccess::Available : EMorvellaAccess::RouteBlocked;
}
static_assert(MorvellaAccess(1u, false) == EMorvellaAccess::Undiscovered,
 "First northern landing must not reveal Morvella");
static_assert(MorvellaAccess((1u<<0)|(1u<<2)|(1u<<3), false)
 == EMorvellaAccess::Available, "Discovered tundra route can reach Gravewind");
static_assert(MorvellaAccess((1u<<0)|(1u<<3), false)
 == EMorvellaAccess::RouteBlocked, "Discovery does not create disconnected travel");
static_assert(MorvellaAccess(AllProvinceBits, true) == EMorvellaAccess::Defeated,
 "Persisted victory must suppress new encounter");
static_assert(MorvellaAccess(1u << ZoneCount, false)
 == EMorvellaAccess::Invalid, "Out-of-range province discovery fails closed");
} // namespace MoonfallNorth
