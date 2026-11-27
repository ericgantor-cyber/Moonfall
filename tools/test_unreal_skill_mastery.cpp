#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallSkillMastery.h"
#include <cassert>
#include <limits>
#ifdef NDEBUG
#error Tests require assertions
#endif
namespace M=MoonfallSkillMastery;
int main(){
 M::State S{},Out{};
 assert(M::Valid(S)&&M::RankFor(S,M::Skill::Agility)==0);
 assert(M::Rank(-1)==-1&&M::Rank(M::MaxXP+1)==-1);
 for(int R=0;R<6;++R){
  assert(M::Rank(M::RankThresholds[static_cast<std::size_t>(R)])==R);
  if(R>0)assert(M::Rank(M::RankThresholds[static_cast<std::size_t>(R)]-1)==R-1);
 }
 for(int I=0;I<M::SkillCount;++I){
  const auto A=static_cast<M::Action>(I);
  assert(M::TryAward(S,A,true,Out));
  for(int J=0;J<M::SkillCount;++J)
   assert(Out.XP[static_cast<std::size_t>(J)]==(I==J?M::Reward(A):0));
 }
 const auto Previous=Out;
 assert(!M::TryAward(S,M::Action::MeleeVictory,false,Out));
 assert(!M::TryAward(S,static_cast<M::Action>(-1),true,Out));
 assert(Out.XP==Previous.XP&&Out.Collected==Previous.Collected);
 assert(M::TryCollect(S,0,63,M::Orb::Agility,true,S));
 assert(M::XPFor(S,M::Skill::Agility)==60&&M::IsCollected(S,0,63));
 assert(!M::TryCollect(S,0,63,M::Orb::Hidden,true,S));
 assert(M::TryCollect(S,7,63,M::Orb::Hidden,true,S));
 for(int I=0;I<M::SkillCount;++I)
  assert(S.XP[static_cast<std::size_t>(I)]==(I==static_cast<int>(M::Skill::Agility)?85:25));
 const auto Collected=S;
 for(int I:{-1,64,std::numeric_limits<int>::max()})
  assert(!M::TryCollect(S,0,I,M::Orb::Agility,true,S));
 assert(!M::TryCollect(S,8,0,M::Orb::Hidden,true,S));
 assert(!M::TryCollect(S,0,0,static_cast<M::Orb>(9),true,S));
 assert(!M::TryCollect(S,0,0,M::Orb::Hidden,false,S));
 assert(S.XP==Collected.XP&&S.Collected==Collected.Collected);
 S.XP.fill(M::MaxXP-1);
 assert(M::TryCollect(S,0,0,M::Orb::Hidden,true,S));
 for(int XP:S.XP)assert(XP==M::MaxXP);
 assert(!M::TryAward(S,M::Action::MeleeVictory,true,S));
 assert(M::TryCollect(S,0,1,M::Orb::Hidden,true,S));
 assert(M::IsCollected(S,0,1));
 S.XP[0]=std::numeric_limits<int>::max();
 assert(!M::TryCollect(S,0,2,M::Orb::Hidden,true,S));
 assert(!M::TryAward(S,M::Action::MeleeVictory,true,S));
 assert(!M::IsCollected(S,0,2));
 return 0;
}
