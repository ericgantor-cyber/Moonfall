#pragma once
// Portable set threshold selection. No Unreal dependencies so native C++
// tests can exercise the exact logic used by the inventory HUD.
namespace MoonfallSetBonusThresholds {
constexpr int Lost(int before, int after) {
 return before >= 3 && after < 3 ? 3
      : before >= 2 && after < 2 ? 2 : 0;
}
constexpr int Gained(int before, int after) {
 return before < 3 && after >= 3 ? 3
      : before < 2 && after >= 2 ? 2 : 0;
}
}
