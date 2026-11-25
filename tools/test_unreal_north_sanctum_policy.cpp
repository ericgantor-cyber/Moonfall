// Standalone C++17 contracts for future Northern Expanse divine sanctum plans.
// Planning data only: no UE actor placement, collision/navmesh, saves, or boss spawning.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthSanctumLayouts.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthSanctumGeometry.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>

namespace North = MoonfallNorth;
static_assert(North::ValidAllSanctumLayouts());
static_assert(North::SafeSanctumPlan(North::PlanSanctum(North::EDivineSite::Morvella)));
static_assert(North::SafeSanctumPlan(North::PlanSanctum(North::EDivineSite::Z)));
static_assert(North::SafeSanctumPlan(North::PlanSanctum(North::EDivineSite::Nyra)));
static_assert(!North::PlanSanctum(static_cast<North::EDivineSite>(-1)).Valid);
static_assert(!North::PlanSanctum(static_cast<North::EDivineSite>(3)).Valid);

int main() {
    std::set<int> provinces;
    for (int id = 0; id < 3; ++id) {
        const auto site = static_cast<North::EDivineSite>(id);
        const auto& layout = North::SanctumLayouts[id];
        const auto metadata = North::DivineSite(site);
        assert(static_cast<int>(layout.Site) == id);
        assert(metadata.EncounterId != nullptr && metadata.Sanctum != nullptr);
        assert(std::strlen(metadata.EncounterId) >= 4);
        assert(std::strlen(metadata.Sanctum) >= 4);
        assert(layout.Province == metadata.Province);
        assert(North::ValidZone(layout.Province));
        assert(provinces.insert(layout.Province).second);
        assert(North::ValidSanctumLayout(layout));
        assert(layout.HasSafeReturnPath);
        assert(layout.EntranceWidthCm >= 400);
        assert(layout.ArenaRadiusCm >= 1000);
        assert(layout.CoverCount >= 0 && layout.CoverCount <= 8);

        const auto plan = North::PlanSanctum(site);
        assert(plan.Valid && plan.Count == 7);
        assert(North::SafeSanctumPlan(plan));
        const auto& approach = plan.Parts[0];
        const auto& arena = plan.Parts[1];
        const auto& left = plan.Parts[2];
        const auto& right = plan.Parts[3];
        const auto& rear = plan.Parts[4];
        const auto& boss = plan.Parts[5];
        const auto& entrance = plan.Parts[6];
        assert(approach.Feature == North::EFeature::ApproachFloor);
        assert(arena.Feature == North::EFeature::ArenaFloor);
        assert(left.Feature == North::EFeature::SideWallLeft);
        assert(right.Feature == North::EFeature::SideWallRight);
        assert(rear.Feature == North::EFeature::RearWall);
        assert(boss.Feature == North::EFeature::BossMarker);
        assert(entrance.Feature == North::EFeature::EntranceMarker);
        assert(approach.CenterXcm == 0 && arena.CenterXcm == 0);
        assert(approach.CenterYcm - approach.HalfYcm == 0);
        assert(approach.CenterYcm + approach.HalfYcm
             == arena.CenterYcm - arena.HalfYcm);
        assert(approach.HalfXcm * 2 == layout.EntranceWidthCm);
        assert(arena.HalfXcm == layout.ArenaRadiusCm);
        assert(arena.HalfYcm == layout.ArenaRadiusCm);
        assert(arena.CenterYcm == layout.ApproachLengthCm + layout.ArenaRadiusCm);
        assert(left.CenterXcm + left.HalfXcm == -layout.ArenaRadiusCm);
        assert(right.CenterXcm - right.HalfXcm == layout.ArenaRadiusCm);
        assert(rear.CenterYcm - rear.HalfYcm
             == arena.CenterYcm + arena.HalfYcm);
        assert(!boss.Solid && !entrance.Solid);
        assert(boss.CenterXcm == 0 && entrance.CenterXcm == 0);
        assert(entrance.CenterYcm == 0);
        const auto toSite = North::RouteToDivineSite(site, North::AllProvinceBits);
        assert(toSite.Length > 0);
        assert(toSite.Stops[toSite.Length-1] == layout.Province);
        assert(North::RouteToDivineSite(site, 1u).Length == 0);

        auto bad = plan;
        bad.Parts[0].CenterYcm += 1; // A one-centimetre gap is rejected.
        assert(!North::SafeSanctumPlan(bad));
        bad = plan;
        bad.Parts[2].Solid = false;
        assert(!North::SafeSanctumPlan(bad));
        bad = plan;
        bad.Parts[5].Solid = true; // Boss markers must be nonblocking.
        assert(!North::SafeSanctumPlan(bad));
        bad = plan;
        bad.Parts[0].HalfXcm = 0;
        assert(!North::SafeSanctumPlan(bad));
        bad = plan;
        bad.Count = 6;
        assert(!North::SafeSanctumPlan(bad));

        auto invalid = layout;
        invalid.Province = -1;
        assert(!North::ValidSanctumLayout(invalid));
        invalid = layout;
        invalid.EntranceWidthCm = 399;
        assert(!North::ValidSanctumLayout(invalid));
        invalid = layout;
        invalid.HasSafeReturnPath = false;
        assert(!North::ValidSanctumLayout(invalid));
        invalid = layout;
        invalid.ArenaRadiusCm = 999;
        assert(!North::ValidSanctumLayout(invalid));
    }
    assert(provinces.size() == 3u);
    assert(!North::PlanSanctum(static_cast<North::EDivineSite>(-1)).Valid);
    assert(!North::PlanSanctum(static_cast<North::EDivineSite>(3)).Valid);
    assert(!North::SafeSanctumPlan(North::PlanSanctum(static_cast<North::EDivineSite>(3))));
    std::cout << "NORTHERN SANCTUM POLICY PASS (3 divine sites, 21 planned primitives)" << std::endl;
    return 0;
}
