#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAttackTellVisual.h"
#include <cassert>
#include <iostream>
using namespace MoonfallAttackTellVisual;

static_assert(Select(true,1.f,1.f)==EMove::CounterSweep);
static_assert(Select(false,2.3f,1.3f)==EMove::Crush);
static_assert(Select(false,1.8f,1.1f)==EMove::Heavy);
static_assert(Select(false,1.f,1.3f)==EMove::Sweep);
static_assert(Select(false,1.f,1.f)==EMove::Quick);
static_assert(Profile(EMove::CounterSweep).Depth > Profile(EMove::Sweep).Depth);
static_assert(Profile(EMove::Sweep).Depth > Profile(EMove::Quick).Depth);
static_assert(Profile(EMove::Heavy).Height > Profile(EMove::Quick).Height);
static_assert(Progress(1.f,1.f,false)==0.f);
static_assert(Progress(0.f,1.f,false)==1.f);
static_assert(Progress(0.5f,1.f,false)==0.5f);
static_assert(Progress(1.f,1.f,true)==1.f);
static_assert(Growth(Progress(1.f,1.f,false))==0.58f);
static_assert(Growth(Progress(0.f,1.f,false))==1.f);

int main(){
 long checked=0;
 for(int i=0;i<5;++i) {
  const FProfile p=Profile(static_cast<EMove>(i));
  assert(p.Width>0.f && p.Width<=0.6f);
  assert(p.Depth>0.f && p.Depth<=1.5f);
  assert(p.Height>0.f && p.Height<=0.5f);
  assert(p.YawDegrees>=-35.f && p.YawDegrees<=35.f);
  for(int durationStep=1;durationStep<=100;++durationStep){
   const float duration=0.05f*durationStep;
   float previous=0.f;
   for(int frame=0;frame<=100;++frame) {
    const float remaining=duration*(100-frame)/100.f;
    const float progress=Progress(remaining,duration,false);
    const float reduced=Progress(remaining,duration,true);
    const float grow=Growth(progress);
    assert(progress>=0.f && progress<=1.f);
    assert(progress+0.00001f>=previous);
    assert(reduced==1.f);
    assert(grow>=0.58f && grow<=1.000001f);
    assert(p.Width*grow<=p.Width);
    assert(p.Depth*grow<=p.Depth);
    assert(p.Height*grow<=p.Height);
    previous=progress;
    ++checked;
   }
  }
 }
 // Invalid or overshot timing inputs clamp rather than exaggerating markers.
 assert(Progress(-10.f,0.2f,false)==1.f);
 assert(Progress(100.f,0.2f,false)==0.f);
 assert(Progress(0.f,0.f,false)==0.f);
 std::cout<<"Moonfall procedural attack tells: "<<checked
          <<" bounded profiles, windup and reduce-motion combinations PASS\n";
 return 0;
}
