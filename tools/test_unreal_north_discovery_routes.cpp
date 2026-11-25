// Exhaustive portable C++17 tests of Northern Expanse discovered-only route policy.
// No Unreal actors, fast travel, unlocked encounters, save changes or character assets.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthMorvella.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthDivine.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthDivineRoutes.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace N = MoonfallNorth;
static_assert(N::ZoneCount == 9);
static_assert(N::AllProvinceBits == 511u);
static_assert(N::NorthernLandingProvince == 0);
static_assert(N::FullyConnected() && N::SymmetricLinks());
static_assert(!N::ValidProvinceMask(1u << N::ZoneCount));
static_assert(N::MorvellaProvince != N::ZProvince);
static_assert(N::MorvellaProvince != N::NyraProvince);
static_assert(N::ZProvince != N::NyraProvince);

// Independent BFS finds the shortest path using only discovered nodes.
int IndependentDistance(int start, int goal, std::uint32_t mask) {
    if (!N::ValidZone(start) || !N::ValidZone(goal) || !N::ValidProvinceMask(mask)
        || (mask & (1u << start)) == 0u || (mask & (1u << goal)) == 0u) return 0;
    int distance[N::ZoneCount] = {};
    int queue[N::ZoneCount] = {};
    int head = 0, tail = 0;
    queue[tail++] = start;
    distance[start] = 1; // Node count rather than edge count.
    while (head < tail) {
        const int current = queue[head++];
        if (current == goal) return distance[current];
        for (int next = 0; next < N::ZoneCount; ++next) {
            if (N::Linked(current, next) && (mask & (1u << next)) != 0u
                && distance[next] == 0) {
                distance[next] = distance[current] + 1;
                queue[tail++] = next;
            }
        }
    }
    return 0;
}
int main() {
    constexpr auto all = N::AllProvinceBits;
    std::uint64_t checkedPaths = 0;
    for (std::uint32_t mask = 0; mask <= all; ++mask) {
        assert(N::ValidProvinceMask(mask));
        for (int start = 0; start < N::ZoneCount; ++start) {
            for (int goal = 0; goal < N::ZoneCount; ++goal) {
                const auto route = N::DiscoveredRoute(start, goal, mask);
                const int independent = IndependentDistance(start, goal, mask);
                assert(N::HasDiscoveredRoute(start, goal, mask) == (independent != 0));
                assert(route.Length == independent);
                if (route.Length > 0) {
                    assert(route.Length <= N::ZoneCount);
                    assert(route.Stops[0] == start);
                    assert(route.Stops[route.Length - 1] == goal);
                    std::uint32_t visited = 0;
                    for (int i = 0; i < route.Length; ++i) {
                        const int stop = route.Stops[i];
                        assert(N::ValidZone(stop));
                        assert((mask & (1u << stop)) != 0u);
                        assert((visited & (1u << stop)) == 0u);
                        visited |= 1u << stop;
                        if (i > 0) assert(N::Linked(route.Stops[i - 1], stop));
                    }
                }
                ++checkedPaths;
            }
        }
        const bool morvellaDiscovered = (mask & (1u << N::MorvellaProvince)) != 0u;
        const bool zDiscovered = (mask & (1u << N::ZProvince)) != 0u;
        const bool nyraDiscovered = (mask & (1u << N::NyraProvince)) != 0u;
        const bool morvellaReachable = IndependentDistance(0, N::MorvellaProvince, mask) != 0;
        const bool zReachable = IndependentDistance(0, N::ZProvince, mask) != 0;
        const bool nyraReachable = IndependentDistance(0, N::NyraProvince, mask) != 0;
        const auto morvella = N::MorvellaAccess(mask, false);
        const auto z = N::ZNorthernAccess(mask, false);
        const auto nyra = N::NyraNorthernAccess(mask, false);
        assert(morvella == (!morvellaDiscovered ? N::EMorvellaAccess::Undiscovered
                     : morvellaReachable ? N::EMorvellaAccess::Available
                     : N::EMorvellaAccess::RouteBlocked));
        assert(z == (!zDiscovered ? N::EDivineAccess::Undiscovered
                 : zReachable ? N::EDivineAccess::Available : N::EDivineAccess::RouteBlocked));
        assert(nyra == (!nyraDiscovered ? N::EDivineAccess::Undiscovered
                    : nyraReachable ? N::EDivineAccess::Available : N::EDivineAccess::RouteBlocked));
        assert(N::RouteToDivineSite(N::EDivineSite::Morvella,mask).Length
               == IndependentDistance(0,N::MorvellaProvince,mask));
        assert(N::RouteToDivineSite(N::EDivineSite::Z,mask).Length
               == IndependentDistance(0,N::ZProvince,mask));
        assert(N::RouteToDivineSite(N::EDivineSite::Nyra,mask).Length
               == IndependentDistance(0,N::NyraProvince,mask));
        assert(N::MorvellaAccess(mask,true) == N::EMorvellaAccess::Defeated);
        assert(N::ZNorthernAccess(mask,true) == N::EDivineAccess::Defeated);
        assert(N::NyraNorthernAccess(mask,true) == N::EDivineAccess::Defeated);
    }
    for (int i = 0; i < 3; ++i) {
        const auto site = N::DivineSite(static_cast<N::EDivineSite>(i));
        assert(site.EncounterId && site.Sanctum && N::ValidZone(site.Province));
        assert(std::strlen(site.EncounterId) >= 4 && std::strlen(site.Sanctum) >= 4);
    }
    assert(N::DivineSite(static_cast<N::EDivineSite>(255)).EncounterId == nullptr);
    assert(N::RouteToDivineSite(static_cast<N::EDivineSite>(255),all).Length == 0);
    assert(!N::ValidProvinceMask(all | (1u << N::ZoneCount)));
    assert(!N::HasDiscoveredRoute(0, 1, all | (1u << N::ZoneCount)));
    assert(N::DiscoveredRoute(0, 1, all | (1u << N::ZoneCount)).Length == 0);
    assert(N::MorvellaAccess(all | (1u << N::ZoneCount),true) == N::EMorvellaAccess::Invalid);
    assert(N::ZNorthernAccess(all | (1u << N::ZoneCount),true) == N::EDivineAccess::Invalid);
    assert(N::DiscoveredRoute(-1,0,all).Length == 0);
    assert(N::DiscoveredRoute(0,N::ZoneCount,all).Length == 0);
    std::cout << "NORTHERN DISCOVERY ROUTES PASS (" << checkedPaths
              << " pairs across all 512 discovery masks)" << std::endl;
    return 0;
}
