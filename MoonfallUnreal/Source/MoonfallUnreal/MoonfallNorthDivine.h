#pragma once
// Proposed read-only discovery access for Z and Nyra in Northern Expanse.
// Neither boss is spawned here; no new victory flags or save schema are added.
// Their original appearances, encounter identities and existing spawns remain.
#include "MoonfallNorthMorvella.h"
namespace MoonfallNorth {
enum class EDivineAccess : unsigned char {
 Invalid, Undiscovered, RouteBlocked, Available, Defeated
};
constexpr EDivineAccess NorthernDivineAccess(int Province,
 std::uint32_t Discovered, bool AlreadyDefeated) {
 if (!ValidZone(Province) || !ValidProvinceMask(Discovered))
  return EDivineAccess::Invalid;
 if (AlreadyDefeated) return EDivineAccess::Defeated;
 if ((Discovered & (1u<<Province)) == 0u)
  return EDivineAccess::Undiscovered;
 return HasDiscoveredRoute(NorthernLandingProvince, Province, Discovered)
  ? EDivineAccess::Available : EDivineAccess::RouteBlocked;
}
constexpr EDivineAccess ZNorthernAccess(std::uint32_t Discovered,
 bool AlreadyDefeated) {
 return NorthernDivineAccess(ZProvince, Discovered, AlreadyDefeated);
}
constexpr EDivineAccess NyraNorthernAccess(std::uint32_t Discovered,
 bool AlreadyDefeated) {
 return NorthernDivineAccess(NyraProvince, Discovered, AlreadyDefeated);
}
static_assert(ZNorthernAccess(1u,false) == EDivineAccess::Undiscovered,
 "Z must not be revealed upon landing");
static_assert(NyraNorthernAccess(1u,false) == EDivineAccess::Undiscovered,
 "Nyra must not be revealed upon landing");
static_assert(ZNorthernAccess(AllProvinceBits,false) == EDivineAccess::Available,
 "Z northern sanctum can be approached by discovered routes");
static_assert(NyraNorthernAccess(AllProvinceBits,false) == EDivineAccess::Available,
 "Nyra northern sanctum can be approached by discovered routes");
static_assert(ZNorthernAccess(AllProvinceBits,true) == EDivineAccess::Defeated,
 "Victory must not reenable Z");
static_assert(NyraNorthernAccess(AllProvinceBits,true) == EDivineAccess::Defeated,
 "Victory must not reenable Nyra");
static_assert(ZNorthernAccess(1u<<ZoneCount,false) == EDivineAccess::Invalid,
 "Invalid discovery must fail closed");
} // namespace MoonfallNorth
