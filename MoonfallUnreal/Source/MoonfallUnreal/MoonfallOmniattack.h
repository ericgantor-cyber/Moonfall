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
inline void Reset(FFlow& Flow) noexcept { Flow = FFlow{}; }
inline int Next(FFlow& Flow, unsigned Roll) noexcept {
 if (Flow.Remaining == 0) Flow.Remaining = 0x0f;
 std::uint8_t Choices = Flow.Remaining;
 // The previous cycle's last direction cannot immediately repeat.
 if (Flow.Remaining == 0x0f && Flow.Last >= 0 && Flow.Last < 4)
  Choices = static_cast<std::uint8_t>(Choices & ~(1u << Flow.Last));
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
