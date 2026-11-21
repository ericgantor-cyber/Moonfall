// Native-free regression suite for Moonfall's six-act, 42-beat storyline.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallChronicle.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIsleChronicle.h"
#include <cassert>
#include <cstring>
#include <iostream>
namespace S = MoonfallChronicle;

static_assert(S::ActCount == 6 && S::BeatsPerAct == 7);
static_assert(S::TotalBeats == 42 && S::DungeonsPerAct == 5);
static_assert(S::BeatState(0, 0, 0u, 0u) == S::EBeatState::Uncovered);
static_assert(S::BeatState(0, 1, 0u, 0u) == S::EBeatState::Hidden);
static_assert(S::BeatState(0, 1, 1u, 0u) == S::EBeatState::Uncovered);
static_assert(S::BeatState(0, 1, 1u, 1u) == S::EBeatState::Cleared);
static_assert(S::BeatState(0, 6, 31u, 30u) == S::EBeatState::Hidden);
static_assert(S::BeatState(0, 6, 31u, 31u) == S::EBeatState::Uncovered);
static_assert(S::BeatState(0, 6, 0u, 1u) == S::EBeatState::Hidden);
static_assert(S::Summarize(0, 0u, 0u).Revealed == 1);
static_assert(S::Summarize(0, 1u, 0u).Revealed == 2);
static_assert(S::Summarize(0, 1u, 0u).FeaturedBeat == 1);
static_assert(S::Summarize(0, 31u, 31u).Revealed == 7);
static_assert(S::Summarize(0, 31u, 31u).FeaturedBeat == 6);
static_assert(!S::Summarize(-1, 0u, 0u).Valid);
static_assert(!S::Summarize(6, 0u, 0u).Valid);
static_assert(!S::ValidProgress(0u, 1u));
static_assert(!S::ValidProgress(1u << 30, 0u));
static_assert(S::RevealedBeat(2, 5, 0u, 0u) == nullptr);

namespace I = MoonfallIsleChronicle;
constexpr bool EqualText(const char* A, const char* B) {
 while (*A && *B) {
  if (*A != *B) return false;
  ++A;
  ++B;
 }
 return *A == *B;
}
static_assert(I::ChapterCount == 7);
static_assert(I::NextReadableChapter(-1,1,false,false,{0,0,0,0,0},false)==-1);
static_assert(I::NextReadableChapter(-1,1,true,false,{0,0,0,0,0},false)==0);
static_assert(I::NextReadableChapter(0,1,true,false,{0,0,0,0,0},false)==0);
static_assert(I::NextReadableChapter(0,1,true,true,{2,0,0,0,0},false)==1);
static_assert(I::NextReadableChapter(1,-1,true,true,{2,0,0,0,0},false)==0);
static_assert(I::NextReadableChapter(5,1,true,true,{2,2,2,2,2},true)==6);
static_assert(I::NextReadableChapter(6,1,true,true,{2,2,2,2,2},true)==0);
static_assert(I::NextReadableChapter(0,-1,true,true,{2,2,2,2,2},true)==6);

static_assert(EqualText(I::NextObjective(false, false, {0,0,0,0,0}, false),
                          "Discover an island shrine"));
static_assert(EqualText(I::NextObjective(true, false, {0,0,0,0,0}, false),
                          "Find Mara Vesper at the harbor"));
static_assert(EqualText(I::NextObjective(true, true, {2,1,2,2,2}, true),
                          "Finish Corvin's salvage contract"));
static_assert(EqualText(I::NextObjective(true, true, {0,1,0,0,0}, false),
                          "Finish Corvin's salvage contract"));
static_assert(EqualText(I::NextObjective(true, true, {0,0,0,1,1}, false),
                          "Finish Rook's salvage contract"));
static_assert(EqualText(I::NextObjective(true, false, {2,2,2,2,2}, true),
                          "Discover the Blackbell shrine"));
static_assert(EqualText(I::NextObjective(true, true, {2,2,2,2,2}, false),
                          "Complete the six mainland acts"));
static_assert(EqualText(I::NextObjective(true, true, {2,2,2,2,2}, true),
                          "Seventh Witness unlocked"));
