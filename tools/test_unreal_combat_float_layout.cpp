#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatFloatLayout.h"
#include <cassert>
#include <iostream>
using namespace MoonfallCombatFloatLayout;

static_assert(HistoryPerEnemy==4, "Fixed four-entry history per attacker");
static_assert(MaxVisibleAttackOutcomes==16, "Crowd defense labels capped");
static_assert(MaxVisibleDealtDamage==32, "Outgoing numbers remain bounded");
static_assert(MaxNearbyCombatActors==64, "Do not allocate extra scan candidates");
static_assert(!Active(-0.01f), "Future timestamps not visible");
static_assert(Active(0.f), "New combat events display immediately");
static_assert(!Active(LifetimeSeconds), "Expired outcomes disappear");
static_assert(Rise(0.5f,true)==0.f, "Reduced Motion never moves floats");
static_assert(Rise(-1.f,false)==0.f, "Invalid future event never moves");
static_assert(Opacity(-1.f)==0.f, "Future event invisible");
static_assert(Opacity(LifetimeSeconds)==0.f, "Expired event invisible");
static_assert(LaneX(0)!=LaneX(1), "Adjacent statuses occupy different columns");
static_assert(LaneY(0)==LaneY(1), "Two columns share first line");
static_assert(LaneY(2)>LaneY(0), "Next two statuses occupy a lower line");

int main() {
 long count=0;
 for(int millis=-250;millis<=1400;++millis) {
  const float age=millis/1000.f;
  const bool expected=age>=0.f && age<LifetimeSeconds;
  assert(Active(age)==expected);
  for(int reduced=0;reduced<2;++reduced) {
   const float alpha=Opacity(age),rise=Rise(age,reduced!=0);
   assert(alpha>=0.f && alpha<=1.f);
   assert(rise>=0.f && rise<=LifetimeSeconds*68.f);
   if(!expected) assert(alpha==0.f && rise==0.f);
   if(reduced) assert(rise==0.f);
   else if(expected) assert(rise==age*68.f);
   for(int slot=0;slot<HistoryPerEnemy;++slot) {
    const float x=LaneX(slot),y=LaneY(slot);
    assert(x>=-160.f && x<=50.f);
    assert(y>=0.f && y<=40.f);
    assert((slot%2==0)==(x<0.f));
    if(slot>=2) assert(y>0.f);
    ++count;
   }
  }
 }
 std::cout<<"Moonfall enemy-only combat float layout: "<<count
   <<" lifetime/visibility/motion/lane combinations PASS\n";
}
