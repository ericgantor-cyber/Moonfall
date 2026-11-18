#pragma once
// Pure C++17: mapping existing Moonfall equipment codes to decorative atlas
// cells. This header deliberately has no Unreal dependencies so CI can test
// every combination without claiming an Unreal Engine compilation.
namespace MoonfallLootArt {
enum class EAtlas : unsigned char { None, Base, Trait, Relic, Masterwork, Ascendant };
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
 bool bBaseLoaded, bool bTraitLoaded, bool bRelicLoaded,
 bool bMasterworkLoaded = false, bool bAscendantLoaded = false) {
 if (!ValidEquipment(Slot, Rarity, Trait, Tier))
  return {EAtlas::None, 0, 0, 0, 0};
 // Existing Legendary tier-22+ equipment can wear an Ascendant motif.
 // This is NOT a new rarity or new item and consumes no gameplay RNG.
 if (bAscendantLoaded && Rarity == 5 && Tier >= 22)
  return {EAtlas::Ascendant, (Tier + Trait + Slot) % 3, Slot, 3, 3};
 // Emerald and Legendary tier-16+ equipment may use an optional fourth
 // original art atlas. The item is still Emerald/Legendary, not a new rarity.
 // Keep this deterministic and never change the underlying loot roll.
 if (bMasterworkLoaded && Rarity >= 4 && Tier >= 16)
  return {EAtlas::Masterwork, (Tier + Rarity + Trait) % 3, Slot, 3, 3};
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
// Defense in depth for future galleries and any damaged art metadata.
constexpr bool ValidCell(const FCell& Cell) {
 return Cell.Atlas == EAtlas::None
   ? Cell.Columns == 0 && Cell.Rows == 0
   : Cell.Columns > 0 && Cell.Rows > 0 && Cell.Column >= 0 && Cell.Row >= 0
     && Cell.Column < Cell.Columns && Cell.Row < Cell.Rows
     && Cell.Columns <= 4 && Cell.Rows <= 4;
}
} // namespace MoonfallLootArt
