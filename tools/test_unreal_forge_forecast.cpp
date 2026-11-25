#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallForgeForecast.h"
#include <cassert>
#include <iostream>
using namespace MoonfallForgeForecast;

static_assert(!ValidCode(0), "Empty equipment must not be craftable");
static_assert(!ValidCode(100001), "Common gear cannot have a trait");
static_assert(ValidCode(5001), "Untraited legendary tier one is valid");
static_assert(Cost(24, 5) == 67, "Tier/rarity price matches the real game");
static_assert(Cost(25, 5) == 0, "Tier 25 is maximum");
static_assert(Preview(25, 500, 5).Upgrades == 0, "Max tier is immutable");
static_assert(Preview(1, 100, 5).Upgrades == 5, "Cheap gear can gain five tiers");
static_assert(Preview(1, 0, 5).Upgrades == 0, "No free forged levels");
static_assert(Preview(1, 6, 5).FinalTier == 2, "Single-level price is correct");
static_assert(Preview(1, 6, 5).NextTierCost == 8, "Next price is not skipped");
static_assert(!Preview(899999, 100, 5).Valid, "Malformed gear fails closed");

int main() {
 long checked = 0;
 // Cross-check thousands of canonical equipment codes, budgets and limits
 // against an independent single-tier transaction reference.
 for (int Trait = 0; Trait <= 3; ++Trait)
 for (int Slot = 0; Slot <= 2; ++Slot)
 for (int Rarity = 0; Rarity <= 5; ++Rarity)
 for (int Tier = 1; Tier <= 25; ++Tier) {
  const int Code = Trait*100000 + Slot*10000 + Rarity*1000 + Tier;
  if (Rarity == 0 && Trait != 0) {
   assert(!ValidCode(Code));
   continue;
  }
  assert(ValidCode(Code));
  for (int Balance : {-5, 0, 1, 6, 20, 55, 100, 200, 1000, 100000000})
  for (int Limit = -2; Limit <= 9; ++Limit) {
   const FPlan Plan = Preview(Code, Balance, Limit);
   assert(Plan.Valid);
   const int ClampedLimit = Limit < 0 ? 0 : Limit > 5 ? 5 : Limit;
   int ReferenceBalance = Balance < 0 ? 0 : Balance;
   int ReferenceTier = Tier, Spent = 0, Levels = 0;
   for (int Step = 0; Step < ClampedLimit && ReferenceTier < 25; ++Step) {
    const int Price = 4 + ReferenceTier*2 + Rarity*3;
    if (ReferenceBalance < Price) break;
    ReferenceBalance -= Price;
    Spent += Price;
    ++ReferenceTier;
    ++Levels;
   }
   assert(Plan.Upgrades == Levels && Plan.TotalCost == Spent);
   assert(Plan.FinalTier == ReferenceTier);
   assert(Plan.NextTierCost == (ReferenceTier < 25
     ? 4 + ReferenceTier*2 + Rarity*3 : 0));
   assert(Plan.TotalCost <= (Balance < 0 ? 0 : Balance));
   assert(Plan.Upgrades <= 5);
   ++checked;
  }
 }
 // Explicitly reject invalid slot/rarity/trait/negative-code combinations.
 for (int Code : {-2, 0, 26, 6001, 30001, 90001, 100001, 900999})
  assert(!Preview(Code, 100, 5).Valid);
 std::cout << "Moonfall forge forecast: " << checked
           << " source-independent budget/cap combinations PASS\n";
 return 0;
}
