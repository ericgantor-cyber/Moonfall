#include "MoonfallNpcDecisionPolicy.h"
#include <cassert>
#include <iostream>
using namespace MoonfallNpcPolicy;
int main(){
 int checks=0;
 auto check=[&](Input i,Action expected){assert(Decide(i)==expected);++checks;};
 Input i; i.hour=3;check(i,Action::Sleep);i.hour=22;check(i,Action::Sleep);
 i.hour=7;check(i,Action::Eat);i.hour=12;check(i,Action::Eat);i.hour=18;check(i,Action::Eat);
 i.hour=9;check(i,Action::Work);i.hour=20;check(i,Action::Socialize);i.sociability=.1;check(i,Action::Idle);
 i.storm=true;check(i,Action::Shelter);i.sheltered=true;check(i,Action::Idle);
 i.curiosity=.7;check(i,Action::Investigate);i.courage=.1;check(i,Action::Idle);
 i=Input{};i.hour=9;i.playerNearby=true;i.sociability=.8;check(i,Action::Talk);i.trust=-.5;check(i,Action::Work);
 i.sociability=.5;i.trust=.5;check(i,Action::Talk);
 i.danger=.8;i.storm=true;i.sheltered=false;check(i,Action::Flee);i.allyNearby=true;check(i,Action::SeekHelp);
 i.danger=.9;check(i,Action::Flee);i.courage=1;i.aggression=1;check(i,Action::Defend);i.canDefend=false;check(i,Action::Flee);
 i.danger=.34;check(i,Action::Shelter);i.danger=.35;check(i,Action::Flee);
 for(int d=35;d<=100;++d)for(int c=0;c<=10;++c)for(int a=0;a<=10;++a)for(int flags=0;flags<4;++flags){Input v;v.danger=d/100.f;v.courage=c/10.f;v.aggression=a/10.f;v.allyNearby=flags&1;v.canDefend=flags&2;auto x=Decide(v);assert(x==Action::Flee||x==Action::Defend||x==Action::SeekHelp);if(!v.canDefend)assert(x!=Action::Defend);++checks;}
 std::cout<<checks<<" policy cases passed\n";
}
