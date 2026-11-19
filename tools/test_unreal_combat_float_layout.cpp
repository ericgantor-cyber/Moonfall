#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatFloatLayout.h"
#include <cassert>
#include <iostream>
#include <limits>
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
static_assert(Intersects({30.f,30.f,100.f,25.f},{35.f,45.f,100.f,25.f}),
              "Crowded labels overlap");
static_assert(!Intersects({20.f,20.f,60.f,20.f},{200.f,100.f,60.f,20.f}),
              "Separated labels do not overlap");
static_assert(InViewport({8.f,8.f,100.f,20.f},1280.f,720.f),
              "A badge fits just inside safe margins");
static_assert(!InViewport({1.f,8.f,100.f,20.f},1280.f,720.f),
              "Screen-edge overflow is rejected");

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
 // Verify the actual viewport placement policy in packed encounters.
 // It must protect the hero's projected body and prior enemy labels,
 // while never moving a source-anchored cue beyond the screen.
 constexpr FScreenRect Hero{565.f,280.f,170.f,220.f};
 for(int x=0;x<=1250;x+=25)
 for(int y=0;y<=690;y+=23) {
  FScreenRect placed{};
  const bool found=TryPlace(static_cast<float>(x),static_cast<float>(y),
    160.f,28.f,1280.f,720.f,true,Hero,nullptr,0,placed);
  if(found) {
   assert(InViewport(placed,1280.f,720.f));
   assert(!Intersects(placed,Hero));
  }
  ++count;
 }
 // Test collisions across all sixteen already accepted slots and ensure
 // no overlap with accepted labels, regardless of how crowded it gets.
 FScreenRect accepted[MaxVisibleAttackOutcomes]{};
 int n=0;
 for(int y=100;y<=450;y+=22)
 for(int x=210;x<=950;x+=41) {
  FScreenRect placed{};
  if(TryPlace(static_cast<float>(x),static_cast<float>(y),160.f,29.f,
              1280.f,720.f,true,Hero,accepted,n,placed)){
   assert(n<MaxVisibleAttackOutcomes);
   assert(!Intersects(placed,Hero));
   assert(InViewport(placed,1280.f,720.f));
   for(int k=0;k<n;++k) assert(!Intersects(placed,accepted[k]));
   accepted[n++]=placed;
  }
  ++count;
  if(n==MaxVisibleAttackOutcomes) break;
 }
 assert(n>0 && n<=MaxVisibleAttackOutcomes);
 FScreenRect scratch{};
 assert(!TryPlace(1.f,1.f,400.f,80.f,120.f,100.f,
                  false,Hero,nullptr,0,scratch));
 assert(!TryPlace(80.f,80.f,160.f,35.f,320.f,240.f,
                  true,{0.f,0.f,320.f,240.f},nullptr,0,scratch));
 assert(!TryPlace(80.f,80.f,160.f,35.f,320.f,240.f,
                  false,Hero,nullptr,-1,scratch));
 assert(!TryPlace(80.f,80.f,160.f,35.f,320.f,240.f,
                  false,Hero,nullptr,1,scratch));
 assert(!InViewport({std::numeric_limits<float>::quiet_NaN(),2.f,20.f,15.f},
                     1280.f,720.f));
 std::cout<<"Moonfall enemy-only combat float layout: "<<count
   <<" lifetime/visibility/motion/lane combinations PASS\n";
}
