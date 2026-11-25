#pragma once
// Native-free, allocation-free combat camera feedback for confirmed offensive
// hits. It reuses Moonfall's existing bounded spring-arm impulse, never pauses
// the world, creates a camera actor, or places text over the player.
#include <cmath>

namespace MoonfallImpactFeedback {
inline constexpr float MaxImpulse = 24.f;
inline constexpr float MinHitGapSeconds = 0.09f;
struct FResult {
 float Offset;
 float LastAt;
 bool Triggered;
};
inline float Clamp(float Value, float Low, float High) {
 return Value < Low ? Low : Value > High ? High : Value;
}
inline FResult ConfirmedHit(float CurrentOffset, float LastHitAt,
 float Now, float DealtDamage, int ComboStage, bool bBoss, bool bLethal,
 bool bReduceMotion, bool bBlockedByMenu) {
 const float SafeOffset = std::isfinite(CurrentOffset)
     ? Clamp(CurrentOffset, 0.f, MaxImpulse) : 0.f;
 const float SafeLast = std::isfinite(LastHitAt) ? LastHitAt : -100.f;
 if (bReduceMotion) return {0.f, SafeLast, false};
 if (bBlockedByMenu || !std::isfinite(Now) || !std::isfinite(DealtDamage)
     || DealtDamage <= 0.f || ComboStage < 1)
  return {SafeOffset, SafeLast, false};
 // Rapid dual casts must not accumulate an unbounded impulse. A world
 // clock reset is legitimate after respawn and must not lock out feedback.
 if (Now >= SafeLast && Now - SafeLast < MinHitGapSeconds)
  return {SafeOffset, SafeLast, false};
 const int Stage = ComboStage > 12 ? 12 : ComboStage;
 const bool bMilestone = Stage == 4 || Stage == 8 || Stage == 12;
 const float Strength = Clamp(
   2.1f + Clamp(DealtDamage, 0.f, 120.f) * 0.035f
   + Stage * 0.25f + (bMilestone ? 1.7f : 0.f)
   + (bBoss ? 1.2f : 0.f) + (bLethal ? 2.0f : 0.f),
   0.f, 10.f);
 return {Clamp(SafeOffset + Strength, 0.f, MaxImpulse), Now, true};
}
static_assert(MaxImpulse == 24.f, "Respect existing spring-arm clamp");
static_assert(MinHitGapSeconds >= 0.05f, "Avoid rapid camera jitter");
} // namespace MoonfallImpactFeedback
