#pragma once
// Twelve original regional quest objectives from the already-persisted
// six-by-five dungeon discovery and boss-victory counts. No save changes,
// duplicate XP, hidden world actors, or per-frame world searches.
namespace MoonfallWorldQuest {
inline constexpr int ZoneCount = 6;
inline constexpr int GoalsPerZone = 2;
inline constexpr int GoalCount = ZoneCount * GoalsPerZone;
inline constexpr int MaxVisible = 7;
inline constexpr const char* RegionNames[ZoneCount] = {
 "Ashen Wilds", "Moonveil Forest", "Hollow Depths",
 "Iron Foundry", "Frostbound Reach", "Eclipse Wastes"
};
inline constexpr const char* GoalTitles[GoalCount] = {
 "Survey the Ashen Road", "Break the Ember Wardens",
 "Chart the Silver Groves", "Silence the Briar Lords",
 "Map the Hollow Tunnels", "Defeat the Deep Sentinels",
 "Trace the Iron Route", "Crush the Foundry Cores",
 "Find the Frostbound Paths", "Face the Icebound Keepers",
 "Chart the Eclipse Wastes", "Scatter the Night Wardens"
};
struct FSelection {
 int Indices[MaxVisible] = {-1, -1, -1, -1, -1, -1, -1};
 int Count = 0;
 int Unfinished = 0;
 int Completed = 0;
 bool Valid = false;
};
// A touched zone's incomplete quests appear before an unvisited zone's.
// Explore/defeat stages are independent, so defeating an early boss doesn't
// erase discovery progress. Completed quests never take visible slots.
constexpr FSelection Select(const int* Discovered, const int* Defeated, int RequestedSlots) {
 FSelection Out{};
 if (!Discovered || !Defeated || RequestedSlots < 0 || RequestedSlots > MaxVisible)
  return Out;
 for (int Z = 0; Z < ZoneCount; ++Z) {
  if (Discovered[Z] < 0 || Discovered[Z] > 5 || Defeated[Z] < 0 ||
      Defeated[Z] > Discovered[Z]) return Out;
 }
 Out.Valid = true;
 for (int Z = 0; Z < ZoneCount; ++Z) {
  if (Discovered[Z] == 5) ++Out.Completed;
  else ++Out.Unfinished;
  if (Defeated[Z] == 5) ++Out.Completed;
  else ++Out.Unfinished;
 }
 for (int Priority = 0; Priority < 2 && Out.Count < RequestedSlots; ++Priority) {
  for (int Z = 0; Z < ZoneCount && Out.Count < RequestedSlots; ++Z) {
   const bool Touched = Discovered[Z] > 0;
   if (Touched != (Priority == 0)) continue;
   for (int Stage = 0; Stage < GoalsPerZone && Out.Count < RequestedSlots; ++Stage) {
    const int Goal = 2 * Z + Stage;
    const int Progress = Stage == 0 ? Discovered[Z] : Defeated[Z];
    if (Progress == 5 || (Stage == 1 && !Touched)) continue;
    Out.Indices[Out.Count++] = Goal;
   }
  }
 }
 return Out;
}
constexpr int Progress(int Goal, const int* Discovered, const int* Defeated) {
 return !Discovered || !Defeated || Goal < 0 || Goal >= GoalCount ? 0
    : Goal % 2 == 0 ? Discovered[Goal/2] : Defeated[Goal/2];
}
constexpr const char* Type(int Goal) {
 return Goal < 0 || Goal >= GoalCount ? "INVALID"
  : Goal % 2 == 0 ? "ENTRANCES" : "BOSSES";
}
static_assert(GoalCount == 12, "Six regions need two unique quests each");
} // namespace MoonfallWorldQuest
