#pragma once
#include <array>
namespace MoonfallGearAdvisor {
using FEquipped = std::array<int, 3>;
// Identical encoding and scoring weights to the original advisor. Worn
// ownership is checked once at the Unreal boundary, not per candidate.
constexpr bool Valid(int Code, int Slot) {
 if (Code <= 0 || Slot < 0 || Slot > 2) return false;
 const int Affix = Code / 100000, EncodedSlot = (Code / 10000) % 10;
 const int Rarity = (Code / 1000) % 10, Tier = Code % 1000;
 return EncodedSlot == Slot && Affix >= 0 && Affix <= 3 && Rarity >= 0
     && Rarity <= 5 && Tier >= 1 && Tier <= 25 && (Rarity != 0 || Affix == 0)
     && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
}
constexpr int Score(int Code, int Slot, const FEquipped& Equipped) {
 if (!Valid(Code, Slot)) return 0;
 const int Affix = Code / 100000;
 int Matching = 0;
 for (int Other = 0; Other < 3; ++Other)
  if (Affix > 0 && Other != Slot && Valid(Equipped[Other], Other)
      && Equipped[Other] / 100000 == Affix) ++Matching;
 return (Code % 1000) * 1000 + ((Code / 1000) % 10) * 100
     + (Affix > 0 ? 10 : 0) + Matching * 40;
}
constexpr int Improvement(int Candidate, const FEquipped& Equipped) {
 if (Candidate <= 0) return 0;
 const int Slot = (Candidate / 10000) % 10;
 if (!Valid(Candidate, Slot) || Candidate == Equipped[Slot]) return 0;
 return Score(Candidate, Slot, Equipped) - Score(Equipped[Slot], Slot, Equipped);
}
}
