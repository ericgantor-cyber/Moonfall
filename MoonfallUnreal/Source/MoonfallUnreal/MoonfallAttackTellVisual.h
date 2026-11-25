#pragma once
// Render-only profiles for optional attack tells. Pure C++17 so source CI
// checks the exact sizes/motion used in Unreal without needing the engine.
namespace MoonfallAttackTellVisual {
enum class EMove : unsigned char { Quick, Heavy, Sweep, Crush, CounterSweep };
struct FProfile {
 float Width, Depth, Height;
 float YawDegrees;
};
constexpr FProfile Profile(EMove Move) {
 switch (Move) {
  case EMove::Heavy: return {0.27f, 0.24f, 0.44f, -11.f};
  case EMove::Sweep: return {0.16f, 1.18f, 0.085f, 24.f};
  case EMove::Crush: return {0.54f, 0.46f, 0.27f, -14.f};
  case EMove::CounterSweep: return {0.18f, 1.44f, 0.095f, 32.f};
  default: return {0.29f, 0.16f, 0.075f, 8.f};
 }
}
constexpr EMove Select(bool Counter, float DamageMultiplier, float RangeMultiplier) {
 return Counter ? EMove::CounterSweep
  : DamageMultiplier > 2.f ? EMove::Crush
  : DamageMultiplier > 1.5f ? EMove::Heavy
  : RangeMultiplier > 1.f ? EMove::Sweep : EMove::Quick;
}
constexpr float Clamp01(float X) {
 return X < 0.f ? 0.f : X > 1.f ? 1.f : X;
}
// Callers reject NaN/Inf via Unreal FMath::IsFinite before passing values.
constexpr float Progress(float Remaining, float Duration, bool ReducedMotion) {
 return ReducedMotion ? 1.f : Duration > 0.f
  ? Clamp01(1.f - Remaining / Duration) : 0.f;
}
constexpr float Growth(float ProgressValue) {
 return 0.58f + 0.42f * Clamp01(ProgressValue);
}
} // namespace MoonfallAttackTellVisual
