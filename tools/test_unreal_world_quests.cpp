// Native-free C++17 policy regression for 12 automatic regional world quests.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestJournal.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestLayout.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace W = MoonfallWorldQuest;
namespace J = MoonfallQuestJournal;
static_assert(W::GoalCount == 12 && W::ZoneCount == 6);
static_assert(W::MaxVisible == J::MaxTracked && J::MaxTracked == 3);
static_assert(W::Type(0)[0] == 'E' && W::Type(1)[0] == 'B');
constexpr bool FreshWorld() {
 int Seen[6]{}, Dead[6]{};
 const auto R = W::Select(Seen, Dead, 3);
 return R.Valid && R.Completed == 0 && R.Unfinished == 12
    && R.Count == 3 && R.Indices[0] == 0
    && R.Indices[1] == 2 && R.Indices[2] == 4;
}
static_assert(FreshWorld(), "Unvisited zones should offer three discovery goals.");
constexpr bool ZoneInProgress() {
 int Seen[6]{1, 0, 0, 0, 0, 0}, Dead[6]{};
 const auto R = W::Select(Seen, Dead, 3);
 return R.Valid && R.Count == 3 && R.Indices[0] == 0
    && R.Indices[1] == 1 && R.Indices[2] == 2;
}
static_assert(ZoneInProgress(), "Started regions prioritize discovery and boss quests.");
constexpr bool DoneStagesDisappear() {
 int Seen[6]{5, 2, 0, 0, 0, 0}, Dead[6]{5, 1, 0, 0, 0, 0};
 const auto R = W::Select(Seen, Dead, 3);
 return R.Valid && R.Completed == 2 && R.Unfinished == 10
    && R.Indices[0] == 2 && R.Indices[1] == 3 && R.Indices[2] == 4;
}
static_assert(DoneStagesDisappear(), "Completed quests must never reuse slots.");
constexpr bool AllFinished() {
 int Seen[6]{5,5,5,5,5,5}, Dead[6]{5,5,5,5,5,5};
 const auto R = W::Select(Seen, Dead, 3);
 return R.Valid && R.Completed == 12 && R.Unfinished == 0 && R.Count == 0;
}
static_assert(AllFinished(), "12 complete regional quests return to exploration.");
constexpr bool RejectCorruption() {
 int Seen[6]{0,0,6,0,0,0}, Dead[6]{};
 const auto Wrong = W::Select(Seen, Dead, 3);
 Seen[2]=0; Dead[1]=1;
 const auto BossWithoutEntrance = W::Select(Seen, Dead, 3);
 return !Wrong.Valid && Wrong.Count == 0
   && !BossWithoutEntrance.Valid && BossWithoutEntrance.Count == 0
   && !W::Select(nullptr, Dead, 3).Valid
   && !W::Select(Seen, nullptr, 3).Valid
   && !W::Select(Seen, Dead, -1).Valid
   && !W::Select(Seen, Dead, 4).Valid;
}
static_assert(RejectCorruption(), "Bad saved counts must not produce a quest.");
int main() {
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
   for(int Slots=0; Slots<=3; ++Slots) {
    const auto R=W::Select(Seen,Dead,Slots);
    assert(R.Valid && R.Count<=Slots && R.Count<=3);
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
     const auto Layout=MoonfallQuestLayout::Compute(720,Slots);
     assert(MoonfallQuestLayout::ValidRowSpacing(Layout));
     assert(MoonfallQuestLayout::ClearsHealthOrb(1280,720,Slots));
    }
   }
  }
 }
 std::cout<<"12 regional world quests and three-slot blending policy passed\n";
}
