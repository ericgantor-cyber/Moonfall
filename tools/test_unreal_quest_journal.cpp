// Pure C++17 bounded journal regression. No Unreal SDK required.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestJournal.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestXP.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallQuestLayout.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace MoonfallQuestJournal;
static_assert(ResidentCount == 25);
static_assert(Cost(-1) == 0 && Cost(25) == 0);
static_assert(Cost(0) == 10); // Guide override
static_assert(Cost(1) == 10 && Cost(2) == 15 && Cost(3) == 20);
static_assert(Cost(4) == 25 && Cost(5) == 10);
static_assert(Cost(13) == 15 && Cost(24) == 25);

constexpr bool CheckReadyFirst() {
 int State[ResidentCount]{};
 State[4] = 1;  // Anvilkeeper: cost 25, not ready.
 State[5] = 1;  // Pip: cost 10, ready.
 State[13] = 2; // Regional contract already completed.
 const auto Summary = Summarize(State, ResidentCount, 10);
 return Summary.Active == 2 && Summary.Completed == 1 &&
   Summary.Ready == 1 && Summary.TrackedIndex == 5;
}
static_assert(CheckReadyFirst(), "Ready contracts must outrank earlier unfinished ones.");
constexpr bool CheckStableOrder() {
 int State[ResidentCount]{};
 State[5] = 1;
 State[7] = 1;
 const auto S = Summarize(State, ResidentCount, 30);
 return S.TrackedIndex == 5 && S.Ready == 2;
}
static_assert(CheckStableOrder(), "Equal priority retains stable catalog order.");


constexpr bool CheckThreeQuestSlots() {
 int State[ResidentCount]{};
 State[1] = 1; // Ready at 10.
 State[2] = 1; // Needs 15.
 State[3] = 1; // Needs 20.
 State[4] = 1; // Needs 25.
 State[5] = 1; // Ready at 10.
 State[6] = 2; // Completed, never occupies a slot.
 const auto S = Summarize(State, ResidentCount, 10);
 return S.Active == 5 && S.Completed == 1 && S.Ready == 2
   && S.TrackedCount == MaxTracked && S.TrackedIndex == 1
   && S.TrackedIndices[0] == 1 && S.TrackedIndices[1] == 5
   && S.TrackedIndices[2] == 2;
}
static_assert(MaxTracked == 3, "Exactly three quest slots visible.");
static_assert(CheckThreeQuestSlots(), "Ready quests must fill three slots first.");
constexpr bool CheckNoActiveSlots() {
 int State[ResidentCount]{};
 State[0] = 2;
 const auto S = Summarize(State, ResidentCount, 100);
 return S.Active == 0 && S.Completed == 1 && S.TrackedCount == 0
   && S.TrackedIndex == -1
   && S.TrackedIndices[0] == -1 && S.TrackedIndices[1] == -1
   && S.TrackedIndices[2] == -1;
}
static_assert(CheckNoActiveSlots(), "Completed quests must not occupy slots.");

