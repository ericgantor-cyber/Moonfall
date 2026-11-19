#pragma once
// Stable UI-only resident catalog and bounded quest selection rules.
// Does not spawn NPCs, change rewards, or alter persistent save formats.
// Match names/races to existing arena and regional-waystation authoring.
namespace MoonfallQuestJournal {
struct FResident {
 const char* Id;
 const char* Name;
 const char* Location;
 int Race;
};
inline constexpr FResident Residents[] = {
 {"CrossroadsGuide", "Wandering Guide", "Shattered Sanctuary", 2},
 {"Alden", "Alden", "Crossroads", 0},
 {"Starweaver", "Starweaver", "Crossroads", 1},
 {"Leafwhisper", "Leafwhisper", "Crossroads", 2},
 {"Anvilkeeper", "Anvilkeeper", "Crossroads", 3},
 {"PipHearthfoot", "Pip Hearthfoot", "Crossroads", 4},
 {"EmberScout", "Ember Scout", "Crossroads", 5},
 {"Geargrin", "Geargrin", "Crossroads", 6},
 {"Mossfang", "Mossfang", "Crossroads", 7},
 {"Birchroot", "Birchroot", "Crossroads", 8},
 {"Duskveil", "Duskveil", "Crossroads", 9},
 {"Silverwatch", "Silverwatch", "Crossroads", 10},
 {"Ashscale", "Ashscale", "Crossroads", 11},
 {"waystation_ashen_scout", "Cinder Runner", "Emberwatch Camp", 5},
 {"waystation_ashen_artisan", "Ashforge Keeper", "Emberwatch Camp", 3},
 {"waystation_moonveil_scout", "Mira Fernstep", "Silverleaf Refuge", 2},
 {"waystation_moonveil_scholar", "Selune Whisper", "Silverleaf Refuge", 1},
 {"waystation_hollows_scout", "Nox Tunnelwatch", "Deepstone Haven", 3},
 {"waystation_hollows_tinker", "Dara Flint", "Deepstone Haven", 6},
 {"waystation_foundry_scout", "Kora Spark", "Sparkfall Relay", 6},
 {"waystation_foundry_tinker", "Mek Ironhand", "Sparkfall Relay", 3},
 {"waystation_frost_scout", "Eira Froststep", "Winterglass Lodge", 10},
 {"waystation_frost_scholar", "Orin Snowbind", "Winterglass Lodge", 1},
 {"waystation_eclipse_scout", "Sable Nightroad", "Duskwatch Refuge", 9},
 {"waystation_eclipse_elder", "Vesper Rune", "Duskwatch Refuge", 11},
};
inline constexpr int ResidentCount = static_cast<int>(sizeof(Residents) / sizeof(Residents[0]));
static_assert(ResidentCount == 25, "Quest journal requires 13 Crossroads and 12 regional residents");

// Match AMoonfallWorldNPC::GetContractSalvageCost, including the Guide exception.
constexpr int Cost(int Index) {
 return Index < 0 || Index >= ResidentCount ? 0
   : Index == 0 ? 10 : 10 + (Residents[Index].Race % 4) * 5;
}
struct FSummary {
 int Active = 0;
 int Completed = 0;
 int Ready = 0;
 int TrackedIndex = -1;
};
// State 1 is accepted, 2 completed. Pick an actionable, ready-to-turn-in
// contract first. Ties follow stable catalog order, not TMap iteration order.
constexpr FSummary Summarize(const int* States, int StateCount, int Salvage) {
 FSummary Result{};
 if (!States || StateCount != ResidentCount || Salvage < 0) return Result;
 for (int I = 0; I < ResidentCount; ++I) {
  if (States[I] == 2) { ++Result.Completed; continue; }
  if (States[I] != 1) continue;
  ++Result.Active;
  const bool bReady = Salvage >= Cost(I);
  if (bReady) ++Result.Ready;
  if (Result.TrackedIndex < 0 ||
      (bReady && Salvage < Cost(Result.TrackedIndex))) Result.TrackedIndex = I;
 }
 return Result;
}
} // namespace MoonfallQuestJournal
