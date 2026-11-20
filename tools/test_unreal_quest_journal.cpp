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


constexpr bool CheckSevenQuestSlots() {
 int State[ResidentCount]{};
 for (const int I : {1, 2, 3, 4, 5, 7, 8, 9, 10}) State[I] = 1;
 State[6] = 2; // Completed, never occupies a slot.
 const auto S = Summarize(State, ResidentCount, 10);
 return S.Active == 9 && S.Completed == 1 && S.Ready == 3
   && S.TrackedCount == MaxTracked && S.TrackedIndex == 1
   && S.TrackedIndices[0] == 1 && S.TrackedIndices[1] == 5
   && S.TrackedIndices[2] == 9 && S.TrackedIndices[3] == 2
   && S.TrackedIndices[4] == 3 && S.TrackedIndices[5] == 4
   && S.TrackedIndices[6] == 7;
}
static_assert(MaxTracked == 7, "Exactly seven quest slots supported.");
static_assert(CheckSevenQuestSlots(), "Ready quests must fill seven slots first.");
constexpr bool CheckNoActiveSlots() {
 int State[ResidentCount]{};
 State[0] = 2;
 const auto S = Summarize(State, ResidentCount, 100);
 if (S.Active != 0 || S.Completed != 1 || S.TrackedCount != 0
     || S.TrackedIndex != -1) return false;
 for (int I = 0; I < MaxTracked; ++I)
  if (S.TrackedIndices[I] != -1) return false;
 return true;
}
static_assert(CheckNoActiveSlots(), "Completed quests must not occupy slots.");

// Native-free layout tests: track seven; show the full seven at 720p+,
// and a safe subset at 420p/480p without overlapping the lower-left orb.
namespace QuestLayout = MoonfallQuestLayout;
static_assert(QuestLayout::MaxRows == MaxTracked);
constexpr int QuestKeys[7] = {1, 3, 10, 24, 50, 52, 72};
constexpr int SameQuestKeys[7] = {1, 3, 10, 24, 50, 52, 72};
constexpr int TurnInReady[7] = {1, 3, 11, 24, 50, 52, 72};
constexpr int QuestReordered[7] = {3, 1, 10, 24, 50, 52, 72};
constexpr int QuestCompleted[7] = {1, 3, 10, 24, 50, 52, 0};
constexpr int InvalidQuestKeys[7] = {1, 3, 10, 128, 50, 52, 72};
static_assert(QuestLayout::SelectionSignature(QuestKeys, 7) > 0ull);
static_assert(QuestLayout::SelectionSignature(QuestKeys, 7)
    == QuestLayout::SelectionSignature(SameQuestKeys, 7));
static_assert(QuestLayout::SelectionSignature(QuestKeys, 7)
    != QuestLayout::SelectionSignature(TurnInReady, 7));
static_assert(QuestLayout::SelectionSignature(QuestKeys, 7)
    != QuestLayout::SelectionSignature(QuestReordered, 7));
static_assert(QuestLayout::SelectionSignature(QuestKeys, 7)
    != QuestLayout::SelectionSignature(QuestCompleted, 6));
static_assert(QuestLayout::SelectionSignature(QuestKeys, 6)
    != QuestLayout::SelectionSignature(QuestKeys, 7));
