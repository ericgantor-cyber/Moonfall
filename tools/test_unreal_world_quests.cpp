// Native-free C++17 policy regression for 12 automatic regional world quests.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestJournal.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace W = MoonfallWorldQuest;
static_assert(W::GoalCount == 12 && W::ZoneCount == 6);
static_assert(W::MaxVisible == 7);
static_assert(W::Type(0)[0] == 'E' && W::Type(1)[0] == 'B');
constexpr bool FreshWorld() {
 int Seen[6]{}, Dead[6]{};
 const auto R = W::Select(Seen, Dead, 7);
 return R.Valid && R.Completed == 0 && R.Unfinished == 12
    && R.Count == 6 && R.Indices[0] == 0
    && R.Indices[1] == 2 && R.Indices[2] == 4
    && R.Indices[3] == 6 && R.Indices[4] == 8
    && R.Indices[5] == 10;
}
static_assert(FreshWorld(), "Unvisited zones should offer six discovery goals.");
constexpr bool ZoneInProgress() {
 int Seen[6]{1, 0, 0, 0, 0, 0}, Dead[6]{};
 const auto R = W::Select(Seen, Dead, 7);
 return R.Valid && R.Count == 7 && R.Indices[0] == 0
    && R.Indices[1] == 1 && R.Indices[2] == 2
    && R.Indices[3] == 4 && R.Indices[4] == 6
    && R.Indices[5] == 8 && R.Indices[6] == 10;
}
static_assert(ZoneInProgress(), "Started regions prioritize discovery and boss quests.");
constexpr bool DoneStagesDisappear() {
 int Seen[6]{5, 2, 0, 0, 0, 0}, Dead[6]{5, 1, 0, 0, 0, 0};
 const auto R = W::Select(Seen, Dead, 7);
 return R.Valid && R.Completed == 2 && R.Unfinished == 10
    && R.Count == 6 && R.Indices[0] == 2 && R.Indices[1] == 3
    && R.Indices[2] == 4 && R.Indices[3] == 6
    && R.Indices[4] == 8 && R.Indices[5] == 10;
}
static_assert(DoneStagesDisappear(), "Completed quests must never reuse slots.");
constexpr bool AllFinished() {
 int Seen[6]{5,5,5,5,5,5}, Dead[6]{5,5,5,5,5,5};
 const auto R = W::Select(Seen, Dead, 7);
 return R.Valid && R.Completed == 12 && R.Unfinished == 0 && R.Count == 0;
}
static_assert(AllFinished(), "12 complete regional quests return to exploration.");
constexpr bool RejectCorruption() {
 int Seen[6]{0,0,6,0,0,0}, Dead[6]{};
 const auto Wrong = W::Select(Seen, Dead, 7);
 Seen[2]=0; Dead[1]=1;
 const auto BossWithoutEntrance = W::Select(Seen, Dead, 7);
 return !Wrong.Valid && Wrong.Count == 0
   && !BossWithoutEntrance.Valid && BossWithoutEntrance.Count == 0
   && !W::Select(nullptr, Dead, 7).Valid
   && !W::Select(Seen, nullptr, 7).Valid
   && !W::Select(Seen, Dead, -1).Valid
   && !W::Select(Seen, Dead, 8).Valid;
}
static_assert(RejectCorruption(), "Bad saved counts must not produce a quest.");
namespace J = MoonfallQuestJournal;
static_assert(J::MaxTracked == W::MaxVisible);
constexpr bool ManualTracking() {
 J::FMask Active{}, M{};
 for(int I=0;I<100;++I) Active.Add(I);
 for(int I=0;I<7;++I) M=J::Toggle(M,Active,I);
 if(J::Count(Active)!=100||J::Count(M)!=7||J::Toggle(M,Active,99)!=M) return false;
 M=J::Toggle(M,Active,2);
 M=J::Toggle(M,Active,99);
 if(J::Count(M)!=7||!M.Has(99)||M.Has(2))return false;
 Active.Remove(0);
 return J::EntryAt(Active,98)==99&&J::EntryAt(Active,99)==-1
   &&J::Count(J::Toggle(M,Active,-1))==6;
}
static_assert(ManualTracking(), "100 accepted quests support seven independent tracking slots.");
static_assert(J::CanAccept(99)&&!J::CanAccept(100)&&!J::CanAccept(-1));
constexpr bool HeaderButtonGeometry() {
 for(int Width=276;Width<=900;++Width) {
  const auto B=J::HeaderButtons(float(Width));
  if(B.AutoX<120.f || B.AutoX+B.Width>=B.MapX || B.MapX+B.Width>Width-14.f) return false;
  if(!B.HitAuto(B.AutoX,46.f)||!B.HitMap(B.MapX,46.f)) return false;
  if(B.HitAuto(B.MapX,46.f)||B.HitMap(B.AutoX,46.f)) return false;
  if(B.HitAuto(B.AutoX+B.Width,46.f)||B.HitMap(B.MapX,66.f)) return false;
 }
 return !J::HeaderButtons(275.f).HitAuto(0.f,46.f)
     && !J::HeaderButtons(901.f).HitMap(0.f,46.f);
}
static_assert(HeaderButtonGeometry(), "Compact and wide quest header controls stay separate.");
static_assert(J::PageRows(240.f)==3 && J::PageRows(1080.f)==17);
static_assert(J::PageRows(239.f)==0 && J::PageRows(1000000000.f)==17);
static_assert(J::HitRow(0.f,4)==0 && J::HitRow(27.9f,4)==0);
static_assert(J::HitRow(28.f,4)==-1 && J::HitRow(30.f,4)==1);
static_assert(J::HitRow(-1.f,4)==-1 && J::HitRow(120.f,4)==-1);
constexpr bool SeparateQuestCounts() {
 J::FMask Active{};
 Active.Add(0); Active.Add(J::ResidentCount-1);
 Active.Add(J::ResidentCount); Active.Add(J::EntryCount-1);
 return J::Count(Active)==4 && J::CountContracts(Active)==2;
}
static_assert(SeparateQuestCounts());
static_assert(J::PageStart(16,4)==16 && J::PageStart(26,17)==17);
constexpr bool PageFooterGeometry() {
 for(int Height=240;Height<=674;++Height) {
  const int Rows=J::PageRows(float(Height));
  const auto B=J::PageButtons(276.f,float(Height-24));
  if(B.Width!=80.f||88.f+(Rows-1)*30.f+28.f>B.Y) return false;
  if(B.Hit(B.PrevX,B.Y)!=-1||B.Hit(B.NextX,B.Y)!=1) return false;
  if(B.Hit(B.PrevX+B.Width,B.Y)!=0||B.Hit(B.NextX,B.Y+22.f)!=0) return false;
 }
 return J::PageButtons(275.f,216.f).Hit(12.f,188.f)==0;
}
static_assert(PageFooterGeometry());
static_assert(J::SelectPage(1,3,100,1)==3 && J::SelectPage(4,3,100,-1)==0);
static_assert(J::SelectPage(98,3,100,1)==99 && J::SelectPage(99,3,100,1)==99);
static_assert(J::SelectPage(0,3,100,-1)==0 && J::SelectPage(5,3,0,1)==0);
static_assert(J::SelectPage(999,3,5,1)==4 && J::SelectPage(4,0,5,-1)==4);
constexpr bool MoreAcceptedThanTracked() {
 int States[J::ResidentCount]{};
 for (int I=0; I<J::ResidentCount; ++I) States[I]=1;
 const auto Before=J::Summarize(States,J::ResidentCount,25);
 if (Before.Active!=J::ResidentCount || Before.TrackedCount!=7) return false;
 const int First=Before.TrackedIndices[0];
 States[First]=2;
 const auto After=J::Summarize(States,J::ResidentCount,25);
 if (After.Active!=J::ResidentCount-1 || After.Completed!=1 || After.TrackedCount!=7) return false;
 for (int I=0; I<After.TrackedCount; ++I)
  if (After.TrackedIndices[I]==First) return false;
 return After.TrackedIndices[6]==Before.TrackedIndices[6]+1;
}
static_assert(MoreAcceptedThanTracked(), "Tracking must never cap accepted quests.");
int main() {
 for (int Active=0; Active<=J::ResidentCount; ++Active) {
  int States[J::ResidentCount]{};
  for (int I=0; I<J::ResidentCount; ++I) States[I]=I<Active?1:2;
  for (int Salvage=0; Salvage<=30; ++Salvage) {
   const auto C=J::Summarize(States,J::ResidentCount,Salvage);
   assert(C.Active==Active && C.Completed==J::ResidentCount-Active);
   assert(C.TrackedCount==(Active<7?Active:7));
   int Seen[6]{1,0,0,0,0,0}, Dead[6]{};
   const auto G=W::Select(Seen,Dead,W::MaxVisible-C.TrackedCount);
   assert(G.Valid && G.Count+C.TrackedCount<=7);
   bool Unready=false;
   for (int I=0; I<C.TrackedCount; ++I) {
    const int Index=C.TrackedIndices[I];
    assert(Index>=0 && Index<J::ResidentCount && States[Index]==1);
    const bool Ready=Salvage>=J::Cost(Index);
    assert(!Unready || !Ready);
    if (!Ready) Unready=true;
    for (int K=0; K<I; ++K) assert(C.TrackedIndices[K]!=Index);
   }
  }
 }
 for (int I=0; I<J::ResidentCount; ++I) {
  assert(J::Cost(I)>=10 && J::Cost(I)<=25);
  for(int K=0; K<I; ++K) assert(std::strcmp(J::Residents[I].Id,J::Residents[K].Id)!=0);
 }
 for (int Goal=0; Goal<W::GoalCount; ++Goal) {
  assert(std::strlen(W::GoalTitles[Goal]) >= 6);
  assert(std::strlen(W::GoalTitles[Goal]) <= 30);
  assert(std::strlen(W::RegionNames[Goal/2]) <= 18);
  for (int Other=Goal+1; Other<W::GoalCount; ++Other)
   assert(std::strcmp(W::GoalTitles[Goal], W::GoalTitles[Other]) != 0);
 }
 for (int Stage=0; Stage<=5; ++Stage) {
  for (int Wins=0; Wins<=Stage; ++Wins) {
   int Seen[6]{}, Dead[6]{};
   Seen[0]=Stage; Dead[0]=Wins;
   for(int Slots=0; Slots<=7; ++Slots) {
    const auto R=W::Select(Seen,Dead,Slots);
    assert(R.Valid && R.Count<=Slots && R.Count<=7);
    assert(R.Completed==((Stage==5)?1:0)+((Wins==5)?1:0));
    for (int I=0; I<R.Count; ++I) {
     const int Goal=R.Indices[I];
     assert(Goal>=0 && Goal<W::GoalCount);
     assert(W::Progress(Goal,Seen,Dead)>=0);
     assert(W::Progress(Goal,Seen,Dead)<5);
     for (int K=I+1;K<R.Count;++K)
      assert(R.Indices[I]!=R.Indices[K]);
    }
    if (Slots>0) {
     for (int Height=0;Height<=300;++Height) {
      const int Rows=W::VisibleRows(static_cast<float>(Height),Slots);
      assert(Rows>=0 && Rows<=Slots);
      assert(Rows==0 || W::HeaderHeight+Rows*W::RowHeight<=Height);
     }
    }
   }
  }
 }
 std::cout<<"12 regional world quests and seven-slot blending policy passed\n";
}
