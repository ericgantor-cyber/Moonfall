#pragma once
// Native-free shared Atlas Chronicle geometry and hit tests.
// DrawHUD and player mouse input must use the SAME coordinates.
namespace MoonfallChronicleUI {
inline constexpr int Tabs = 7;
struct FCard {
 bool Visible;
 bool Compact;
 bool Expanded;
 float Left;
 float Top;
 float Width;
 float Height;
 float TabLeft;
 float TabTop;
 float TabWidth;
 float TabHeight;
 float ToggleLeft;
 float ToggleTop;
 float ToggleWidth;
 float ToggleHeight;
 int LoreLines;
};
constexpr float Min(float A,float B) {return A<B?A:B;}
constexpr bool Inside(float X,float Y,float Left,float Top,float Width,float Height) {
 return X>=Left && X<Left+Width && Y>=Top && Y<Top+Height;
}
constexpr FCard Layout(float MapLeft,float MapTop,float MapWidth,
 float MapHeight,bool bFocused) {
 const bool Compact=MapHeight<390.f;
 const bool Visible=MapWidth>=295.f && MapHeight>=185.f;
 const bool Expanded=Compact && bFocused;
 const float Height=Compact?(Expanded?Min(198.f,MapHeight-16.f):108.f):163.f;
 const float Left=MapLeft+8.f;
 const float Top=MapTop+MapHeight-Height-8.f;
 const float Width=MapWidth-16.f;
 const float TabWidth=(Width-20.f)/Tabs;
 return {Visible,Compact,Expanded,Left,Top,Width,Height,
  Left+10.f,Top+26.f,TabWidth,21.f,
  Left+Width-65.f,Top+4.f,57.f,19.f,
  Expanded?(Height>=198.f?5:3):(Compact?1:3)};
}
constexpr int HitChapter(FCard Card,float X,float Y) {
 if (!Card.Visible || !Inside(X,Y,Card.TabLeft,Card.TabTop,
      Card.TabWidth*Tabs,Card.TabHeight)) return -1;
 const int Tab=static_cast<int>((X-Card.TabLeft)/Card.TabWidth);
 return Tab>=0 && Tab<Tabs?Tab:-1;
}
constexpr bool HitToggle(FCard Card,float X,float Y) {
 return Card.Visible && Inside(X,Y,Card.ToggleLeft,Card.ToggleTop,
  Card.ToggleWidth,Card.ToggleHeight);
}
constexpr bool HitCard(FCard Card,float X,float Y) {
 return Card.Visible && Inside(X,Y,Card.Left,Card.Top,Card.Width,Card.Height);
}
constexpr int NextChapter(int Current,int Featured,int Direction) {
 if (Direction!=1 && Direction!=-1) return Current;
 const int Base=Current>=0 && Current<Tabs?Current
   : Featured>=0 && Featured<Tabs?Featured:0;
 return (Base+Direction+Tabs)%Tabs;
}
static_assert(HitChapter(Layout(100.f,110.f,420.f,315.f,false),
 Layout(100.f,110.f,420.f,315.f,false).TabLeft+2.f,
 Layout(100.f,110.f,420.f,315.f,false).TabTop+2.f)==0);
static_assert(HitChapter(Layout(100.f,110.f,420.f,315.f,false),
 Layout(100.f,110.f,420.f,315.f,false).TabLeft+
 Layout(100.f,110.f,420.f,315.f,false).TabWidth*6.f+2.f,
 Layout(100.f,110.f,420.f,315.f,false).TabTop+2.f)==6);
static_assert(Layout(100.f,110.f,420.f,315.f,true).LoreLines==5);
static_assert(Layout(100.f,110.f,420.f,315.f,false).LoreLines==1);
static_assert(Layout(100.f,110.f,626.f,509.f,false).LoreLines==3);
static_assert(NextChapter(6,0,1)==0);
static_assert(NextChapter(0,0,-1)==6);
static_assert(NextChapter(-1,2,1)==3);
static_assert(NextChapter(-1,2,0)==-1);
} // namespace MoonfallChronicleUI
