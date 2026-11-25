#pragma once
// Portable policy shared with the live UE5 startup path; no engine dependencies.
#include <cmath>
namespace MoonfallDungeonStartup {
inline constexpr int MaxSlots = 30;
inline constexpr int MaxBatch = 8;
constexpr int ClampBatch(int value) {
 return value < 1 ? 1 : value > MaxBatch ? MaxBatch : value;
}
inline double ClampBudget(double value) {
 return !std::isfinite(value) ? 4.0 : value < 0.5 ? 0.5 : value > 12.0 ? 12.0 : value;
}
inline bool CanProcess(int processed, int limit, double elapsedMs, double budgetMs) {
 if (processed < 0 || processed >= ClampBatch(limit)) return false;
 if (processed == 0) return true;
 return std::isfinite(elapsedMs) && elapsedMs >= 0.0 && elapsedMs < ClampBudget(budgetMs);
}
constexpr int ReadyCount(int count, bool wasReady, bool ready) {
 const int safe = count < 0 ? 0 : count > MaxSlots ? MaxSlots : count;
 const int updated = safe + (ready == wasReady ? 0 : ready ? 1 : -1);
 return updated < 0 ? 0 : updated > MaxSlots ? MaxSlots : updated;
}
constexpr bool CanQueueRetry(int slot, bool retryPass) {
 return !retryPass && slot >= 0 && slot < MaxSlots;
}
}
