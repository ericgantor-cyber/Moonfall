#pragma once
#include <cstdint>
#include "MoonfallWorldQuest.h"
// Stable UI-only resident catalog and bounded quest selection rules.
// Does not spawn NPCs, change rewards, or alter persistent save formats.
// Match names/races to the existing native sanctuary and Crossroads authoring.
namespace MoonfallQuestJournal {
struct FResident {
 const char* Id;
 const char* Name;
 const char* QuestTitle;
 const char* Location;
 int Race;
};
inline constexpr FResident Residents[] = {
 {"CrossroadsGuide", "Wandering Guide", "Firstlight Provisions", "Shattered Sanctuary", 2},
 {"Alden", "Alden", "Watchtower Stockpile", "Crossroads", 0},
 {"Starweaver", "Starweaver", "Starlit Offerings", "Crossroads", 1},
 {"Leafwhisper", "Leafwhisper", "Grove Supply Run", "Crossroads", 2},
 {"Anvilkeeper", "Anvilkeeper", "Forge Reserves", "Crossroads", 3},
 {"PipHearthfoot", "Pip Hearthfoot", "Hearthfoot Supplies", "Crossroads", 4},
 {"EmberScout", "Ember Scout", "Ember Patrol Supplies", "Crossroads", 5},
 {"Geargrin", "Geargrin", "Tinker's Cache", "Crossroads", 6},
 {"Mossfang", "Mossfang", "Clansguard Provision", "Crossroads", 7},
 {"Birchroot", "Birchroot", "Rootwarden Stores", "Crossroads", 8},
 {"Duskveil", "Duskveil", "Duskbound Reagents", "Crossroads", 9},
 {"Silverwatch", "Silverwatch", "Moonwatch Supplies", "Crossroads", 10},
 {"Ashscale", "Ashscale", "Ashen Elder's Stock", "Crossroads", 11},
 {"FirstlightArchivist", "Archivist Veyra", "Archive Supplies", "Firstlight", 1},
 {"FirstlightWatchkeeper", "Warden Cael", "Watchkeeper Supplies", "Firstlight", 0},
 {"waystation_ashen_scout", "Cinder Runner", "Ashen Trail Supplies", "Emberwatch Camp", 5},
 {"waystation_ashen_artisan", "Ashforge Keeper", "Ashforge Stockpile", "Emberwatch Camp", 3},
 {"waystation_moonveil_scout", "Mira Fernstep", "Forest Trail Supplies", "Silverleaf Refuge", 2},
 {"waystation_moonveil_scholar", "Selune Whisper", "Moon-rune Research", "Silverleaf Refuge", 1},
 {"waystation_hollows_scout", "Nox Tunnelwatch", "Tunnel Lantern Supplies", "Deepstone Haven", 3},
 {"waystation_hollows_tinker", "Dara Flint", "Workshop Salvage", "Deepstone Haven", 6},
 {"waystation_foundry_scout", "Kora Spark", "Foundry Patrol Stock", "Sparkfall Relay", 6},
 {"waystation_foundry_tinker", "Mek Ironhand", "Ironworks Salvage", "Sparkfall Relay", 3},
 {"waystation_frost_scout", "Eira Froststep", "Frozen Trail Supplies", "Winterglass Lodge", 10},
 {"waystation_frost_scholar", "Orin Snowbind", "Frozen Archive", "Winterglass Lodge", 1},
 {"waystation_eclipse_scout", "Sable Nightroad", "Frontier Supplies", "Duskwatch Refuge", 9},
 {"waystation_eclipse_elder", "Vesper Rune", "Eclipse Remnants", "Duskwatch Refuge", 11},
};
inline constexpr int ResidentCount = static_cast<int>(sizeof(Residents) / sizeof(Residents[0]));
static_assert(ResidentCount == 27, "Match the current resident save whitelist");

// Match AMoonfallWorldNPC::GetContractSalvageCost, including the Guide exception.
constexpr int Cost(int Index) {
 return Index < 0 || Index >= ResidentCount ? 0
   : Index == 0 ? 10 : 10 + (Residents[Index].Race % 4) * 5;
}
// World tracker supports up to seven quests at once. Compact windows may
// display a smaller subset, but never discard the remaining tracked quests.
// Completed quests remain recorded in existing saves, not displayed as active.
inline constexpr int MaxTracked = 7;
struct FSummary {
 int Active = 0;
 int Completed = 0;
 int Ready = 0;
 int TrackedIndex = -1; // First visible row, for existing callers.
 int TrackedIndices[MaxTracked] = {-1, -1, -1, -1, -1, -1, -1};
 int TrackedCount = 0;
};
// State 1 is accepted, 2 completed. Fill up to seven slots, ready-to-turn-in
// first, then quests needing salvage. Within each group, catalog order is
// deterministic and independent of saved TMap iteration or game RNG.
constexpr FSummary Summarize(const int* States, int StateCount, int Salvage) {
 FSummary Result{};
 if (!States || StateCount != ResidentCount || Salvage < 0) return Result;
 for (int I = 0; I < ResidentCount; ++I) {
  if (States[I] == 2) { ++Result.Completed; continue; }
  if (States[I] == 1) {
   ++Result.Active;
   if (Salvage >= Cost(I)) ++Result.Ready;
  }
 }
 for (int Priority = 0; Priority < 2 && Result.TrackedCount < MaxTracked; ++Priority) {
  const bool bWantReady = Priority == 0;
  for (int I = 0; I < ResidentCount && Result.TrackedCount < MaxTracked; ++I) {
   if (States[I] != 1 || (Salvage >= Cost(I)) != bWantReady) continue;
   Result.TrackedIndices[Result.TrackedCount++] = I;
  }
 }
 if (Result.TrackedCount > 0) Result.TrackedIndex = Result.TrackedIndices[0];
 return Result;
}
// 100 accepted contracts, plus automatic regional goals. Completed contracts
// remain stored separately from the active limit. Keep capacity independent of content.
inline constexpr int MaxActive=100;
inline constexpr int MaxStoredRecords=256;
inline constexpr int EntryCount=ResidentCount+MoonfallWorldQuest::GoalCount;
inline constexpr int MaskCapacity=MaxStoredRecords+64;
static_assert(EntryCount<=MaskCapacity);
struct FMask {
 std::uint64_t Words[MaskCapacity/64]{};
 constexpr bool Has(int Id) const {
  return Id>=0&&Id<MaskCapacity&&(Words[Id/64]&(std::uint64_t(1)<<(Id%64)))!=0;
 }
 constexpr void Add(int Id) {
  if(Id>=0&&Id<MaskCapacity) Words[Id/64]|=std::uint64_t(1)<<(Id%64);
 }
 constexpr void Remove(int Id) {
  if(Id>=0&&Id<MaskCapacity) Words[Id/64]&=~(std::uint64_t(1)<<(Id%64));
 }
 constexpr bool operator==(const FMask& Other) const {
  for(int I=0;I<MaskCapacity/64;++I) if(Words[I]!=Other.Words[I])return false;
  return true;
 }
 constexpr bool operator!=(const FMask& Other) const {return !(*this==Other);}
 constexpr FMask operator&(const FMask& Other) const {
  FMask Out{};
  for(int I=0;I<MaskCapacity/64;++I)Out.Words[I]=Words[I]&Other.Words[I];
  return Out;
 }
};
constexpr int Count(FMask Mask) {
 int N=0;
 for(auto Word:Mask.Words) while(Word){Word&=Word-1;++N;}
 return N;
}
constexpr bool CanAccept(int Active) {return Active>=0&&Active<MaxActive;}
constexpr FMask Toggle(FMask Mask,FMask Active,int Id) {
 Mask=Mask&Active;
 if(!Active.Has(Id))return Mask;
 if(Mask.Has(Id))Mask.Remove(Id);
 else if(Count(Mask)<MaxTracked)Mask.Add(Id);
 return Mask;
}
constexpr int EntryAt(FMask Mask,int Row) {
 for(int I=0;I<MaskCapacity;++I)
  if(Mask.Has(I)&&Row--==0)return I;
 return -1;
}
constexpr int CountContracts(FMask Mask) {
 int N=0;
 for(int I=0;I<ResidentCount;++I) if(Mask.Has(I))++N;
 return N;
}
constexpr int PageRows(float Height) {
 // Clamp before converting to int; never convert infinity or NaN.
 if(!(Height>=240.f))return 0;
 const float PanelHeight=Height>=674.f?650.f:Height-24.f;
 return static_cast<int>((PanelHeight-92.f)/30.f);
}
constexpr int HitRow(float LocalY,int Rows) {
 if(!(LocalY>=0.f&&LocalY<Rows*30.f))return -1;
 const int Row=static_cast<int>(LocalY/30.f);
 return LocalY-Row*30.f<28.f?Row:-1;
}
constexpr int PageStart(int Selected,int Rows) {
 return Rows>0 && Selected>=0?(Selected/Rows)*Rows:0;
}
} // namespace MoonfallQuestJournal
