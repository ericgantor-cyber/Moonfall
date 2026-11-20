#pragma once
// C++17-only reward schedule for completed NPC salvage contracts.
// Keep independent of Unreal types and gameplay RNG so every case can be
// tested without native engine compilation.
namespace MoonfallQuestXP {
constexpr int Reward(int SalvageCost, int PlayerLevel) {
 // Only validated contracts and living, levelable characters may earn XP.
 if (SalvageCost < 1 || SalvageCost > 30
     || PlayerLevel < 1 || PlayerLevel >= 100) return 0;
 // Cost measures contract difficulty. Player level offsets increasing XP
 // thresholds so NPC quests stay worth completing while progressing.
 const int RewardXP = 20 + SalvageCost * 2 + PlayerLevel * 2;
 return RewardXP > 300 ? 300 : RewardXP;
}
// Player-facing contract rank is derived from the existing validated
// salvage price, never an additional random roll or persisted quest field.
constexpr const char* RankName(int SalvageCost) {
 if (SalvageCost < 1 || SalvageCost > 30) return "INVALID";
 if (SalvageCost <= 10) return "APPRENTICE";
 if (SalvageCost <= 15) return "ADVENTURER";
 if (SalvageCost <= 20) return "VETERAN";
 return "MASTER";
}
struct FProjection {
 int RewardXP;
 int FinalLevel;
 int LevelsGained;
 int RemainingXP;
 int NextLevelXP;
};
// Mirror the current AwardExperience rollover, including the level-100 cap.
// Read-only: HUD can forecast a level-up before a player confirms salvage.
constexpr FProjection Preview(int SalvageCost, int PlayerLevel,
                              int CurrentXP, int CurrentNextLevelXP) {
 const int Gain = Reward(SalvageCost, PlayerLevel);
 if (PlayerLevel < 1 || PlayerLevel >= 100 || CurrentXP < 0
     || CurrentNextLevelXP <= 0 || Gain == 0)
  return {0, PlayerLevel, 0, CurrentXP, CurrentNextLevelXP};
 int LevelAfter = PlayerLevel;
 int XPAfter = CurrentXP + Gain > 1000000 ? 1000000 : CurrentXP + Gain;
 int Threshold = CurrentNextLevelXP;
 while (XPAfter >= Threshold && LevelAfter < 100) {
  XPAfter -= Threshold;
  ++LevelAfter;
  Threshold = 100 + (LevelAfter - 1) * 55;
 }
 if (LevelAfter >= 100) XPAfter = 0;
 return {Gain, LevelAfter, LevelAfter - PlayerLevel,
   XPAfter, LevelAfter >= 100 ? 0 : Threshold};
}
} // namespace MoonfallQuestXP
