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
// Six distinct silhouettes, using the same fixed cube mesh and gap positions.
// Scale values stay within the widest existing 270-unit landmark footprint.
struct FArtProfile { float CrownWidth; float CrownHeight; float CrownYaw; float TipWidth; float TipHeight; };
inline constexpr FArtProfile ArtProfiles[6] = {
 {.70f,.55f,0.f,.40f,.75f},   // Ashen Wilds: fractured watchtower
 {1.40f,.75f,45.f,.65f,1.2f}, // Moonveil Forest: tiered spire
 {2.60f,.45f,0.f,1.25f,.35f}, // Hollow Depths: massive lintel
 {1.75f,.35f,0.f,.85f,.65f}, // Iron Foundry: industrial stack
 {1.15f,.85f,45.f,.48f,1.0f}, // Frostbound Reach: crystal-like peak
 {.85f,.50f,45.f,.38f,.90f}   // Eclipse Wastes: broken obelisk
};
constexpr bool ArtProfilesSafe() {
 for (const FArtProfile& p:ArtProfiles) {
  if (p.CrownWidth<=0.f || p.TipWidth<=0.f || p.CrownHeight<=0.f
      || p.TipHeight<=0.f || p.CrownHeight>1.2f || p.TipHeight>1.2f) return false;
  // Rotated cubes occupy at most sqrt(2) times their unrotated footprint.
  if (p.CrownWidth*(p.CrownYaw==0.f?1.f:1.415f)>2.7f || p.TipWidth>2.7f) return false;
 }
 return true;
}
static_assert(ArtProfilesSafe(), "Regional skyline art must stay in existing landmark gaps.");
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