static_assert(EqualText(I::NextObjective(true, true, {-1,2,2,2,2}, true),
                          "Island records unavailable"));
static_assert(I::ChapterState(0, false, false, {0,0,0,0,0}, false)
  == I::EState::Hidden);
static_assert(I::ChapterState(0, true, false, {0,0,0,0,0}, false)
  == I::EState::Uncovered);
static_assert(I::ChapterState(1, true, false, {1,0,0,0,0}, false)
  == I::EState::Uncovered);
static_assert(I::ChapterState(1, true, false, {2,0,0,0,0}, false)
  == I::EState::Cleared);
static_assert(I::ChapterState(6, true, true, {2,2,2,2,2}, false)
  == I::EState::Hidden);
static_assert(I::ChapterState(6, true, true, {2,2,2,2,2}, true)
  == I::EState::Uncovered);
static_assert(I::Summarize(true, true, {2,2,2,2,2}, true).Revealed == 7);
static_assert(I::Summarize(true, false, {2,2,2,2,2}, true).Revealed == 6);
static_assert(!I::Summarize(true, false, {2,2,2,2,2}, true).EpilogueReady);
static_assert(I::Summarize(false, true, {1,0,1,0,0}, false).Revealed == 3);
static_assert(I::Summarize(false, true, {1,0,1,0,0}, false).FeaturedChapter == 1);
static_assert(I::Summarize(true, true, {2,0,1,0,0}, false).FeaturedChapter == 3);
static_assert(I::Summarize(false, false, {2,2,2,2,2}, true).CompletedContracts == 0);
static_assert(!I::Summarize(true, true, {-1,0,0,0,0}, true).Valid);
static_assert(I::RevealedChapter(3, true, true, {0,0,0,0,0}, true) == nullptr);

