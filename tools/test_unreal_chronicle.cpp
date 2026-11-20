// Native-free regression suite for Moonfall's six-act, 42-beat storyline.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallChronicle.h"
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

int main() {
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
 // Discover dungeon 5 first, without revealing dungeon 1 through 4.
 const auto NonLinear=S::Summarize(3, (1u<<19), 0u);
 assert(NonLinear.Valid && NonLinear.FeaturedBeat==5);
 assert(S::RevealedBeat(3,1,1u<<19,0u)==nullptr);
 assert(S::RevealedBeat(3,5,1u<<19,0u)!=nullptr);
 std::cout<<"Moonfall six acts / 42 chapters / discovery-gated lore passed\n";
}
