// Native-free C++17 regression for Moonfall's confirmed-hit camera polish.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallImpactFeedback.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace I = MoonfallImpactFeedback;
int main() {
 const auto Base = I::ConfirmedHit(0.f, -100.f, 1.f, 40.f,
    1, false, false, false, false);
 assert(Base.Triggered && Base.Offset >= 3.f && Base.Offset <= 5.f);
 assert(Base.LastAt == 1.f);
 const auto SameFrame = I::ConfirmedHit(Base.Offset, Base.LastAt, 1.01f,
    200.f, 12, true, true, false, false);
 assert(!SameFrame.Triggered && SameFrame.Offset == Base.Offset);
 assert(SameFrame.LastAt == Base.LastAt);

 const auto Later = I::ConfirmedHit(Base.Offset, Base.LastAt, 1.11f,
    40.f, 2, false, false, false, false);
 assert(Later.Triggered && Later.LastAt == 1.11f);
 assert(Later.Offset > Base.Offset && Later.Offset <= I::MaxImpulse);
 const auto Milestone = I::ConfirmedHit(0.f, -100.f, 1.f,
    40.f, 4, false, false, false, false);
 const auto Ordinary = I::ConfirmedHit(0.f, -100.f, 1.f,
    40.f, 3, false, false, false, false);
 assert(Milestone.Triggered && Milestone.Offset > Ordinary.Offset);
 const auto BossKill = I::ConfirmedHit(0.f, -100.f, 1.f,
    100.f, 12, true, true, false, false);
 assert(BossKill.Triggered && BossKill.Offset > Milestone.Offset);
 assert(BossKill.Offset <= 10.f);

 // Reduce Motion removes existing oscillation and refuses new impulses.
 const auto Reduced = I::ConfirmedHit(14.f, 0.f, 1.f,
    100.f, 12, true, true, true, false);
 assert(!Reduced.Triggered && Reduced.Offset == 0.f);
 const auto Menu = I::ConfirmedHit(5.f, 0.f, 1.f,
    100.f, 12, true, true, false, true);
 assert(!Menu.Triggered && Menu.Offset == 5.f);
 for (float Damage : {0.f, -1.f, std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN()}) {
  const auto Result = I::ConfirmedHit(3.f, 0.f, 1.f,
     Damage, 4, true, true, false, false);
  assert(!Result.Triggered && Result.Offset == 3.f);
 }
 const auto NonFiniteTime = I::ConfirmedHit(3.f, 0.f,
    std::numeric_limits<float>::quiet_NaN(), 100.f, 4, true, true, false, false);
 assert(!NonFiniteTime.Triggered && NonFiniteTime.Offset == 3.f);
 const auto NonFiniteOffset = I::ConfirmedHit(
    std::numeric_limits<float>::infinity(), -100.f, 1.f,
    30.f, 1, false, false, false, false);
 assert(NonFiniteOffset.Triggered && std::isfinite(NonFiniteOffset.Offset));
 const auto ZeroStage = I::ConfirmedHit(3.f, -100.f, 1.f,
    40.f, 0, false, false, false, false);
 assert(!ZeroStage.Triggered);
 const auto ClockReset = I::ConfirmedHit(4.f, 300.f, 1.f,
    40.f, 4, false, false, false, false);
 assert(ClockReset.Triggered && ClockReset.LastAt == 1.f);

 float Offset = 0.f;
 float Last = -100.f;
 int Triggers = 0;
 for (int Frame=0; Frame<600; ++Frame) {
  // 120 high-damage spells per second. Visible response must not build
  // beyond the existing 24-unit camera arm safety limit.
  const auto R = I::ConfirmedHit(Offset, Last, Frame / 120.f,
     10000.f, 12, true, true, false, false);
  Offset = R.Offset; Last = R.LastAt;
  Triggers += R.Triggered ? 1 : 0;
  assert(std::isfinite(Offset) && Offset >= 0.f && Offset <= I::MaxImpulse);
 }
 assert(Triggers > 0 && Triggers < 100);

 std::cout << "Moonfall confirmed-hit AAA camera feedback policy passed\n";
}
