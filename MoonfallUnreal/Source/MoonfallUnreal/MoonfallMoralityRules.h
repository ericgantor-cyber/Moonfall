#pragma once
// Original Moonfall moral-alignment rules. Pure C++17, no Unreal dependency.
namespace MoonfallMorality {
constexpr int Minimum = -100;
constexpr int Maximum = 100;
constexpr int Clamp(int score) {
 return score < Minimum ? Minimum : score > Maximum ? Maximum : score;
}
constexpr int Apply(int previous, int delta) {
 const int current = Clamp(previous);
 const int result = current + delta;
 return Clamp(result);
}
constexpr int ResidentActionDelta(bool aid) { return aid ? 10 : -10; }
constexpr int CouncilDelta(int decision) {
 return decision == 0 ? 10 : decision == 1 ? 8 : 0;
}
constexpr int QuestCompletionDelta() { return 4; }
constexpr int ResidentSalvageDelta(bool aid) { return aid ? -10 : 5; }
constexpr int ResidentReputationDelta(bool aid) { return aid ? 2 : -4; }
constexpr int ResidentBondDelta(bool aid) { return aid ? 6 : -6; }
constexpr const char* Alignment(int score) {
 return Clamp(score) <= -60 ? "Dreaded" :
        Clamp(score) <= -20 ? "Cruel" :
        Clamp(score) >= 60 ? "Exalted" :
        Clamp(score) >= 20 ? "Kind" : "Unaligned";
}
// Visible world tiers are earned at sustained alignment, not on each small
// action. One neutral tier and two mirrored reputation milestones.
constexpr int WorldTier(int score) {
 const int s = Clamp(score);
 return s >= 60 ? 2 : s >= 25 ? 1 : s <= -60 ? -2 : s <= -25 ? -1 : 0;
}
// Shops in Crossroads acknowledge reputation. Prices never become zero and
// never discount extortion; tinker and merchant share the same quote rule.
constexpr int AdjustShopPrice(int basePrice, int morality) {
 if (basePrice <= 0 || basePrice > 1000000) return 0;
 const int tier = WorldTier(morality);
 return tier == 2 ? (basePrice * 90 + 99) / 100
      : tier == 1 ? (basePrice * 95 + 99) / 100
      : tier == -2 ? (basePrice * 120 + 99) / 100
      : tier == -1 ? (basePrice * 110 + 99) / 100
      : basePrice;
}
constexpr const char* WorldState(int score) {
 const int tier = WorldTier(score);
 return tier == 2 ? "Beacon of Hope" : tier == 1 ? "Hopeful"
      : tier == -2 ? "Under Shadow" : tier == -1 ? "Uneasy" : "Unchanged";
}
constexpr bool IsValidAction(int action) { return action == -1 || action == 1; }
} // namespace MoonfallMorality