static_assert(QuestLayout::SelectionSignature(InvalidQuestKeys, 7) == 0ull);
static_assert(QuestLayout::SelectionSignature(nullptr, 7) == 0ull);
static_assert(QuestLayout::NormalizePageStart(-1, 3, 7) == 0);
static_assert(QuestLayout::NormalizePageStart(6, 3, 7) == 6);
static_assert(QuestLayout::NormalizePageStart(6, 7, 7) == 0);
static_assert(QuestLayout::NextPageStart(0, 3, 7) == 3);
static_assert(QuestLayout::NextPageStart(3, 3, 7) == 6);
static_assert(QuestLayout::NextPageStart(6, 3, 7) == 0);
static_assert(QuestLayout::NextPageStart(0, 4, 7) == 4);
static_assert(QuestLayout::NextPageStart(4, 4, 7) == 0);
static_assert(QuestLayout::NextPageStart(0, 7, 7) == 0);
static_assert(QuestLayout::NextPageStart(6, 3, 5) == 0);
static_assert(QuestLayout::Compute(420, 7, 600).Top == 18);
static_assert(QuestLayout::Compute(420, 7, 600).VisibleRows == 3);
static_assert(QuestLayout::Compute(420, 7, 600).PanelHeight == 265);
static_assert(QuestLayout::Compute(480, 7, 640).VisibleRows == 4);
static_assert(QuestLayout::Compute(520, 7, 760).VisibleRows == 4);
static_assert(QuestLayout::Compute(720, 7, 1280).VisibleRows == 7);
static_assert(QuestLayout::Compute(720, 7, 1280).Top == 18);
static_assert(QuestLayout::Compute(720, 7, 1280).PanelHeight == 505);
static_assert(QuestLayout::Compute(1080, 7, 1920).Top == 78);
static_assert(QuestLayout::Compute(1080, 7, 1920).PanelHeight == 575);
static_assert(QuestLayout::Compute(1080, 7, 1920).RowStride == 70);
static_assert(QuestLayout::Compute(420, 0).PanelHeight == 124);
static_assert(QuestLayout::Compute(720, 0).PanelHeight == 124);
static_assert(QuestLayout::Compute(420, -2).VisibleRows == 0);
static_assert(QuestLayout::ValidRowSpacing(QuestLayout::Compute(420, 7, 600)));
static_assert(QuestLayout::ValidRowSpacing(QuestLayout::Compute(1080, 7, 1920)));
static_assert(QuestLayout::ClearsHealthOrb(600, 420, 7));
static_assert(QuestLayout::ClearsHealthOrb(640, 480, 7));
static_assert(QuestLayout::ClearsHealthOrb(760, 520, 7));
static_assert(QuestLayout::ClearsHealthOrb(1280, 720, 7));
static_assert(QuestLayout::ClearsHealthOrb(1920, 1080, 7));
static_assert(QuestLayout::ClearsHealthOrb(3840, 2160, 7));

int main() {
 for (int width : {600, 640, 760, 1280, 1920, 3840}) {
  for (int height : {420, 480, 519, 520, 720, 1080, 2160}) {
   for (int rows=1; rows<=QuestLayout::MaxRows; ++rows) {
    const auto metrics=QuestLayout::Compute(height, rows, width);
    assert(QuestLayout::ValidRowSpacing(metrics));
    assert(QuestLayout::ClearsHealthOrb(width, height, rows));
    assert(metrics.VisibleRows>=0 && metrics.VisibleRows<=rows);
    const int pageSize=metrics.VisibleRows;
    int pageStart=0;
    bool Seen[7]{};
    for (int page=0; page<7 && pageSize>0; ++page) {
     const int available=(pageStart+pageSize<=rows?pageSize:rows-pageStart);
     for(int I=0; I<available; ++I) Seen[pageStart+I]=true;
     pageStart=QuestLayout::NextPageStart(pageStart,pageSize,rows);
     if(pageStart==0) break;
    }
    for (int I=0; I<rows; ++I) assert(Seen[I]);
    if (height>=720) assert(metrics.VisibleRows==rows);
    assert(metrics.RowHeight+4==metrics.RowStride);
    assert(metrics.PanelHeight==85+metrics.RowStride*metrics.VisibleRows);
   }
  }
 }
 // A ready-state bit flip or a newly tracked contract must invalidate an
 // earlier compact page even if the number of accepted quests stays seven.
 for (int Count=1; Count<=QuestLayout::MaxRows; ++Count) {
  int Keys[7]{1,3,5,7,9,11,13};
  const auto Before=QuestLayout::SelectionSignature(Keys, Count);
  assert(Before != 0ull);
  for (int Row=0; Row<Count; ++Row) {
   Keys[Row] ^= 1; // Toggle contract ready/not-ready bit.
   assert(QuestLayout::SelectionSignature(Keys,Count)!=Before);
   Keys[Row] ^= 1;
   assert(QuestLayout::SelectionSignature(Keys,Count)==Before);
  }
 }
 for (int I=0; I<ResidentCount; ++I) {
  const auto& Resident = Residents[I];
  assert(Resident.Id && *Resident.Id && Resident.Name && *Resident.Name);
  assert(Resident.QuestTitle && *Resident.QuestTitle);
  // Title length stays bounded for the compact seven-slot Canvas HUD.
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
   for(int Slot=1; Slot<MaxTracked; ++Slot) assert(result.TrackedIndices[Slot]==-1);
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
