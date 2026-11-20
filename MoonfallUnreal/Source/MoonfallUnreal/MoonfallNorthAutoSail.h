#pragma once
// Deterministic, native-free fixed-duration automatic ferry crossing policy.
// No player helm controls, enemy encounters, ocean combat or manual steering.
// Reuse the same policy in UE5 and in portable C++17 tests.
namespace MoonfallNorth {
inline constexpr float DefaultAutoSailSeconds = 8.f;
inline constexpr float AutoSailUpdateSeconds = 0.1f;
constexpr bool ValidAutoSailDuration(float Seconds) {
 return Seconds >= 4.f && Seconds <= 30.f;
}
constexpr float AutoSailProgress(float ElapsedSeconds, float DurationSeconds) {
 if (!ValidAutoSailDuration(DurationSeconds)) return 0.f;
 if (!(ElapsedSeconds > 0.f)) return 0.f; // NaN and negative fail closed.
 if (ElapsedSeconds >= DurationSeconds) return 1.f;
 return ElapsedSeconds / DurationSeconds;
}
// Smooth departure and slowing at the receiving dock without overshooting.
constexpr float AutoSailEasedProgress(float ElapsedSeconds, float DurationSeconds) {
 const float T = AutoSailProgress(ElapsedSeconds, DurationSeconds);
 return T * T * (3.f - 2.f * T);
}
constexpr bool AutoSailShouldArrive(float ElapsedSeconds, float DurationSeconds) {
 return ValidAutoSailDuration(DurationSeconds)
     && ElapsedSeconds >= DurationSeconds;
}
static_assert(ValidAutoSailDuration(DefaultAutoSailSeconds), "Auto crossing needs a bounded duration");
static_assert(!ValidAutoSailDuration(0.f), "Zero-time crossings are not animated");
static_assert(AutoSailProgress(-1.f,DefaultAutoSailSeconds) == 0.f, "Time cannot run backward");
static_assert(AutoSailProgress(4.f,DefaultAutoSailSeconds) == 0.5f, "Midpoint should be half");
static_assert(AutoSailProgress(50.f,DefaultAutoSailSeconds) == 1.f, "Progress must clamp");
static_assert(AutoSailEasedProgress(4.f,DefaultAutoSailSeconds) == 0.5f, "Ease midpoint");
} // namespace MoonfallNorth
