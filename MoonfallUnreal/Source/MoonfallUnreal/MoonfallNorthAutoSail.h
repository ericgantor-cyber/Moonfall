#pragma once
// Deterministic, native-free fixed-duration automatic ferry crossing policy.
// No player helm controls, enemy encounters, ocean combat or manual steering.
// Reuse the same policy in UE5 and in portable C++17 tests.
namespace MoonfallNorth {
inline constexpr float DefaultAutoSailSeconds = 11.f;
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
// Short, accessible phases for audio, mist and sail cues. These are
// PRESENTATION phases only: no enemies, steering or encounter spawns.
enum class EAutoSailPhase : unsigned char {
 Invalid, Departing, OpenSea, Approaching, Arrived
};
constexpr EAutoSailPhase AutoSailPhaseAt(float ElapsedSeconds, float DurationSeconds) {
 if (!ValidAutoSailDuration(DurationSeconds) || !(ElapsedSeconds >= 0.f))
  return EAutoSailPhase::Invalid;
 if (ElapsedSeconds >= DurationSeconds) return EAutoSailPhase::Arrived;
 const float Progress = AutoSailProgress(ElapsedSeconds, DurationSeconds);
 if (Progress < 0.25f) return EAutoSailPhase::Departing;
 if (Progress < 0.75f) return EAutoSailPhase::OpenSea;
 return EAutoSailPhase::Approaching;
}
// Ceiling with no library dependencies; called only for bounded durations.
// A countdown showing 1 sec never displays 'arrived' before safe disembark.
constexpr int AutoSailSecondsRemaining(float ElapsedSeconds, float DurationSeconds) {
 if (!ValidAutoSailDuration(DurationSeconds) || !(ElapsedSeconds >= 0.f)
     || ElapsedSeconds >= DurationSeconds) return 0;
 const float Remaining = DurationSeconds - ElapsedSeconds;
 const int Whole = static_cast<int>(Remaining);
 return Whole + (Remaining > static_cast<float>(Whole) ? 1 : 0);
}
static_assert(AutoSailSecondsRemaining(0.f,11.f) == 11, "Starts at eleven");
static_assert(AutoSailSecondsRemaining(1.f,11.f) == 10, "Countdown updates per second");
static_assert(AutoSailSecondsRemaining(10.1f,11.f) == 1, "Last second is readable");
static_assert(AutoSailSecondsRemaining(11.f,11.f) == 0, "No early arrival");
static_assert(AutoSailPhaseAt(0.f,11.f) == EAutoSailPhase::Departing, "Depart cue");
static_assert(AutoSailPhaseAt(5.5f,11.f) == EAutoSailPhase::OpenSea, "Open sea cue");
static_assert(AutoSailPhaseAt(9.f,11.f) == EAutoSailPhase::Approaching, "Arrival cue");
static_assert(AutoSailPhaseAt(11.f,11.f) == EAutoSailPhase::Arrived, "Complete phase");
static_assert(ValidAutoSailDuration(DefaultAutoSailSeconds), "Auto crossing needs a bounded duration");
static_assert(!ValidAutoSailDuration(0.f), "Zero-time crossings are not animated");
static_assert(AutoSailProgress(-1.f,DefaultAutoSailSeconds) == 0.f, "Time cannot run backward");
static_assert(AutoSailProgress(5.5f,DefaultAutoSailSeconds) == 0.5f, "Midpoint should be half");
static_assert(AutoSailProgress(50.f,DefaultAutoSailSeconds) == 1.f, "Progress must clamp");
static_assert(AutoSailEasedProgress(5.5f,DefaultAutoSailSeconds) == 0.5f, "Ease midpoint");
} // namespace MoonfallNorth
