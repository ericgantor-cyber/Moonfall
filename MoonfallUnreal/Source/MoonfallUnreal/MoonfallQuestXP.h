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
} // namespace MoonfallQuestXP
