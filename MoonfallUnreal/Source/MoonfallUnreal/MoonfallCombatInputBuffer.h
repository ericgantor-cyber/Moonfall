#pragma once
// Bounded, one-action melee input buffer. Engine-independent C++17 for
// executable policy tests. Never produces attacks: callers own modal checks,
// resource spending, recovery and hit resolution.
#include <cmath>

namespace MoonfallCombatInputBuffer {
inline constexpr float EarlyWindowSeconds = 0.18f;
inline constexpr float LateGraceSeconds = 0.12f;

struct FSingleSwing {
 float ExpiresAt = -1.f;
 float QueuedAt = -1.f;

 void Clear() noexcept { ExpiresAt = -1.f; QueuedAt = -1.f; }
 // A malformed half-populated slot still requires validation and cleanup.
 bool HasPending() const noexcept { return ExpiresAt != -1.f || QueuedAt != -1.f; }

 // Only a second deliberate press shortly before sword recovery can queue.
 // Repeated presses replace the same single slot, never stack free attacks.
 bool Queue(float Now, float NextReadyAt) noexcept {
  if (!std::isfinite(Now) || Now < 0.f || !std::isfinite(NextReadyAt)) {
   Clear();
   return false;
  }
  if (HasPending() && (!std::isfinite(QueuedAt) || !std::isfinite(ExpiresAt)
      || QueuedAt < 0.f || Now < QueuedAt || Now > ExpiresAt)) Clear();
  const float Remaining = NextReadyAt - Now;
  if (!std::isfinite(Remaining) || Remaining <= 0.f
      || Remaining > EarlyWindowSeconds) return false;
  const float Deadline = NextReadyAt + LateGraceSeconds;
  if (!std::isfinite(Deadline)) {
   Clear();
   return false;
  }
  ExpiresAt = Deadline;
  QueuedAt = Now;
  return true;
 }

 // Pending input is consumed at most once. Unsafe state or an expired
 // deadline discards it instead of leaking an attack through a closed menu.
 bool Consume(float Now, float NextReadyAt, bool bCanAttack) noexcept {
  if (!HasPending()) return false;
  if (!bCanAttack || !std::isfinite(Now) || !std::isfinite(NextReadyAt)
      || !std::isfinite(ExpiresAt) || !std::isfinite(QueuedAt)
      || QueuedAt < 0.f || Now < QueuedAt || Now > ExpiresAt) {
   Clear();
   return false;
  }
  if (Now < NextReadyAt) return false;
  Clear();
  return true;
 }
};
} // namespace MoonfallCombatInputBuffer
