#pragma once
// UE5 Omniattack direction policy. Engine-independent C++17 so CI can test
// swing sequences without pretending to run an Unreal gameplay build.
#include <cstdint>

namespace MoonfallOmniattack {
struct FFlow {
 std::uint8_t Remaining = 0x0f;
 int Last = -1;
 int Step = 0;
};
// A bounded profile for each automatic swing. Index values remain stable for
// existing Blueprint OnMeleeSwing listeners and animation assets.
struct FAttackProfile {
 const char* Name;
 float DamageScale, RecoveryScale, RangeScale, Radius, PostureScale;
};
inline constexpr FAttackProfile Profiles[4] = {
 {"OVERHEAD", 1.15f, 1.12f, 0.90f, 42.f, 0.30f},
 {"RIGHT SLASH", 1.00f, 0.94f, 1.00f, 70.f, 0.18f},
 {"LOW SWEEP", 0.92f, 1.06f, 0.95f, 78.f, 0.48f},
 {"LEFT CUT", 1.06f, 0.88f, 1.14f, 32.f, 0.12f}
};
inline constexpr const FAttackProfile& Profile(int Angle) noexcept {
 return Profiles[Angle >= 0 && Angle < 4 ? Angle : 1];
}
// Twelve equally likely rolls divide evenly into 1, 2, 3 or 4 choices.
inline constexpr int FairRollCount = 12;
inline void Reset(FFlow& Flow) noexcept { Flow = FFlow{}; }
inline constexpr bool ValidAngle(int Angle) noexcept { return Angle >= 0 && Angle < 4; }
inline constexpr int AnticipatedGuard(int LastAttack) noexcept {
 return ValidAngle(LastAttack) ? (LastAttack + 2) % 4 : -1;
}
inline constexpr float GuardDamageScale(int Attack, int Guard, bool Riposte) noexcept {
 return !Riposte && ValidAngle(Attack) && Attack == Guard ? 0.65f : 1.f;
}
inline int Next(FFlow& Flow, unsigned Roll, int Avoid = -1) noexcept {
 // Reject impossible bit masks and step counts rather than entering a stuck bag.
 if ((Flow.Remaining & 0xf0u) != 0 || Flow.Step < 0 || Flow.Step > 4
     || Flow.Last < -1 || Flow.Last > 3) Reset(Flow);
 if (Flow.Remaining == 0) Flow.Remaining = 0x0f;
 std::uint8_t Choices = Flow.Remaining;
 // The previous cycle's last direction cannot immediately repeat.
 if (Flow.Remaining == 0x0f && Flow.Last >= 0 && Flow.Last < 4)
  Choices = static_cast<std::uint8_t>(Choices & ~(1u << Flow.Last));
 // Prefer exposed angles without discarding the final unused angle.
 if (ValidAngle(Avoid)) {
  const auto Exposed = static_cast<std::uint8_t>(Choices & ~(1u << Avoid));
  if (Exposed != 0) Choices = Exposed;
 }
 unsigned Count = 0;
 for (int Angle = 0; Angle < 4; ++Angle)
  if ((Choices & (1u << Angle)) != 0) ++Count;
 // A valid nonempty four-angle bag always has at least three choices
 // at the start of a cycle, and at least one choice within the cycle.
 if (Count == 0) { Reset(Flow); return 0; }
 unsigned Choice = Roll % Count;
 int Selected = 0;
 for (int Angle = 0; Angle < 4; ++Angle) {
  if ((Choices & (1u << Angle)) == 0) continue;
  if (Choice == 0) { Selected = Angle; break; }
  --Choice;
 }
 Flow.Remaining = static_cast<std::uint8_t>(Flow.Remaining & ~(1u << Selected));
 Flow.Last = Selected;
 Flow.Step = Flow.Step == 4 ? 1 : Flow.Step + 1;
 return Selected;
}
} // namespace MoonfallOmniattack
