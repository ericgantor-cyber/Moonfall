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
 // Record restoration is atomic, with exact lengths and range checks.
 M::State Restored{};
 const std::array<int,6> SavedXP{{0,650,1800,300,650,300}};
 const std::array<std::uint64_t,8> SavedOrbs{{1,0,0,0,0,0,0,std::uint64_t{1}<<63}};
 assert(M::TryRestoreRecord(1,SavedXP,SavedOrbs,Restored));
 assert(Restored.XP==SavedXP&&Restored.Collected==SavedOrbs);
 assert(M::HasUnlock(Restored,M::Unlock::Vault));
 assert(M::HasUnlock(Restored,M::Unlock::AirDash));
 assert(M::HasUnlock(Restored,M::Unlock::AdvancedClimb));
 assert(M::HasUnlock(Restored,M::Unlock::PerfectGuard));
 assert(M::HasUnlock(Restored,M::Unlock::HeavyFollowup));
 assert(M::HasUnlock(Restored,M::Unlock::ArcaneChain));
 assert(M::HasUnlock(Restored,M::Unlock::EfficientCraft));
 assert(!M::HasUnlock(Restored,static_cast<M::Unlock>(99)));
 assert(!M::TryRestoreRecord(2,SavedXP,SavedOrbs,Restored));
 assert(!M::TryRestoreRecord(0,SavedXP,SavedOrbs,Restored));
 auto BadXP=SavedXP;BadXP[2]=-1;
 assert(!M::TryRestoreRecord(1,BadXP,SavedOrbs,Restored));
 assert(!M::TryRestoreRecord(1,std::array<int,5>{},SavedOrbs,Restored));
 assert(!M::TryRestoreRecord(1,SavedXP,std::array<std::uint64_t,7>{},Restored));
 assert(Restored.XP==SavedXP&&Restored.Collected==SavedOrbs);
 assert(M::TryRestoreRecord(0,std::array<int,0>{},std::array<std::uint64_t,0>{},Restored));
 assert(M::RankFor(Restored,M::Skill::Agility)==0&&!M::HasUnlock(Restored,M::Unlock::Vault));
 for(auto Bits:Restored.Collected)assert(Bits==0);
 // Stable route identities, bounded region capacity and exactly one hidden
 // collectible per enabled route; no random IDs or overlapping coordinates.
 int Hidden=0;
 for(int I=0;I<M::WorldRouteCount;++I){
  const auto P=M::RoutePlacement(I);
  assert(M::ValidOrbID(P.Region,P.Index)&&P.Index<4);
  if(P.Kind==M::Orb::Hidden)++Hidden;
  for(int J=0;J<I;++J){
   const auto Q=M::RoutePlacement(J);
   assert(P.Region!=Q.Region||P.Index!=Q.Index);
   assert(P.X!=Q.X||P.Y!=Q.Y);
  }
  if(P.Region<6)assert(P.Y>=-2950&&P.Y<=-2050);
 }
 assert(Hidden==7);
 assert(!M::ValidOrbID(M::RoutePlacement(-1).Region,0));
 assert(!M::ValidOrbID(M::RoutePlacement(M::WorldRouteCount).Region,0));
 M::State Travel{};
 assert(M::JumpBonusBps(Travel)==0);
 assert(!M::CanAirDash(Travel,true,false,true));
 Travel.XP[static_cast<int>(M::Skill::Agility)]=100;
 assert(M::JumpBonusBps(Travel)==600);
 Travel.XP[static_cast<int>(M::Skill::Agility)]=650;
 assert(M::JumpBonusBps(Travel)==1800);
 assert(M::CanAirDash(Travel,true,false,true));
 assert(!M::CanAirDash(Travel,false,false,true));
 assert(!M::CanAirDash(Travel,true,true,true));
 assert(!M::CanAirDash(Travel,true,false,false));
 Travel.XP[static_cast<int>(M::Skill::Agility)]=1800;
 assert(M::JumpBonusBps(Travel)==2500);
 Travel.XP[0]=-1;
 assert(M::JumpBonusBps(Travel)==0&&!M::CanAirDash(Travel,true,false,true));
 return 0;
}
