// Engine-independent C++17 acceptance tests for nine-province greybox plans.
// These are specifications, not spawned world meshes, collision or navmesh.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthContinentLayout.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthLandmarks.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthZoneGreybox.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>

namespace N = MoonfallNorth;
static_assert(N::ZoneCount == 9);
static_assert(N::FullyConnected() && N::SymmetricLinks());
static_assert(N::ValidContinentCenters());
static_assert(N::RoadEdgeCount() > 8 && N::RoadEdgeCount() <= 20);
static_assert(N::ValidLandmarks() && N::LandmarkCount == 18);
static_assert(N::PlanZoneGreybox(-1).Valid == false);
static_assert(N::PlanZoneGreybox(N::ZoneCount).Valid == false);

int main() {
    int observedEdges = 0;
    std::set<std::uint64_t> distinctCenters;
    for (int province = 0; province < N::ZoneCount; ++province) {
        const auto& center = N::ContinentCenters[province];
        // Key with a signed-safe 64-bit combination, not an overflow-prone int.
        const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(center.Xcm)) << 32)
                               | static_cast<std::uint32_t>(center.Ycm);
        assert(distinctCenters.insert(key).second);

        const auto plan = N::PlanZoneGreybox(province);
        assert(N::ValidZoneGreybox(plan));
        assert(plan.Valid && plan.Province == province && plan.Count == 5);
        const auto& hub = plan.Pieces[0];
        const auto& northRoad = plan.Pieces[1];
        const auto& dungeon = plan.Pieces[2];
        const auto& southRoad = plan.Pieces[3];
        const auto& entrance = plan.Pieces[4];
        assert(hub.Kind == N::EZonePiece::HubFloor);
        assert(dungeon.Kind == N::EZonePiece::DungeonFloor);
        assert(entrance.Kind == N::EZonePiece::EntranceFloor);
        assert(hub.Ycm + hub.HalfYcm == northRoad.Ycm - northRoad.HalfYcm);
        assert(northRoad.Ycm + northRoad.HalfYcm == dungeon.Ycm - dungeon.HalfYcm);
        assert(southRoad.Ycm + southRoad.HalfYcm == hub.Ycm - hub.HalfYcm);
        assert(entrance.Ycm + entrance.HalfYcm == southRoad.Ycm - southRoad.HalfYcm);
        assert(hub.HalfXcm >= northRoad.HalfXcm);
        assert(dungeon.HalfXcm >= northRoad.HalfXcm);

        for (int i = 0; i < N::LandmarksPerProvince; ++i) {
            const auto& landmark = N::Landmarks[province*N::LandmarksPerProvince+i];
            assert(landmark.Province == province);
            assert(landmark.HalfWidthCm >= 200 && landmark.HalfWidthCm <= 600);
            assert(landmark.HeightCm >= 300 && landmark.HeightCm <= 2000);
            const int offsetX = landmark.OffsetXcm < 0 ? -landmark.OffsetXcm : landmark.OffsetXcm;
            const int offsetY = landmark.OffsetYcm < 0 ? -landmark.OffsetYcm : landmark.OffsetYcm;
            assert(offsetX >= N::ProvincePadHalfSizeCm + landmark.HalfWidthCm);
            assert(offsetY >= N::ProvincePadHalfSizeCm + landmark.HalfWidthCm);
        }

        for (int other = province+1; other < N::ZoneCount; ++other) {
            const auto& dest = N::ContinentCenters[other];
            const std::int64_t dx = static_cast<std::int64_t>(center.Xcm)-dest.Xcm;
            const std::int64_t dy = static_cast<std::int64_t>(center.Ycm)-dest.Ycm;
            const std::int64_t distanceSquared = dx*dx+dy*dy;
            assert(distanceSquared >= 10000LL*10000LL);
            if (N::Linked(province,other)) {
                assert(distanceSquared <= 20000LL*20000LL);
                ++observedEdges;
            }
        }
    }
    assert(distinctCenters.size() == 9u);
    assert(observedEdges == N::RoadEdgeCount());

    auto broken = N::PlanZoneGreybox(0);
    broken.Pieces[1].Ycm += 1; // One-centimeter gap is not acceptable.
    assert(!N::ValidZoneGreybox(broken));
    broken = N::PlanZoneGreybox(0);
    broken.Pieces[4].HalfXcm = 0;
    assert(!N::ValidZoneGreybox(broken));
    broken = N::PlanZoneGreybox(0);
    broken.Count = 0;
    assert(!N::ValidZoneGreybox(broken));
    assert(!N::ValidZoneGreybox(N::PlanZoneGreybox(-1)));
    assert(!N::ValidZoneGreybox(N::PlanZoneGreybox(9)));

    std::cout << "NORTHERN EXPANSE GEOMETRY POLICY PASS: 9 hubs, "
              << observedEdges << " road connections, 18 landmarks" << std::endl;
    return 0;
}
