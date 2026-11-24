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
 for(int bond:{-100,0,59,60,100})for(int rep:{-100,-1,0,15,100})
 for(double home:{-1.0,0.0,202500.0,202501.0,nan,inf})
 for(double hero:{-1.0,0.0,57600.0,57601.0,360000.0,360001.0,nan,inf})
 for(bool safe:{false,true})for(bool enabled:{false,true}){
  const bool expected=enabled&&safe&&bond>=60&&rep>=0&&std::isfinite(home)&&std::isfinite(hero)
    &&home>=0&&home<=202500.0&&hero>57600.0&&hero<=360000.0;
  assert(CanLocalGuide(bond,rep,home,hero,safe,enabled)==expected);++n;
 }
 for(float damage:{-1.f,0.f,1.f,20.f,80.f,1000.f,static_cast<float>(nan),static_cast<float>(inf)})
 for(double elapsed:{-1.0,0.0,11.99,12.0,60.0,nan,inf})
 for(double distance:{-1.0,0.0,90000.0,90001.0,nan,inf})
 for(bool eligible:{false,true}){
  const float expected=eligible&&std::isfinite(damage)&&damage>0&&std::isfinite(elapsed)&&elapsed>=12
    &&std::isfinite(distance)&&distance>=0&&distance<=90000?std::fmin(damage*0.15f,12.f):0.f;
  assert(CompanionCover(damage,elapsed,distance,eligible)==expected);++n;
 }
 for(double age:{-1.0,0.0,5.0,5.01,nan,inf})
 for(double cooldown:{-1.0,0.0,7.99,8.0,60.0,nan,inf})
 for(double scout:{-1.0,0.0,32400.0,32401.0,nan,inf})
 for(double hero:{-1.0,0.0,122500.0,122501.0,nan,inf})
 for(bool eligible:{false,true}){
  const bool expected=eligible&&std::isfinite(age)&&age>=0&&age<=5
    &&std::isfinite(cooldown)&&cooldown>=8&&std::isfinite(scout)&&scout>=0&&scout<=32400
    &&std::isfinite(hero)&&hero>=0&&hero<=122500;
  assert(CanCompanionStrike(age,cooldown,scout,hero,eligible)==expected);++n;
 }
 for(int bond=-100;bond<=100;++bond){
  const int rank=bond>=100?3:bond>=80?2:bond>=60?1:0;
  assert(CompanionRank(bond)==rank);
  assert(CompanionStrikeDamage(rank)==(rank==0?0.f:rank==1?6.f:rank==2?8.f:10.f));++n;
 }
 for(int rank=-1;rank<=4;++rank)for(float damage:{0.f,10.f,100.f,1000.f})
 for(bool eligible:{false,true}){
  const float expected=!eligible||rank<1||rank>3?0.f:
    std::fmin(damage*(rank==3?0.20f:rank==2?0.18f:0.15f),rank==3?16.f:rank==2?14.f:12.f);
  assert(CompanionCover(damage,12.0,90000.0,eligible,rank)==expected);++n;
 }
 for(int rank=-1;rank<=4;++rank)for(int role=-1;role<=3;++role){
  const int effective=rank>=2&&rank<=3&&role>=1&&role<=2?role:0;
  assert(EffectiveCompanionRole(rank,role)==effective);
  const float base=rank==3?10.f:rank==2?8.f:rank==1?6.f:0.f;
  assert(CompanionStrikeDamage(rank,role)==(effective==1?base-2.f:effective==2?std::fmin(base+2.f,10.f):base));
  for(float damage:{0.f,10.f,100.f,1000.f}){
   const float fraction=rank==3?0.20f:rank==2?0.18f:0.15f;
   const float cap=rank==3?16.f:rank==2?14.f:12.f;
   const float shift=effective==1?0.04f:effective==2?-0.04f:0.f;
   const float capshift=effective==1?2.f:effective==2?-2.f:0.f;
   assert(CompanionCover(damage,12.0,90000.0,true,rank,role)==
      (rank<1||rank>3?0.f:std::fmin(damage*(fraction+shift),cap+capshift)));++n;
  }
  for(double elapsed:{5.99,6.0,7.99,8.0}){
   assert(CanCompanionStrike(1.0,elapsed,100.0,100.0,true,rank,role)==(elapsed>=(effective==2?6.0:8.0)));++n;
  }
 }
 for(int rank=-1;rank<=4;++rank)for(int role=-1;role<=3;++role){
  const auto perk=rank!=3?CompanionPerk::None:role==1?CompanionPerk::ShieldExpert:
    role==2?CompanionPerk::Precision:CompanionPerk::Fieldcraft;
  assert(VeteranPerk(rank,role)==perk);
  assert(CompanionExtraPosture(rank,role)==(perk==CompanionPerk::Precision?4.f:0.f));
  const double coverRange=perk==CompanionPerk::Fieldcraft?350.0:300.0;
  const double strikeRange=perk==CompanionPerk::Fieldcraft?220.0:180.0;
  const double coverCooldown=perk==CompanionPerk::ShieldExpert?8.0:12.0;
  assert(CompanionCoverRange(rank,role)==coverRange);
  assert(CompanionStrikeRange(rank,role)==strikeRange);
  assert(CompanionCoverCooldown(rank,role)==coverCooldown);
  for(double elapsed:{7.99,8.0,11.99,12.0,nan,inf})
  for(double distance:{90000.0,90001.0,122500.0,122501.0,nan,inf}){
   const bool expected=rank>=1&&rank<=3&&std::isfinite(elapsed)&&elapsed>=coverCooldown
     &&std::isfinite(distance)&&distance<=coverRange*coverRange;
   assert((CompanionCover(100.f,elapsed,distance,true,rank,role)>0.f)==expected);++n;
  }
  for(double distance:{32400.0,32401.0,48400.0,48401.0,nan,inf}){
   assert(CanCompanionStrike(1.0,8.0,distance,100.0,true,rank,role)==
       (std::isfinite(distance)&&distance<=strikeRange*strikeRange));++n;
  }
 }
 for(unsigned mask=0;mask<(1u<<18);++mask){
  const bool valid=ValidCompanionTraining(mask);
  if(!valid){
   assert(TrainCompanion(mask,3,0,0)==mask);
   assert(ActiveCompanionTraining(mask,3,0,0)==0);++n;continue;
  }
  for(int rank=0;rank<=3;++rank)for(int role=0;role<3;++role)for(int node=0;node<9;++node){
   const int level=CompanionNodeRank(mask,node);
   const int spent=CompanionPointsSpent(mask);
   const bool can=rank>=1&&role==node/3&&(role==0||rank>=2)&&rank>=1+node%3
       &&level<2&&spent<rank*2&&(node%3==0||CompanionNodeRank(mask,node-1)>0);
   assert(CanTrainCompanion(mask,rank,role,node)==can);
   const unsigned next=TrainCompanion(mask,rank,role,node);
   assert(next==(can?mask+(1u<<(2*node)):mask));
   if(can)assert(ValidCompanionTraining(next)&&CompanionPointsSpent(next)==spent+1);
   ++n;
  }
 }
 unsigned balanced=TrainCompanion(0,3,0,0);
 balanced=TrainCompanion(balanced,3,0,1);
 balanced=TrainCompanion(balanced,3,0,2);
 assert(CompanionCover(100.f,12.0,360.0*360.0,true,3,0,balanced)>0.f);
 assert(CompanionCover(100.f,12.0,371.0*371.0,true,3,0,balanced)==0.f);
 assert(CanCompanionStrike(1.0,8.0,230.0*230.0,100.0,true,3,0,balanced));
 unsigned protector=TrainCompanion(0,3,1,3);
 protector=TrainCompanion(protector,3,1,4);
 protector=TrainCompanion(protector,3,1,5);
 assert(CompanionCover(100.f,6.0,100.0,true,3,1,protector)>0.f);
 assert(CompanionCover(100.f,5.99,100.0,true,3,1,protector)==0.f);
 unsigned striker=TrainCompanion(0,3,2,6);
 striker=TrainCompanion(striker,3,2,7);
 striker=TrainCompanion(striker,3,2,8);
 assert(CanCompanionStrike(1.0,5.5,190.0*190.0,100.0,true,3,2,striker));
 assert(!CanCompanionStrike(1.0,5.49,190.0*190.0,100.0,true,3,2,striker));
 assert(ActiveCompanionTraining(striker,3,2,7)==1);
 assert(ActiveCompanionTraining(striker,1,0,7)==0);n+=9;
 for(unsigned mask=0;mask<(1u<<18);++mask){
  for(int role=0;role<4;++role){
   const unsigned profile=mask|(unsigned(role)<<18);
   const bool valid=role<3&&ValidCompanionTraining(mask);
   assert(ValidCompanionProfile(profile)==valid);
   assert(PackCompanionProfile(mask,role)==(valid?profile:0u));
   if(valid){assert((profile&CompanionTrainingMask)==mask);assert((profile>>18)==unsigned(role));}
   ++n;
  }
 }
 assert(!ValidCompanionProfile(0xffffffffu));++n;
 for(int current=-100;current<=100;++current)for(int victories=-1;victories<=31;++victories)
 for(bool contract:{false,true})for(int reputation:{-100,-1,0,100}){
  const int target=victories>=5?100:victories>=3?80:60;
  const int expected=current<0||!contract||reputation<0||victories<1||victories>30?current:(current<target?target:current);
  assert(ScoutDeedTrust(current,victories,contract,reputation)==expected);
  assert(ScoutDeedTrust(expected,victories,contract,reputation)==expected);++n;
 }
 for(double now:{-1.0,0.0,10.0,nan,inf})
 for(double target:{-1.0,0.0,122500.0,122501.0,nan,inf})
 for(double scout:{-1.0,0.0,250000.0,250001.0,nan,inf}){
  const bool expected=std::isfinite(now)&&now>=0&&std::isfinite(target)&&target>=0&&target<=122500
       &&std::isfinite(scout)&&scout>=0&&scout<=250000;
  assert(CanReportCompanionThreat(now,target,scout)==expected);++n;
 }
 std::cout<<"PASS: "<<n<<" living world policy cases\n";
}
