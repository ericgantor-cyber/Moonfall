#pragma once
// Pure C++17: mapping existing Moonfall equipment codes to decorative atlas
// cells. This header deliberately has no Unreal dependencies so CI can test
// every combination without claiming an Unreal Engine compilation.
namespace MoonfallLootArt {
enum class EAtlas : unsigned char { None, Base, Trait, Relic };
struct FCell {
 EAtlas Atlas;
 int Column;
 int Row;
 int Columns;
 int Rows;
};
constexpr bool ValidEquipment(int Slot, int Rarity, int Trait, int Tier) {
 return Slot >= 0 && Slot <= 2 && Rarity >= 0 && Rarity <= 5
     && Trait >= 0 && Trait <= 3 && Tier >= 1 && Tier <= 25
     && (Rarity != 0 || Trait == 0);
}
constexpr FCell Select(int Slot, int Rarity, int Trait, int Tier,
 bool bBaseLoaded, bool bTraitLoaded, bool bRelicLoaded) {
 if (!ValidEquipment(Slot, Rarity, Trait, Tier))
  return {EAtlas::None, 0, 0, 0, 0};
 // An epic-or-higher tier-eight item earns additional visual variety.
 // This does not change its stats, ownership, rarity, equipment slot or roll.
 if (bRelicLoaded && Rarity >= 3 && Tier >= 8)
  return {EAtlas::Relic, (Tier + Rarity + Trait) % 3, Slot, 3, 3};
 if (bTraitLoaded && Trait > 0) {
  if (Slot == 0) return {EAtlas::Trait, Trait - 1, 0, 3, 3};
  if (Slot == 1) return {EAtlas::Trait,
    Trait == 1 ? 2 : (Trait == 2 ? 0 : 1), 1, 3, 3};
  // The current atlas shares the emerald ring between two traits.
  return {EAtlas::Trait, Trait == 3 ? 2 : 0, 2, 3, 3};
 }
 if (bBaseLoaded) return {EAtlas::Base, Slot, 0, 3, 2};
 return {EAtlas::None, 0, 0, 0, 0}; // Caller paints fallback silhouettes
}
} // namespace MoonfallLootArt
