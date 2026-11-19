#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAttackOutcome.h"
#include <cassert>
#include <iostream>
using namespace MoonfallAttackOutcome;

static_assert(Classify(false, false, false, 0.f)==EResult::Miss);
static_assert(Classify(false, true, false, 50.f)==EResult::Miss);
static_assert(Classify(true, true, false, 50.f)==EResult::Parry);
static_assert(Classify(true, false, true, 50.f)==EResult::Dodge);
static_assert(Classify(true, false, false, 25.f)==EResult::Hit);
static_assert(Classify(true, false, false, 0.f)==EResult::Guard);
static_assert(!MayPlayHitSound(EResult::Miss));
static_assert(!MayPlayHitSound(EResult::Dodge));
static_assert(!MayPlayHitSound(EResult::Parry));
static_assert(!MayPlayHitSound(EResult::Guard));
static_assert(MayPlayHitSound(EResult::Hit));
static_assert(MayPlayParrySound(EResult::Parry));
static_assert(!MayPlayParrySound(EResult::Hit));

int main(){
 long checked=0;
 for (int mask=0;mask<8;++mask) {
  const bool reached=(mask&1)!=0, parried=(mask&2)!=0, dodged=(mask&4)!=0;
  for (int step=0;step<10001;++step) {
   const float hpLost=step>0 ? (step-1)*0.01f : 0.f;
   const EResult actual=Classify(reached,parried,dodged,hpLost);
   const EResult expected=!reached ? EResult::Miss
     : parried ? EResult::Parry : dodged ? EResult::Dodge
     : hpLost>0.f ? EResult::Hit : EResult::Guard;
   assert(actual==expected);
   assert(MayPlayHitSound(actual)==(actual==EResult::Hit));
   assert(MayPlayParrySound(actual)==(actual==EResult::Parry));
   if (!reached || parried || dodged) assert(actual!=EResult::Hit);
   ++checked;
  }
 }
 std::cout<<"Moonfall attack outcomes: "<<checked
          <<" contact/defense/HP and sound-gating combinations PASS\n";
 return 0;
}
