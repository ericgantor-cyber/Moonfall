#pragma once
// An original Gothic Isle story bridge to the existing Six Roads Chronicle.
// Read only: uses previously saved shrine discovery and resident quest states.
// Does not extend the six-act/42-beat dungeon save masks or invent a boss kill.
#include "MoonfallChronicle.h"

namespace MoonfallIsleChronicle {
inline constexpr int ChapterCount = 7;
inline constexpr const char* Title = "The Seventh Witness";
struct FChapter { const char* Id; const char* Title; const char* Lore; };
inline constexpr FChapter Chapters[ChapterCount] = {
 {"isle_ch1", "The Nightferry's Wake",
  "Raven's Landing receives travelers whom the six roads have forgotten. Beneath the dock bell lies a ledger of boats that sailed after the moon fractured, though nobody remembers seeing them return."},
 {"isle_ch2", "Mara's Missing Tide",
  "Mara Vesper has kept the harbor's ruined manifests. One ship bore a message copied from every mainland capital, each copy arriving on the same impossible night."},
 {"isle_ch3", "The Blackbell Testament",
  "Brother Corvin guards a bell that has not been rung since the catastrophe. Its inner rim names six witnesses and leaves a seventh place empty. The absence may be deliberate."},
 {"isle_ch4", "Names Beneath the Ash",
  "Veyra Ash protects graves without bodies. Their memorials describe people who crossed the sea carrying records, not treasure, and were struck from the histories of their homes."},
 {"isle_ch5", "The Last Lantern",
  "Rook Thorn watches a coast where false beacons once guided ships onto the rocks. His surviving signal codes match the forged orders discovered in the Iron Foundry."},
 {"isle_ch6", "Roots of the Unsaid",
  "Elias Briar has found roots wrapped around fragments of a star chart. The markings echo Frostbound Reach, but point toward an island rather than an eclipse in the sky."},
 {"isle_ch7", "The Seventh Witness",
  "The harbor, bell, graves, watch and grove tell one story: the six mainland records were not the only testimony. The seventh witness must be heard before anyone judges the gods."}
};
enum class EState : int { Hidden = 0, Uncovered = 1, Cleared = 2 };
struct FProgress {
 bool Valid;
 int Revealed;
 int CompletedContracts;
 int FeaturedChapter;
 bool EpilogueReady;
};
constexpr bool ValidQuestState(int State) { return State >= 0 && State <= 2; }
constexpr bool ValidQuests(const int (&Quests)[5]) {
 for (int I = 0; I < 5; ++I)
  if (!ValidQuestState(Quests[I])) return false;
 return true;
}
constexpr bool AllContractsComplete(const int (&Quests)[5]) {
 for (int I = 0; I < 5; ++I)
  if (Quests[I] != 2) return false;
 return true;
}
constexpr EState ChapterState(int Chapter, bool HarborDiscovered,
 bool CathedralDiscovered, const int (&Quests)[5], bool MainlandComplete) {
 if (Chapter < 0 || Chapter >= ChapterCount || !ValidQuests(Quests))
  return EState::Hidden;
 // Island evidence stays unavailable on a fresh mainland save.
 if (!HarborDiscovered && !CathedralDiscovered)
  return EState::Hidden;
 if (Chapter == 0) return EState::Uncovered;
 if (Chapter == 6)
  return CathedralDiscovered && MainlandComplete && AllContractsComplete(Quests)
   ? EState::Uncovered : EState::Hidden;
 return Quests[Chapter - 1] == 2 ? EState::Cleared
   : Quests[Chapter - 1] == 1 ? EState::Uncovered : EState::Hidden;
}
constexpr FProgress Summarize(bool HarborDiscovered, bool CathedralDiscovered,
 const int (&Quests)[5], bool MainlandComplete) {
 if (!ValidQuests(Quests)) return {false, 0, 0, 0, false};
 // One validation and one five-contract pass per Atlas update, not seven
 // repeated ChapterState calls that each recheck the whole quest array.
 if (!HarborDiscovered && !CathedralDiscovered)
  return {true, 0, 0, 0, false};
 int Count = 1, Complete = 0, Featured = 0;
 for (int Index = 0; Index < 5; ++Index) {
  if (Quests[Index] >= 1) ++Count;
  if (Quests[Index] == 2) ++Complete;
  else if (Quests[Index] == 1 && Featured == 0) Featured = Index + 1;
 }
 const bool Epilogue = CathedralDiscovered && MainlandComplete && Complete == 5;
 if (Epilogue) {
  ++Count;
  Featured = 6;
 }
 return {true, Count, Complete, Featured, Epilogue};
}
// Single, spoiler-safe next action for the optional island storyline.
// Reads existing shrine and contract save data; grants no progression.
constexpr const char* NextObjective(bool HarborDiscovered,
 bool CathedralDiscovered, const int (&Quests)[5], bool MainlandComplete) {
 if (!ValidQuests(Quests)) return "Island records unavailable";
 if (!HarborDiscovered && !CathedralDiscovered)
  return "Discover an island shrine";
 // Prioritize a contract the player has already accepted over an
 // untouched NPC lead. Both orders are deterministic across save/reload.
 constexpr const char* TurnIns[5] = {
  "Finish Mara's salvage contract", "Finish Corvin's salvage contract",
  "Finish Veyra's salvage contract", "Finish Rook's salvage contract",
  "Finish Elias's salvage contract"
 };
 constexpr const char* Leads[5] = {
  "Find Mara Vesper at the harbor", "Find Brother Corvin at Blackbell",
  "Find Veyra Ash in the graves", "Find Rook Thorn at Duskwatch",
  "Find Elias Briar in the grove"
 };
 for (int Index = 0; Index < 5; ++Index)
  if (Quests[Index] == 1) return TurnIns[Index];
 for (int Index = 0; Index < 5; ++Index)
  if (Quests[Index] == 0) return Leads[Index];
 if (!CathedralDiscovered) return "Discover the Blackbell shrine";
 if (!MainlandComplete) return "Complete the six mainland acts";
 return "Read the Seventh Witness";
}
constexpr const FChapter* RevealedChapter(int Chapter, bool HarborDiscovered,
 bool CathedralDiscovered, const int (&Quests)[5], bool MainlandComplete) {
 return ChapterState(Chapter, HarborDiscovered, CathedralDiscovered,
   Quests, MainlandComplete) == EState::Hidden ? nullptr : &Chapters[Chapter];
}
// Reader navigation has no save fields and never selects a locked page.
// The current chapter survives if still visible; otherwise use the first
// available chapter. Page cycling wraps across only revealed evidence.
constexpr int NextReadableChapter(int Current, int Direction,
 bool HarborDiscovered, bool CathedralDiscovered,
 const int (&Quests)[5], bool MainlandComplete) {
 if (!ValidQuests(Quests) || (!HarborDiscovered && !CathedralDiscovered))
  return -1;
 if (Direction != -1 && Direction != 1) {
  return Current >= 0 && Current < ChapterCount
   && ChapterState(Current, HarborDiscovered, CathedralDiscovered,
      Quests, MainlandComplete) != EState::Hidden ? Current : 0;
 }
 int Cursor = Current >= 0 && Current < ChapterCount ? Current
   : Direction == 1 ? ChapterCount - 1 : 0;
 for (int Attempt = 0; Attempt < ChapterCount; ++Attempt) {
  Cursor = (Cursor + Direction + ChapterCount) % ChapterCount;
  if (ChapterState(Cursor, HarborDiscovered, CathedralDiscovered,
       Quests, MainlandComplete) != EState::Hidden)
   return Cursor;
 }
 return -1;
}
static_assert(ChapterCount == 7, "Seven original Gothic Isle chapters");
static_assert(MoonfallChronicle::TotalBeats == 42, "Keep mainland 42 beats");
static_assert(Summarize(false, false, {0, 0, 0, 0, 0}, false).Revealed == 0,
 "Fresh mainland saves do not reveal Gothic Isle");
static_assert(Summarize(true, false, {0, 0, 0, 0, 0}, false).Revealed == 1,
 "Harbor discovery reveals the prologue");
static_assert(Summarize(true, true, {2, 2, 2, 2, 2}, true).EpilogueReady,
 "All evidence and mainland completion reveal the final witness");
static_assert(!Summarize(true, true, {2, 2, 2, 2, 2}, false).EpilogueReady,
 "Isle epilogue cannot skip the mainland story");
static_assert(!Summarize(true, true, {3, 0, 0, 0, 0}, true).Valid,
 "Invalid quest state fails closed");
} // namespace MoonfallIsleChronicle