// Pure compile-time viewport policy. Every row must retain distinct title,
// resident, reward and progress lines, with three slots clearing the HP orb.
namespace QuestLayout = MoonfallQuestLayout;
static_assert(QuestLayout::Compute(420, 3).Top == 18);
static_assert(QuestLayout::Compute(420, 3).PanelHeight == 265);
static_assert(QuestLayout::Compute(420, 3).RowStride == 60);
static_assert(QuestLayout::Compute(420, 3).RowHeight == 56);
static_assert(QuestLayout::Compute(420, 3).ProgressOffset == 52);
static_assert(QuestLayout::Compute(480, 3).RowStride == 60);
static_assert(QuestLayout::Compute(519, 3).RowStride == 60);
static_assert(QuestLayout::Compute(520, 3).Top == 78);
static_assert(QuestLayout::Compute(520, 3).PanelHeight == 295);
static_assert(QuestLayout::Compute(520, 3).RowStride == 70);
static_assert(QuestLayout::Compute(520, 3).RowHeight == 66);
static_assert(QuestLayout::Compute(520, 3).ProgressOffset == 62);
static_assert(QuestLayout::Compute(720, 3).RowStride == 70);
static_assert(QuestLayout::Compute(420, 0).PanelHeight == 124);
static_assert(QuestLayout::Compute(720, 0).PanelHeight == 124);
static_assert(QuestLayout::Compute(720, 7).PanelHeight == 295);
static_assert(QuestLayout::Compute(420, -2).PanelHeight == 124);
static_assert(QuestLayout::ValidRowSpacing(QuestLayout::Compute(420, 3)));
static_assert(QuestLayout::ValidRowSpacing(QuestLayout::Compute(520, 3)));
static_assert(QuestLayout::ClearsHealthOrb(600, 420, 3));
static_assert(QuestLayout::ClearsHealthOrb(640, 480, 3));
static_assert(QuestLayout::ClearsHealthOrb(760, 520, 3));
static_assert(QuestLayout::ClearsHealthOrb(1280, 720, 3));
static_assert(QuestLayout::ClearsHealthOrb(1920, 1080, 3));
static_assert(QuestLayout::ClearsHealthOrb(3840, 2160, 3));

int main() {
 for (int width : {600, 640, 760, 1280, 1920, 3840}) {
  for (int height : {420, 480, 519, 520, 720, 1080, 2160}) {
   for (int rows=1; rows<=QuestLayout::MaxRows; ++rows) {
    const auto metrics=QuestLayout::Compute(height, rows);
    assert(QuestLayout::ValidRowSpacing(metrics));
    assert(QuestLayout::ClearsHealthOrb(width, height, rows));
    assert(metrics.RowHeight+4==metrics.RowStride);
    assert(metrics.PanelHeight==85+metrics.RowStride*rows);
   }
  }
 }
 for (int I=0; I<ResidentCount; ++I) {
  const auto& Resident = Residents[I];
  assert(Resident.Id && *Resident.Id && Resident.Name && *Resident.Name);
  assert(Resident.QuestTitle && *Resident.QuestTitle);
  // Title length stays bounded for the compact three-row Canvas HUD.
  assert(std::strlen(Resident.QuestTitle) <= 26);
  assert(Resident.Location && *Resident.Location);
  assert(Resident.Race >= 0 && Resident.Race <= 11);
  assert(Cost(I) >= 10 && Cost(I) <= 25);
  assert(MoonfallQuestXP::Reward(Cost(I), 1) > 0);
  for(int J=I+1;J<ResidentCount;++J) {
   assert(std::strcmp(Resident.Id, Residents[J].Id) != 0);
   assert(std::strcmp(Resident.QuestTitle, Residents[J].QuestTitle) != 0);
  }
 }
 for(int salvage=0;salvage<=40;++salvage) {
  for(int tracked=0;tracked<ResidentCount;++tracked) {
   int States[ResidentCount]{};
   States[tracked]=1;
   auto result=Summarize(States,ResidentCount,salvage);
   assert(result.Active==1 && result.TrackedIndex==tracked);
   assert(result.TrackedCount==1 && result.TrackedIndices[0]==tracked);
   assert(result.TrackedIndices[1]==-1 && result.TrackedIndices[2]==-1);
   assert(result.Ready==(salvage>=Cost(tracked) ? 1 : 0));
   States[tracked]=2;
   result=Summarize(States,ResidentCount,salvage);
   assert(result.Active==0 && result.Completed==1 && result.TrackedIndex==-1);
   assert(result.TrackedCount==0);
  }
 }
 int Bad[ResidentCount]{};
 Bad[0]=1;
 assert(Summarize(Bad,ResidentCount-1,10).Active==0);
 assert(Summarize(nullptr,ResidentCount,10).Active==0);
 assert(Summarize(Bad,ResidentCount,-1).Active==0);
 std::cout << "25-resident quest journal selection and catalog passed\n";
}
