#include "MoonfallLivingWorldPolicy.h"
#include <cassert>
#include <iostream>
using namespace MoonfallWorldPolicy;
int main(){long n=0;
 for(int a=-150;a<=150;++a)for(int b=-150;b<=150;++b){int c=Relationship(a,b);assert(c>=-100&&c<=100);assert(c==Clamp(Clamp(a,-100,100)+Clamp(b,-100,100),-100,100));++n;}
 for(int c=0;c<=100;++c)for(int h=-1;h<=5;++h){assert(RelayConfidence(c)<=c);assert(CanRelay(c,h)==(h>=0&&h<3&&c*3/4>=20));++n;}
 for(int f=0;f<4;++f)for(int t=0;t<4;++t)for(bool r:{false,true})for(bool c:{false,true}){bool expected=(f==0&&t==1)||(f==1&&t==2)||(f==2&&(t==1||t==3))||(f==3&&t==1&&r&&c);assert(CanTransition(f,t,r,c)==expected);++n;}
 int c=100;for(int i=0;i<3;++i){assert(CanRelay(c,i));c=RelayConfidence(c);++n;}assert(c==42&&!CanRelay(c,3));++n;
 std::cout<<"PASS: "<<n<<" living world policy cases\n";
}
