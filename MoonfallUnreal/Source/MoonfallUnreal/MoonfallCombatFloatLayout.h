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
} // namespace MoonfallCombatFloatLayout
