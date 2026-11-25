#pragma once
// Pure C++17 rules shared by Unreal gameplay and deterministic offline tests.
// This is a read-only forecast: it never mutates inventory, balance or RNG.
namespace MoonfallForgeForecast {
struct FPlan {
 bool Valid;
 int Upgrades;
 int TotalCost;
 int FinalTier;
 int NextTierCost; // 0 means max tier, not a free upgrade.
};
constexpr bool ValidCode(int Code) {
 if (Code <= 0) return false;
 const int Trait = Code / 100000, Slot = (Code / 10000) % 10;
 const int Rarity = (Code / 1000) % 10, Tier = Code % 1000;
 return Trait >= 0 && Trait <= 3 && Slot >= 0 && Slot <= 2
   && Rarity >= 0 && Rarity <= 5 && Tier >= 1 && Tier <= 25
   && (Rarity != 0 || Trait == 0)
   && Code == Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
}
constexpr int Cost(int Tier, int Rarity) {
 return Tier >= 1 && Tier < 25 && Rarity >= 0 && Rarity <= 5
  ? 4 + Tier * 2 + Rarity * 3 : 0;
}
constexpr FPlan Preview(int Code, int Salvage, int MaxLevels = 5) {
 if (!ValidCode(Code)) return {false, 0, 0, 0, 0};
 const int Tier = Code % 1000;
 const int Rarity = (Code / 1000) % 10;
 const int Limit = MaxLevels < 0 ? 0 : (MaxLevels > 5 ? 5 : MaxLevels);
 int Balance = Salvage < 0 ? 0 : Salvage;
 int FinalTier = Tier, Count = 0, Spent = 0;
 for (int Step = 0; Step < Limit && FinalTier < 25; ++Step) {
  const int Price = Cost(FinalTier, Rarity);
  if (Price == 0 || Balance < Price) break;
  Balance -= Price;
  Spent += Price;
  ++FinalTier;
  ++Count;
 }
 return {true, Count, Spent, FinalTier, Cost(FinalTier, Rarity)};
}
} // namespace MoonfallForgeForecast
