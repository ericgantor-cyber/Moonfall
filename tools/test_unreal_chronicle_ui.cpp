// Native-free C++17 tests for 42-chapter Atlas reader geometry and input.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallChronicleUI.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallChronicle.h"
#include <cassert>
#include <iostream>

namespace U = MoonfallChronicleUI;
namespace S = MoonfallChronicle;
static_assert(U::Tabs == S::BeatsPerAct);
static_assert(U::NextChapter(6,0,1) == 0);
static_assert(U::NextChapter(0,0,-1) == 6);
static_assert(U::NextChapter(-1,2,1) == 3);
static_assert(U::NextChapter(-1,0,0) == -1);
static_assert(!U::Layout(0.f,0.f,294.f,185.f,true).Visible);
static_assert(!U::Layout(0.f,0.f,295.f,184.f,true).Visible);
static_assert(U::Layout(0.f,0.f,295.f,185.f,true).Visible);
static_assert(U::Layout(0.f,0.f,295.f,185.f,true).LoreLines == 3);
static_assert(U::Layout(0.f,0.f,420.f,315.f,true).LoreLines == 5);
static_assert(U::Layout(0.f,0.f,420.f,315.f,false).LoreLines == 1);
static_assert(U::Layout(0.f,0.f,626.f,509.f,true).LoreLines == 3);

int main() {
 for (float W : {294.f,295.f,316.f,414.f,420.f,626.f,900.f}) {
  for (float H : {184.f,185.f,200.f,255.f,315.f,389.f,390.f,509.f}) {
   for (bool Focus : {false,true}) {
    const auto Card=U::Layout(100.f,110.f,W,H,Focus);
    assert(Card.Visible == (W>=295.f && H>=185.f));
    assert(Card.TabWidth>0.f);
    if (!Card.Visible) {
     assert(U::HitChapter(Card,Card.TabLeft+3.f,Card.TabTop+3.f) == -1);
     assert(!U::HitToggle(Card,Card.ToggleLeft+3.f,Card.ToggleTop+3.f));
     assert(!U::HitCard(Card,Card.Left+3.f,Card.Top+3.f));
     continue;
    }
    assert(Card.Left>=100.f);
    assert(Card.Top>=110.f);
    assert(Card.Left+Card.Width<=100.f+W);
    assert(Card.Top+Card.Height<=110.f+H);
    assert(Card.TabLeft>=Card.Left);
    assert(Card.TabTop>=Card.Top);
    assert(Card.TabLeft+Card.TabWidth*7<=Card.Left+Card.Width);
    assert(Card.TabTop+Card.TabHeight<=Card.Top+Card.Height);
    assert(Card.ToggleLeft>=Card.Left);
    assert(Card.ToggleTop>=Card.Top);
    assert(Card.ToggleLeft+Card.ToggleWidth<=Card.Left+Card.Width);
    assert(Card.ToggleTop+Card.ToggleHeight<=Card.Top+Card.Height);
    assert(U::HitToggle(Card,Card.ToggleLeft+3.f,Card.ToggleTop+3.f));
    assert(U::HitCard(Card,Card.Left+3.f,Card.Top+3.f));
    assert(!U::HitCard(Card,Card.Left-1.f,Card.Top+3.f));
    assert(!U::HitCard(Card,Card.Left+3.f,Card.Top+Card.Height+1.f));
    assert(Card.LoreLines>=1 && Card.LoreLines<=5);
    for (int Tab=0;Tab<7;++Tab) {
     const float MidX=Card.TabLeft+Card.TabWidth*(Tab+0.5f);
     const float MidY=Card.TabTop+Card.TabHeight*0.5f;
     assert(U::HitChapter(Card,MidX,MidY)==Tab);
     assert(U::HitCard(Card,MidX,MidY));
    }
    assert(U::HitChapter(Card,Card.TabLeft-1.f,Card.TabTop+2.f)==-1);
    assert(U::HitChapter(Card,Card.TabLeft+7*Card.TabWidth+1.f,
       Card.TabTop+2.f)==-1);
    assert(U::HitChapter(Card,Card.TabLeft+4.f,Card.TabTop-1.f)==-1);
    assert(U::HitChapter(Card,Card.TabLeft+4.f,
       Card.TabTop+Card.TabHeight+1.f)==-1);
   }
  }
 }
 for (int Start=-1;Start<=7;++Start) {
  for (int Featured=0;Featured<7;++Featured) {
   int Forward=Start;
   int Backward=Start;
   for(int Step=0;Step<7;++Step) {
    Forward=U::NextChapter(Forward,Featured,1);
    Backward=U::NextChapter(Backward,Featured,-1);
    assert(Forward>=0 && Forward<7);
    assert(Backward>=0 && Backward<7);
   }
   if(Start>=0 && Start<=6) {
    assert(Forward==Start);
    assert(Backward==Start);
   } else {
    assert(Forward==Featured);
    assert(Backward==Featured);
   }
  }
 }
 std::cout<<"42 chapter navigation, hit tests, compact and full-text Atlas layout passed\n";
}
