#pragma once
// Stateless tactical rule shared by native Unreal logic and C++17 tests.
// A parry does not grant hidden accuracy, extra damage, or an extra attack.
namespace MoonfallEliteTactics {
constexpr float ParryMemorySeconds = 4.f;
constexpr float ParryResponseCooldownSeconds = 8.f;
constexpr float CounterSweepWindup = 0.82f;
constexpr int CounterSweepPattern = 2;
constexpr bool CanArm(bool Elite, bool Boss, bool Practice, bool Alive,
                      bool PostureBroken, bool CooldownReady) {
 return Elite && !Boss && !Practice && Alive && !PostureBroken && CooldownReady;
}
constexpr int SelectPattern(int NormalPattern, bool Elite, bool Boss,
                            bool ArmedAndUnexpired) {
 return Elite && !Boss && ArmedAndUnexpired ? CounterSweepPattern : NormalPattern;
}
} // namespace MoonfallEliteTactics
