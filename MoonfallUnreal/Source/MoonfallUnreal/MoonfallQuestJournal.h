#pragma once
// Stable UI-only resident catalog and bounded quest selection rules.
// Does not spawn NPCs, change rewards, or alter persistent save formats.
// Match names/races to existing arena and regional-waystation authoring.
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
 {"waystation_ashen_scout", "Cinder Runner", "Cinder Road Supplies", "Emberwatch Camp", 5},
 {"waystation_ashen_artisan", "Ashforge Keeper", "Ashforge Materials", "Emberwatch Camp", 3},
 {"waystation_moonveil_scout", "Mira Fernstep", "Silverleaf Provisions", "Silverleaf Refuge", 2},
 {"waystation_moonveil_scholar", "Selune Whisper", "Runekeeper Supplies", "Silverleaf Refuge", 1},
 {"waystation_hollows_scout", "Nox Tunnelwatch", "Deepstone Stores", "Deepstone Haven", 3},
 {"waystation_hollows_tinker", "Dara Flint", "Tunnelwork Materials", "Deepstone Haven", 6},
 {"waystation_foundry_scout", "Kora Spark", "Foundry Patrol Stores", "Sparkfall Relay", 6},
 {"waystation_foundry_tinker", "Mek Ironhand", "Ironhand Components", "Sparkfall Relay", 3},
 {"waystation_frost_scout", "Eira Froststep", "Wintertrail Provisions", "Winterglass Lodge", 10},
 {"waystation_frost_scholar", "Orin Snowbind", "Snowbind Reagents", "Winterglass Lodge", 1},
 {"waystation_eclipse_scout", "Sable Nightroad", "Duskwatch Provisions", "Duskwatch Refuge", 9},
 {"waystation_eclipse_elder", "Vesper Rune", "Eclipse Elder's Cache", "Duskwatch Refuge", 11},
};
inline constexpr int ResidentCount = static_cast<int>(sizeof(Residents) / sizeof(Residents[0]));
static_assert(ResidentCount == 25, "Quest journal requires 13 Crossroads and 12 regional residents");

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
