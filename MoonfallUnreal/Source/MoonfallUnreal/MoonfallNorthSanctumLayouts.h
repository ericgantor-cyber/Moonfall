#pragma once
// Deterministic layout specifications for three FUTURE divine sanctums.
// Distances are local centimeters from a hypothetical zone-level anchor.
// These are planning coordinates, NOT world transforms or spawned geometry.
// No UE includes, art substitutions, save writes, or duplicated gods.
#include "MoonfallNorthDivineRoutes.h"
namespace MoonfallNorth {
struct FSanctumLayout {
 EDivineSite Site;
 int Province;
 int ApproachLengthCm;    // From entrance to arena, positive walkable route.
 int ArenaRadiusCm;       // Clear central combat radius.
 int EntranceWidthCm;     // Wide, unobstructed main access.
 int CoverCount;          // Fixed environment budget, not spawning instructions.
 bool HasSafeReturnPath;  // Arena may never trap the player.
};
inline constexpr FSanctumLayout SanctumLayouts[3] = {
 {EDivineSite::Morvella, MorvellaProvince, 3200, 1250, 500, 6, true},
 {EDivineSite::Z, ZProvince, 2800, 1400, 500, 4, true},
 {EDivineSite::Nyra, NyraProvince, 3600, 1500, 550, 5, true}
};
constexpr bool ValidSanctumLayout(const FSanctumLayout& Layout) {
 const FDivineSite Site = DivineSite(Layout.Site);
 return Site.EncounterId != nullptr && Site.Province == Layout.Province
  && ValidZone(Layout.Province)
  && Layout.ApproachLengthCm >= 1200 && Layout.ApproachLengthCm <= 5000
  && Layout.ArenaRadiusCm >= 1000 && Layout.ArenaRadiusCm <= 2000
  && Layout.EntranceWidthCm >= 400 && Layout.EntranceWidthCm <= 900
  && Layout.CoverCount >= 0 && Layout.CoverCount <= 8
  && Layout.HasSafeReturnPath;
}
constexpr bool ValidAllSanctumLayouts() {
 for (int Index=0; Index<3; ++Index) {
  if (!ValidSanctumLayout(SanctumLayouts[Index])) return false;
  if (static_cast<int>(SanctumLayouts[Index].Site) != Index) return false;
 }
 return SanctumLayouts[0].Province != SanctumLayouts[1].Province
  && SanctumLayouts[0].Province != SanctumLayouts[2].Province
  && SanctumLayouts[1].Province != SanctumLayouts[2].Province;
}
static_assert(ValidAllSanctumLayouts(),
 "All sanctums require distinct provinces and accessible safe layouts");
} // namespace MoonfallNorth
