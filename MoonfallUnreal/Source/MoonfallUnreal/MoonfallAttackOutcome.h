#pragma once
// A read-only result label for the actual melee resolution, separate from
// the attack's scheduled impact clock. Engine-free C++17 testable policy.
// The caller first verifies finite hero-health values, and supplies positive
// measured HP loss after applying the original damage/guard/parry pipeline.
namespace MoonfallAttackOutcome {
enum class EResult : unsigned char { Miss, Parry, Dodge, Hit, Guard };
constexpr EResult Classify(bool ReachedAndUnblocked, bool Parried,
                           bool PerfectDodged, float HealthLost) {
 if (!ReachedAndUnblocked) return EResult::Miss;
 if (Parried) return EResult::Parry;
 if (PerfectDodged) return EResult::Dodge;
 if (HealthLost > 0.f) return EResult::Hit;
 return EResult::Guard;
}
constexpr bool MayPlayHitSound(EResult Result) { return Result == EResult::Hit; }
constexpr bool MayPlayParrySound(EResult Result) { return Result == EResult::Parry; }
} // namespace MoonfallAttackOutcome
