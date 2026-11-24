#include "MoonfallLivingWorldPolicy.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace MoonfallWorldPolicy;
int main(){long n=0;
 for(int a=-150;a<=150;++a)for(int b=-150;b<=150;++b){int c=Relationship(a,b);assert(c>=-100&&c<=100);assert(c==Clamp(Clamp(a,-100,100)+Clamp(b,-100,100),-100,100));++n;}
 for(int c=0;c<=100;++c)for(int h=-1;h<=5;++h){assert(RelayConfidence(c)<=c);assert(CanRelay(c,h)==(h>=0&&h<3&&c*3/4>=20));++n;}
 for(int f=0;f<4;++f)for(int t=0;t<4;++t)for(bool r:{false,true})for(bool c:{false,true}){bool expected=(f==0&&t==1)||(f==1&&t==2)||(f==2&&(t==1||t==3))||(f==3&&t==1&&r&&c);assert(CanTransition(f,t,r,c)==expected);++n;}
 int c=100;for(int i=0;i<3;++i){assert(CanRelay(c,i));c=RelayConfidence(c);++n;}assert(c==42&&!CanRelay(c,3));++n;
 const double nan=std::numeric_limits<double>::quiet_NaN();
 const double inf=std::numeric_limits<double>::infinity();
 assert(GuardWatchPoint(nan,0,75,true)==WatchPoint::Unchanged);++n;
 assert(GuardWatchPoint(0,inf,75,true)==WatchPoint::Unchanged);++n;
 for(int x=-100;x<=100;x+=10)for(int y=-100;y<=100;y+=10)
 for(int confidence=-1;confidence<=101;++confidence)for(bool available:{false,true}){
  auto expected=WatchPoint::Unchanged;
  if(available&&confidence>=56&&confidence<=100&&(std::abs(x)>10||std::abs(y)>10)){
   if(std::abs(x)>=std::abs(y))expected=x<0?WatchPoint::West:WatchPoint::East;
   else expected=y<0?WatchPoint::South:WatchPoint::North;
  }
  assert(GuardWatchPoint(x,y,confidence,available)==expected);++n;
 }
 for(int score=-100;score<=100;++score)for(int confidence=-1;confidence<=101;++confidence)
 for(double elapsed:{-1.0,0.0,59.9,60.0,120.0,nan,inf}){
  const int expected=score>=0&&score<30&&confidence>=56&&confidence<=100&&std::isfinite(elapsed)&&elapsed>=60.0?1:0;
  assert(AmbientTrustDelta(score,confidence,elapsed)==expected);
  ++n;
 }
 for(int bond=-100;bond<=100;++bond)for(int rep=-100;rep<=100;++rep)
 for(bool danger:{false,true})for(bool report:{false,true})for(bool guard:{false,true}){
  const Mood expected=danger?(guard?Mood::Vigilant:Mood::Afraid):
    (rep<=-50||bond<=-60)?Mood::Resentful:
    report?(guard?Mood::Vigilant:Mood::Uneasy):
    (rep<=-15||bond<=-15)?Mood::Wary:
    bond>=60?Mood::Loyal:(bond>=15||rep>=15)?Mood::Warm:Mood::Calm;
  assert(ResolveMood(bond,rep,danger,report,guard)==expected);++n;
 }
 std::cout<<"PASS: "<<n<<" living world policy cases\n";
}
