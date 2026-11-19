#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallEliteTactics.h"
#include <cassert>
#include <iostream>
using namespace MoonfallEliteTactics;

static_assert(CanArm(true,false,false,true,false,true), "Only healthy active elites may adapt");
static_assert(!CanArm(true,true,false,true,false,true), "Bosses must keep original patterns");
static_assert(!CanArm(false,false,false,true,false,true), "Ordinary mobs cannot adapt");
static_assert(!CanArm(true,false,true,true,false,true), "Practice mobs remain predictable");
static_assert(!CanArm(true,false,false,false,false,true), "Dead elites never adapt");
static_assert(!CanArm(true,false,false,true,true,true), "Broken posture prevents adaptation");
static_assert(!CanArm(true,false,false,true,false,false), "Adaptation has its own cooldown");
static_assert(SelectPattern(0,true,false,true)==CounterSweepPattern);
static_assert(SelectPattern(3,true,false,true)==CounterSweepPattern);
static_assert(SelectPattern(3,true,true,true)==3);
static_assert(SelectPattern(0,false,false,true)==0);
static_assert(SelectPattern(1,true,false,false)==1);
static_assert(CounterSweepWindup>=0.75f && CounterSweepWindup<=1.2f);
static_assert(ParryMemorySeconds>0.f && ParryMemorySeconds<ParryResponseCooldownSeconds);
static_assert(ParryResponseCooldownSeconds == 5.f, "Elite counter cooldown must be five seconds");

int main(){
 long checked=0;
 // All four original patterns, eight enemy/practice/health state flags,
 // plus independent response availability. No combat RNG involved.
 for(int pattern=0; pattern<4; ++pattern)
 for(int mask=0; mask<128; ++mask){
  const bool elite=(mask&1)!=0, boss=(mask&2)!=0;
  const bool practice=(mask&4)!=0, alive=(mask&8)!=0;
  const bool broken=(mask&16)!=0, cooldownReady=(mask&32)!=0;
  const bool activeMemory=(mask&64)!=0;
  const bool mayArm=CanArm(elite,boss,practice,alive,broken,cooldownReady);
  assert(mayArm == (elite&&!boss&&!practice&&alive&&!broken&&cooldownReady));
  const bool armed=mayArm&&activeMemory;
  const int chosen=SelectPattern(pattern,elite,boss,armed);
  assert(chosen>=0 && chosen<4);
  assert(chosen == (armed?CounterSweepPattern:pattern));
  // The original move's damage/range/recovery remain defined by its
  // normal pattern. An expired memory must never change a pattern.
  assert(SelectPattern(pattern,elite,boss,false)==pattern);
  ++checked;
 }
 // After parry, one read-only memory window is consumed by one attack
 // and a separate 8-second cooldown prevents permanent adaptive sweeps.
 float now=10.f;
 float readyUntil=now+ParryMemorySeconds;
 float nextAllowedAt=now+ParryResponseCooldownSeconds;
 assert(readyUntil>now && readyUntil<nextAllowedAt);
 assert(CanArm(true,false,false,true,false,now>=nextAllowedAt)==false);
 float nextAttackAt=now+1.1f;
 assert(nextAttackAt<readyUntil);
 assert(SelectPattern(0,true,false,nextAttackAt<readyUntil)==2);
 readyUntil=-1.f; // consumed at commitment
 assert(SelectPattern(0,true,false,nextAttackAt<readyUntil)==0);
 assert(SelectPattern(0,true,false,(now+10.f)<readyUntil)==0);
 assert(CanArm(true,false,false,true,false,(now+10.f)>=nextAllowedAt));
 std::cout<<"Moonfall elite tactics: "<<checked
          <<" deterministic policy combinations PASS\n";
 return 0;
}
