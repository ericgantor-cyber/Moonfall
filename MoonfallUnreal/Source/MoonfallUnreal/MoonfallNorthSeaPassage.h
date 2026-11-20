#pragma once
// Native-free Northern Expanse ferry policy. This is a distinct travel
// edge, not a tenth province or a change to legacy six-zone save IDs.
#include <cstdint>
#include "MoonfallNorthZones.h"

namespace MoonfallNorth {
enum class ESeaPort : std::uint8_t { Mainland, Shiverwake };
constexpr bool ValidSeaRoute(ESeaPort Origin, ESeaPort Destination) {
 return (Origin == ESeaPort::Mainland && Destination == ESeaPort::Shiverwake)
     || (Origin == ESeaPort::Shiverwake && Destination == ESeaPort::Mainland);
}
constexpr bool MayBoard(ESeaPort Origin, ESeaPort Destination,
                        bool bBothGatesEnabled, bool bReciprocalPair,
                        bool bHeroAlive, bool bMenuOpen, bool bLandingUsable) {
 return ValidSeaRoute(Origin, Destination)
     && bBothGatesEnabled && bReciprocalPair && bHeroAlive
     && !bMenuOpen && bLandingUsable;
}
constexpr bool IsFirstIslandLanding(ESeaPort Origin, ESeaPort Destination,
                                    bool bReachedIslandThisSession) {
 return ValidSeaRoute(Origin, Destination)
     && Origin == ESeaPort::Mainland
     && !bReachedIslandThisSession;
}
static_assert(ZoneCount == 9 && Zones[0].Biome == EBiome::GlacialCoast,
              "The ferry must land at the original Shiverwake Coast province");
static_assert(ValidSeaRoute(ESeaPort::Mainland,ESeaPort::Shiverwake),
              "First voyage needs a valid northbound ferry");
static_assert(ValidSeaRoute(ESeaPort::Shiverwake,ESeaPort::Mainland),
              "The return ferry must be possible");
static_assert(!ValidSeaRoute(ESeaPort::Mainland,ESeaPort::Mainland),
              "Prevent self-loop crossings");
} // namespace MoonfallNorth
