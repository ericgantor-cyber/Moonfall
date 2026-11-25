// Standalone C++17 acceptance test for the data-only Northern Expanse catalog.
// This does not require Unreal Engine, create actors or alter any save files.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthZones.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace N = MoonfallNorth;
static_assert(N::ZoneCount == 9 && N::FirstFutureRegionId == 6);
static_assert(N::FullyConnected() && N::SymmetricLinks());
static_assert(N::DivineHomesValid());
static_assert(N::ShortestRoute(-1, 0).Length == 0);
static_assert(N::ShortestRoute(0, N::ZoneCount).Length == 0);

int main() {
    assert(!N::ValidZone(-1) && !N::ValidZone(N::ZoneCount));
    assert(N::ValidZone(0) && N::ValidZone(N::ZoneCount-1));
    for (int a = 0; a < N::ZoneCount; ++a) {
        const auto& zone = N::Zones[a];
        assert(std::strlen(zone.Id) >= 5);
        assert(std::strlen(zone.Name) >= 5);
        assert(std::strlen(zone.Hub) >= 5);
        assert(std::strlen(zone.Dungeon) >= 5);
        assert(std::strlen(zone.Boss) >= 5);
        assert(zone.SuggestedLevel >= 25 && zone.SuggestedLevel <= 50);
        assert(zone.NeighborMask != 0u);
        assert(!N::Linked(a,a));
        for (int b = 0; b < N::ZoneCount; ++b) {
            if (a != b) {
                assert(std::strcmp(zone.Id, N::Zones[b].Id) != 0);
                assert(std::strcmp(zone.Name, N::Zones[b].Name) != 0);
            }
            assert(N::Linked(a,b) == N::Linked(b,a));
            const N::FRoute path = N::ShortestRoute(a,b);
            assert(path.Length >= 1 && path.Length <= N::ZoneCount);
            assert(path.Stops[0] == a);
            assert(path.Stops[path.Length-1] == b);
            for (int stop = 0; stop < path.Length-1; ++stop)
                assert(N::Linked(path.Stops[stop],path.Stops[stop+1]));
            const N::FRoute reverse = N::ShortestRoute(b,a);
            assert(path.Length == reverse.Length);
        }
    }
    assert(N::MorvellaProvince != N::ZProvince);
    assert(N::ZProvince != N::NyraProvince);
    assert(N::NyraProvince != N::MorvellaProvince);
    std::cout << "NORTHERN CATALOG DATA-ONLY PASS (81 routes)" << std::endl;
    return 0;
}
