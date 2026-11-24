#pragma once
// C++17 road-landmark placement. Keep decorative silhouettes in the
// 450-unit open gaps between 900-unit-wide dungeon chamber footprints,
// never overlapping the 30 contiguous dungeons or walkable public road.
namespace MoonfallRoadLandmarks {
struct FPlacement { float X; float Y; float Height; };
inline constexpr FPlacement Placements[8] = {
 {-3375.f,-4900.f,1.f}, {-2025.f,-5100.f,1.2f},
 {-675.f,-4750.f,.7f}, {675.f,-4800.f,.85f},
 {2025.f,-5200.f,1.1f}, {3375.f,-4900.f,1.f},
 {-2025.f,-6400.f,.75f}, {2025.f,-6500.f,.9f}
};
constexpr float Abs(float x) { return x < 0.f ? -x : x; }
constexpr bool ClearDungeonFootprint(float offsetX) {
 // Five lanes per region. A 2.7x scaled cube has 135 units half width,
 // while 900-unit chambers have a 450-unit half width. Keep 15 extra.
 for (int i=0; i<5; ++i) {
  const float center=(static_cast<float>(i)-2.f)*1350.f;
  if (Abs(offsetX-center)<600.f) return false;
 }
 return true;
}
constexpr bool AllLandmarksSafe() {
 for (const FPlacement& p:Placements) {
  if (!ClearDungeonFootprint(p.X) || p.Y > -4000.f || p.Y < -7000.f
      || p.Height <= 0.f) return false;
 }
 return true;
}
static_assert(AllLandmarksSafe(),
 "World landmarks must stay in the clear spaces between dungeon chambers.");
} // namespace MoonfallRoadLandmarks
