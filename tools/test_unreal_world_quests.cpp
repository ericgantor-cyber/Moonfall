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
 const auto Active=J::Bit(0)|J::Bit(1)|J::Bit(2)|J::Bit(3)|J::Bit(4)|J::Bit(5)|J::Bit(6)|J::Bit(7)|J::Bit(26);
 auto M=std::uint32_t(0);
 for(int I=0;I<7;++I) M=J::Toggle(M,Active,I);
 if(J::Count(M)!=7||J::Toggle(M,Active,7)!=M) return false;
 M=J::Toggle(M,Active,2);
 M=J::Toggle(M,Active,26);
 return J::Count(M)==7&&(M&J::Bit(26))&&!(M&J::Bit(2))
   &&J::EntryAt(Active,8)==26&&J::EntryAt(Active,9)==-1
   &&J::Count(J::Toggle(M,Active&~J::Bit(0),-1))==6;
}
static_assert(ManualTracking(), "Manual tracking is bounded, independent of accepted quests.");
static_assert(J::PageRows(240.f)==4 && J::PageRows(1080.f)==17);
static_assert(J::PageStart(16,4)==16 && J::PageStart(26,17)==17);
constexpr bool MoreAcceptedThanTracked() {
 int States[J::ResidentCount]{};
 for (int I=0; I<J::ResidentCount; ++I) States[I]=1;
 const auto Before=J::Summarize(States,J::ResidentCount,25);
 if (Before.Active!=15 || Before.TrackedCount!=7) return false;
 const int First=Before.TrackedIndices[0];
 States[First]=2;
 const auto After=J::Summarize(States,J::ResidentCount,25);
 if (After.Active!=14 || After.Completed!=1 || After.TrackedCount!=7) return false;
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
