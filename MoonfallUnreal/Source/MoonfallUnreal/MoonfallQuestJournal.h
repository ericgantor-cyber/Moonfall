#pragma once
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
};
inline constexpr int ResidentCount = static_cast<int>(sizeof(Residents) / sizeof(Residents[0]));
static_assert(ResidentCount == 15, "Match the current resident save whitelist");

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
} // namespace MoonfallQuestJournal
