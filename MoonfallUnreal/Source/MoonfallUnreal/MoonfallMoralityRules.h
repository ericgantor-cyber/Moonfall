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
constexpr bool IsValidAction(int action) { return action == -1 || action == 1; }
} // namespace MoonfallMorality
