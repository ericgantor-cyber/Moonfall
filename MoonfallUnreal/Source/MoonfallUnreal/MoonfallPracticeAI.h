#pragma once
// Only tagged Firstlight practice sentries use this predictable AI policy.
// Engine-independent C++17 enables executable tuning and safety tests.
#include <cmath>
namespace MoonfallPracticeAI {
inline constexpr float EngageRange = 450.f;
inline constexpr float ReapproachDelay = 0.6f;
inline constexpr float MinimumWindup = 1.10f;
inline constexpr float MaximumPracticeDamage = 1.5f;
inline constexpr float HeroHealthFloor = 1.f;
inline constexpr float IdleTickSeconds = 0.4f;
constexpr int PatternFor(int Sequence) noexcept {
 return (Sequence & 1) ? 0 : 2;
}
inline float ReadableWindup(float Normal, int Sequence) noexcept {
 const float TeachingMinimum = MinimumWindup +
     (PatternFor(Sequence) == 2 ? 0.25f : 0.f);
 return std::isfinite(Normal) && Normal > TeachingMinimum
     ? Normal : TeachingMinimum;
}
inline float NonLethalDamage(float Normal, float HeroHealth) noexcept {
 if (!std::isfinite(Normal) || !std::isfinite(HeroHealth) ||
     Normal <= 0.f || HeroHealth <= HeroHealthFloor) return 0.f;
 return std::fmin(std::fmin(Normal, MaximumPracticeDamage),
     HeroHealth - HeroHealthFloor);
}
} // namespace MoonfallPracticeAI
