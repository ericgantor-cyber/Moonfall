#pragma once
// Bounded, actor-anchored combat floats. Engine-independent C++17 policy
// allows exhaustive regression checks without installing Unreal Engine.
// Only enemy actors own combat-result labels. The hero is never a label anchor.
namespace MoonfallCombatFloatLayout {
constexpr int HistoryPerEnemy = 4;
constexpr int MaxVisibleAttackOutcomes = 16;
constexpr int MaxVisibleDealtDamage = 32;
constexpr int MaxNearbyCombatActors = 64;
constexpr float LifetimeSeconds = 1.1f;
constexpr bool Active(float Age) {
 return Age >= 0.f && Age < LifetimeSeconds;
}
constexpr float Clamp01(float Value) {
 return Value < 0.f ? 0.f : Value > 1.f ? 1.f : Value;
}
constexpr float Opacity(float Age) {
 return Active(Age) ? Clamp01(1.f - Age / LifetimeSeconds) : 0.f;
}
constexpr float Rise(float Age, bool ReduceMotion) {
 return !Active(Age) || ReduceMotion ? 0.f : Age * 68.f;
}
// Fixed two-column, two-row lanes; no random or per-frame allocations.
constexpr float LaneX(int Slot) { return (Slot % 2 == 0) ? -150.f : 30.f; }
constexpr float LaneY(int Slot) { return (Slot / 2) * 34.f; }

// Collision avoidance is screen-space only. The original source actor
// remains the text anchor, with no gameplay hitboxes or camera movement.
struct FScreenRect { float X, Y, Width, Height; };
constexpr bool Intersects(const FScreenRect& A, const FScreenRect& B, float Gap = 7.f) {
 return A.X < B.X+B.Width+Gap && A.X+A.Width+Gap > B.X
     && A.Y < B.Y+B.Height+Gap && A.Y+A.Height+Gap > B.Y;
}
constexpr bool InViewport(const FScreenRect& R, float W, float H) {
 return W > 0.f && H > 0.f && R.Width > 0.f && R.Height > 0.f
     && R.X >= 8.f && R.Y >= 8.f
     && R.X+R.Width <= W-8.f && R.Y+R.Height <= H-8.f;
}
// At most eleven fixed nearby placements and sixteen comparisons each.
// The hero's screen region is protected, including when an enemy physically
// approaches the camera/player. No additional actor scan, heap or RNG.
constexpr bool TryPlace(float X, float Y, float Width, float Height,
                        float ViewWidth, float ViewHeight,
                        bool bHeroOnScreen, FScreenRect HeroExclusion,
                        const FScreenRect* Existing, int ExistingCount,
                        FScreenRect& Out) {
 if (ExistingCount < 0 || ExistingCount > MaxVisibleAttackOutcomes
     || (ExistingCount > 0 && Existing == nullptr)) return false;
 for (int Try = 0; Try < 11; ++Try) {
  const float Dx = (Try == 4 || Try == 6 || Try == 9)
    ? Width+16.f : (Try == 5 || Try == 7 || Try == 10)
    ? -(Width+16.f) : 0.f;
  const float Dy = Try == 1 || Try == 6 || Try == 7 ? -36.f
    : Try == 2 || Try == 9 || Try == 10 ? -72.f
    : Try == 3 ? -108.f : Try == 8 ? -144.f : 0.f;
  const FScreenRect Candidate{X+Dx, Y+Dy, Width, Height};
  if (!InViewport(Candidate,ViewWidth,ViewHeight)
      || (bHeroOnScreen && Intersects(Candidate,HeroExclusion))) continue;
  bool bOverlaps = false;
  for (int I = 0; I < ExistingCount; ++I) {
   if (Intersects(Candidate,Existing[I])) { bOverlaps=true; break; }
  }
  if (bOverlaps) continue;
  Out = Candidate;
  return true;
 }
 return false;
}
} // namespace MoonfallCombatFloatLayout