int main() {
 int IsleEntries = 0;
 for (int Chapter = 0; Chapter < I::ChapterCount; ++Chapter) {
  const auto& Entry = I::Chapters[Chapter];
  assert(std::strlen(Entry.Id) >= 7);
  assert(std::strlen(Entry.Title) >= 8);
  assert(std::strlen(Entry.Lore) >= 90);
  for (int Other = 0; Other < Chapter; ++Other) {
   assert(std::strcmp(Entry.Id, I::Chapters[Other].Id) != 0);
   assert(std::strcmp(Entry.Title, I::Chapters[Other].Title) != 0);
  }
  ++IsleEntries;
 }
 assert(IsleEntries == 7);
 // Exhaust valid combinations to protect new chapter spoilers and ledger.
 for (int Q0 = 0; Q0 <= 2; ++Q0)
 for (int Q1 = 0; Q1 <= 2; ++Q1)
 for (int Q2 = 0; Q2 <= 2; ++Q2)
 for (int Q3 = 0; Q3 <= 2; ++Q3)
 for (int Q4 = 0; Q4 <= 2; ++Q4) {
  const int Quests[5] = {Q0,Q1,Q2,Q3,Q4};
  const auto NoIsland = I::Summarize(false,false,Quests,false);
  assert(NoIsland.Revealed == 0);
  for (int Ch=0; Ch<I::ChapterCount; ++Ch)
   assert(I::RevealedChapter(Ch,false,false,Quests,false)==nullptr);
  const auto Started = I::Summarize(true,true,Quests,false);
  assert(Started.Valid);
  assert(!Started.EpilogueReady);
  assert(I::RevealedChapter(6,true,true,Quests,false)==nullptr);
  int Completed = 0, StartedQuests = 0;
  for(int j=0; j<5; ++j) {
   Completed += Quests[j] == 2;
   StartedQuests += Quests[j] >= 1;
   assert((I::RevealedChapter(j+1,true,true,Quests,false)!=nullptr)
      ==(Quests[j]>=1));
  }
  assert(Started.CompletedContracts==Completed);
  assert(Started.Revealed==1+StartedQuests);
  // Every navigated page must be revealed, even for sparse, out-of-order
  // contract completions and both directions at the end of the list.
  for (int Current=-1; Current<I::ChapterCount; ++Current)
   for (int Direction=-1; Direction<=1; ++Direction) {
    const int Next=I::NextReadableChapter(
      Current,Direction,true,true,Quests,false);
    assert(Next>=0 && Next<I::ChapterCount);
    assert(I::RevealedChapter(Next,true,true,Quests,false)!=nullptr);
    if (Direction!=0 && Current>=0) {
     int Cursor=Current;
     int Expected=-1;
     for (int Attempt=0;Attempt<I::ChapterCount;++Attempt) {
      Cursor=(Cursor+Direction+I::ChapterCount)%I::ChapterCount;
      if (I::RevealedChapter(Cursor,true,true,Quests,false)!=nullptr) {
       Expected=Cursor;
       break;
      }
     }
     assert(Next==Expected);
    }
   }
  // Every one of 243 valid contract combinations: current work wins over
  // a new lead. Neither state can expose the finale prematurely.
  constexpr const char* ActiveHints[5] = {
   "Finish Mara's salvage contract", "Finish Corvin's salvage contract",
   "Finish Veyra's salvage contract", "Finish Rook's salvage contract",
   "Finish Elias's salvage contract"
  };
  constexpr const char* NewHints[5] = {
   "Find Mara Vesper at the harbor", "Find Brother Corvin at Blackbell",
   "Find Veyra Ash in the graves", "Find Rook Thorn at Duskwatch",
   "Find Elias Briar in the grove"
  };
  int FirstActive = -1, FirstAvailable = -1;
  for (int j=0; j<5; ++j) {
   if (Quests[j] == 1 && FirstActive < 0) FirstActive = j;
   if (Quests[j] == 0 && FirstAvailable < 0) FirstAvailable = j;
  }
  const char* Hint = I::NextObjective(true,true,Quests,false);
  if (FirstActive >= 0) assert(std::strcmp(Hint,ActiveHints[FirstActive])==0);
  else if (FirstAvailable >= 0) assert(std::strcmp(Hint,NewHints[FirstAvailable])==0);
  else assert(std::strcmp(Hint,"Complete the six mainland acts")==0);
  const auto WithMainland = I::Summarize(true,true,Quests,true);
  assert(WithMainland.EpilogueReady==(Completed==5));
 }
 int Total = 0;
 for (int Act=0; Act<S::ActCount; ++Act) {
  assert(std::strlen(S::RegionNames[Act]) >= 5);
  assert(std::strlen(S::ActNames[Act]) >= 8);
  for (int Beat=0; Beat<S::BeatsPerAct; ++Beat) {
   ++Total;
   const auto& Entry=S::Acts[Act][Beat];
   assert(std::strlen(Entry.Id) >= 7);
   assert(std::strlen(Entry.Title) >= 8);
   assert(std::strlen(Entry.Title) <= 38);
   assert(std::strlen(Entry.Lore) >= 90);
   assert(std::strlen(Entry.Lore) <= 300);
   for (int OtherAct=0; OtherAct<S::ActCount; ++OtherAct) {
    for (int OtherBeat=0; OtherBeat<S::BeatsPerAct; ++OtherBeat) {
     if (OtherAct == Act && OtherBeat == Beat) continue;
     const auto& Other=S::Acts[OtherAct][OtherBeat];
     assert(std::strcmp(Entry.Id, Other.Id) != 0);
     assert(std::strcmp(Entry.Title, Other.Title) != 0);
    }
   }
  }
 }
 assert(Total == 42);
 // Every reachable pattern of discovered dungeons and defeated bosses:
 // a boss can only be defeated when that dungeon was discovered.
 for (int Act=0; Act<S::ActCount; ++Act) {
  for (unsigned Found=0; Found<32u; ++Found) {
   for (unsigned Won=0; Won<32u; ++Won) {
    const std::uint32_t FoundMask=static_cast<std::uint32_t>(Found) << (Act*5);
    const std::uint32_t WonMask=static_cast<std::uint32_t>(Won) << (Act*5);
    const auto Summary=S::Summarize(Act, FoundMask, WonMask);
    if ((Won & ~Found) != 0u) {
     assert(!Summary.Valid);
     for (int Beat=0; Beat<7; ++Beat)
      assert(S::RevealedBeat(Act,Beat,FoundMask,WonMask)==nullptr);
     continue;
    }
    const int FoundCount=S::PopCount5(Found);
    const int WonCount=S::PopCount5(Won);
    assert(Summary.Valid);
    assert(Summary.BossesDefeated==WonCount);
    assert(Summary.Revealed==1+FoundCount+(WonCount==5?1:0));
    assert(Summary.Complete==(WonCount==5));
    assert(Summary.FeaturedBeat>=0 && Summary.FeaturedBeat<7);
    assert(S::RevealedBeat(Act,Summary.FeaturedBeat,FoundMask,WonMask)!=nullptr);
    assert(S::RevealedBeat(Act,0,FoundMask,WonMask)!=nullptr);
    for (int Dungeon=0; Dungeon<5; ++Dungeon) {
     const auto State=S::BeatState(Act,Dungeon+1,FoundMask,WonMask);
     const bool Discovered=(Found&(1u<<Dungeon))!=0u;
     const bool Defeated=(Won&(1u<<Dungeon))!=0u;
     assert((State!=S::EBeatState::Hidden)==Discovered);
     assert((State==S::EBeatState::Cleared)==Defeated);
     assert((S::RevealedBeat(Act,Dungeon+1,FoundMask,WonMask)!=nullptr)==Discovered);
    }
    assert((S::BeatState(Act,6,FoundMask,WonMask)!=S::EBeatState::Hidden)
           ==(WonCount==5));
   }
  }
 }
 // World-wide progression must stay consistent with every regional view.
 // Test all valid five-dungeon patterns independently in each of six regions.
 for (int Act=0; Act<S::ActCount; ++Act) {
  for (unsigned Found=0; Found<32u; ++Found) {
   for (unsigned Won=0; Won<32u; ++Won) {
    const std::uint32_t FoundMask=static_cast<std::uint32_t>(Found) << (Act*5);
    const std::uint32_t WonMask=static_cast<std::uint32_t>(Won) << (Act*5);
    const auto World=S::SummarizeWorld(FoundMask,WonMask);
    if ((Won & ~Found) != 0u) {
     assert(!World.Valid);
     assert(World.DungeonsDiscovered==0);
     assert(World.BossesDefeated==0);
     assert(World.ActsCompleted==0);
     assert(World.ChaptersRevealed==0);
     assert(!World.ChronicleComplete);
     continue;
    }
    const int FoundCount=S::PopCount5(Found);
    const int WonCount=S::PopCount5(Won);
    assert(World.Valid);
    assert(World.DungeonsDiscovered==FoundCount);
    assert(World.BossesDefeated==WonCount);
    assert(World.ActsCompleted==(WonCount==5?1:0));
    assert(World.ChaptersRevealed==6+FoundCount+(WonCount==5?1:0));
    assert(!World.ChronicleComplete);
   }
  }
 }
 const auto CompleteWorld=S::SummarizeWorld(S::ValidDungeonBits,S::ValidDungeonBits);
 assert(CompleteWorld.Valid && CompleteWorld.DungeonsDiscovered==30);
 assert(CompleteWorld.BossesDefeated==30 && CompleteWorld.ActsCompleted==6);
 assert(CompleteWorld.ChaptersRevealed==42 && CompleteWorld.ChronicleComplete);
 const auto FreshWorld=S::SummarizeWorld(0u,0u);
 assert(FreshWorld.Valid && FreshWorld.ChaptersRevealed==6);
 assert(!FreshWorld.ChronicleComplete);
 assert(!S::SummarizeWorld(1u<<30,0u).Valid);
 assert(!S::SummarizeWorld(0u,1u<<30).Valid);
 // Discover dungeon 5 first, without revealing dungeon 1 through 4.
 const auto NonLinear=S::Summarize(3, (1u<<19), 0u);
 assert(NonLinear.Valid && NonLinear.FeaturedBeat==5);
 assert(S::RevealedBeat(3,1,1u<<19,0u)==nullptr);
 assert(S::RevealedBeat(3,5,1u<<19,0u)!=nullptr);
 std::cout<<"Moonfall six acts / 42 chapters / discovery-gated lore passed\n";
}
