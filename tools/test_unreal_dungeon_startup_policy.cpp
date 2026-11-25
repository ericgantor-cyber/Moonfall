#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallDungeonStartupPolicy.h"
#include <cassert>
#include <limits>
#include <iostream>
namespace P = MoonfallDungeonStartup;
int main() {
 int cases = 0;
 for (int slots = -2; slots <= 32; ++slots) {
  for (int count = -2; count <= 32; ++count) {
   for (int was = 0; was <= 1; ++was) for (int now = 0; now <= 1; ++now) {
    const int result = P::ReadyCount(count, was != 0, now != 0);
    assert(result >= 0 && result <= P::MaxSlots);
    if (count >= 0 && count <= 30 && was == now) assert(result == count);
    if (count >= 1 && count <= 29) assert(result == count + now - was);
    assert(!P::CanQueueRetry(slots, true));
    assert(P::CanQueueRetry(slots, false) == (slots >= 0 && slots < 30));
    ++cases;
   }
  }
 }
 for (int cap = -2; cap <= 12; ++cap)
  for (int processed = -1; processed <= 9; ++processed)
   for (int elapsed = -1; elapsed <= 13; ++elapsed) {
    const int clamped = cap < 1 ? 1 : cap > 8 ? 8 : cap;
    const bool expected = processed >= 0 && processed < clamped &&
     (processed == 0 || (elapsed >= 0 && elapsed < 4));
    assert(P::CanProcess(processed, cap, elapsed, 4) == expected);
    ++cases;
   }
 const double nan = std::numeric_limits<double>::quiet_NaN();
 const double inf = std::numeric_limits<double>::infinity();
 assert(P::ClampBudget(nan) == 4 && P::ClampBudget(inf) == 4);
 assert(P::ClampBudget(-1) == 0.5 && P::ClampBudget(100) == 12);
 assert(P::CanProcess(0, 2, nan, 4));
 assert(!P::CanProcess(1, 2, nan, 4));
 assert(!P::CanProcess(1, 2, inf, 4));
 std::cout << cases << " deterministic startup policy cases passed\n";
}
