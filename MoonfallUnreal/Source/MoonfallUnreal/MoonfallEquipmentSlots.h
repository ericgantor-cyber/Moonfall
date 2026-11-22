#pragma once
// Nine-slot equipment migration contract. This file deliberately changes no
// runtime equipment, loot encoding or serialized save schema.
#include <array>
#include <cstddef>
namespace MoonfallEquipmentSlots {
constexpr int LegacyCount = 3;
constexpr int SlotCount = 9;
constexpr int FutureCount = SlotCount - LegacyCount;
// Forward migration contract only; live USaveGame still writes version 12.
constexpr int LegacySaveVersion = 12;
constexpr int ExpandedSaveVersion = 13;
constexpr std::size_t MaxOwnedItems = 10000;
enum Slot : int {
 Weapon = 0, Armor = 1, Accessory = 2,
 Head = 3, Shoulders = 4, Hands = 5, Legs = 6, Feet = 7, Ring = 8
};
using LegacyCodes = std::array<int, LegacyCount>;
using ExpandedCodes = std::array<int, SlotCount>;
using FutureCodes = std::array<int, FutureCount>;
// Preserve all three existing equipped codes at their original indices.
// The six future slots start empty, including for older saved characters.
constexpr ExpandedCodes FromLegacy(const LegacyCodes& existing) {
 return {{existing[0], existing[1], existing[2], 0, 0, 0, 0, 0, 0}};
}
constexpr LegacyCodes ToLegacy(const ExpandedCodes& expanded) {
 return {{expanded[Weapon], expanded[Armor], expanded[Accessory]}};
}
constexpr bool IsLegacySlot(int slot) { return slot >= 0 && slot < LegacyCount; }
constexpr bool IsKnownSlot(int slot) { return slot >= 0 && slot < SlotCount; }
// Stable UI metadata for all nine slots. Unknown indices never alias another slot.
constexpr bool IsFutureSlot(int slot) { return slot >= LegacyCount && slot < SlotCount; }
constexpr const char* SlotName(int slot) {
 switch (slot) {
 case Weapon: return "Weapon";
 case Armor: return "Armor";
 case Accessory: return "Accessory";
 case Head: return "Head";
 case Shoulders: return "Shoulders";
 case Hands: return "Hands";
 case Legs: return "Legs";
 case Feet: return "Feet";
 case Ring: return "Ring";
 default: return "Unknown";
 }
}
// Future Version 13 policy. V12 saving/loading is deliberately unchanged.
// Do not accept a nine-slot save unless every nonempty worn code is canonical,
// belongs to its precise slot, and is present in the saved inventory.
constexpr bool IsCanonicalForSlot(int code, int expectedSlot) {
 if (code <= 0 || !IsKnownSlot(expectedSlot)) return false;
 const int trait = code / 100000;
 const int slot = (code / 10000) % 10;
 const int rarity = (code / 1000) % 10;
 const int tier = code % 1000;
 return slot == expectedSlot && trait >= 0 && trait <= 3
   && rarity >= 0 && rarity <= 5 && tier >= 1 && tier <= 25
   && (rarity != 0 || trait == 0)
   && code == trait * 100000 + slot * 10000 + rarity * 1000 + tier;
}
inline bool IsOwned(int code, const int* inventory, std::size_t count) {
 if (!inventory) return false;
 for (std::size_t i = 0; i < count; ++i)
  if (inventory[i] == code) return true;
 return false;
}
inline bool ValidateV13(const ExpandedCodes& gear,
                        const LegacyCodes& legacy,
                        const int* inventory, std::size_t count) {
 if (count > MaxOwnedItems || (count && !inventory)) return false;
 for (int slot = 0; slot < SlotCount; ++slot) {
  const int code = gear[slot];
  if (slot < LegacyCount && code != legacy[slot]) return false;
  if (code != 0 && (!IsCanonicalForSlot(code, slot) ||
                    !IsOwned(code, inventory, count))) return false;
 }
 return true;
}
// Atomic preparation: never partially write output on a corrupt or stale item.
inline bool TryPrepareV13(const LegacyCodes& legacy, const FutureCodes& future,
                         const int* inventory, std::size_t count,
                         ExpandedCodes& output) {
 ExpandedCodes candidate = FromLegacy(legacy);
 for (int slot = LegacyCount; slot < SlotCount; ++slot)
  candidate[slot] = future[slot - LegacyCount];
 if (!ValidateV13(candidate, legacy, inventory, count)) return false;
 output = candidate;
 return true;
}
// Read-only advisor scoring against a prevalidated nine-slot snapshot.
// Callers populate equipped with GetEquipmentSlotCode, which checks ownership
// once per slot. Scanning a large bag then costs O(items * 9), not O(items^2).
// The 2pc/3pc set synergy is capped by threshold, not by the total pieces.
constexpr int AdvisorItemScore(int code, int expectedSlot,
                               const ExpandedCodes& equipped) {
 if (!IsCanonicalForSlot(code, expectedSlot)) return 0;
 const int affix = code / 100000;
 const int rarity = (code / 1000) % 10;
 const int tier = code % 1000;
 int matchingPieces = 0;
 if (affix > 0) {
  for (int otherSlot = 0; otherSlot < SlotCount; ++otherSlot) {
   if (otherSlot == expectedSlot) continue;
   const int wornCode = equipped[otherSlot];
   if (IsCanonicalForSlot(wornCode, otherSlot)
       && wornCode / 100000 == affix) ++matchingPieces;
  }
 }
 const int synergy = matchingPieces >= 2 ? 80
   : matchingPieces == 1 ? 40 : 0;
 return tier * 1000 + rarity * 100 + (affix > 0 ? 10 : 0) + synergy;
}
constexpr int AdvisorUpgradeDelta(int candidate, const ExpandedCodes& equipped) {
 const int slot = (candidate / 10000) % 10;
 if (!IsCanonicalForSlot(candidate, slot) || equipped[slot] == candidate)
  return 0;
 return AdvisorItemScore(candidate, slot, equipped)
   - AdvisorItemScore(equipped[slot], slot, equipped);
}
// A downgrade that simply drops six worn items would lose player progress.
// Leave destination untouched and refuse it until all six slots are empty.
constexpr bool TryDowngradeToV12(const ExpandedCodes& gear,
                                LegacyCodes& output) {
 for (int slot = LegacyCount; slot < SlotCount; ++slot)
  if (gear[slot] != 0) return false;
 output = ToLegacy(gear);
 return true;
}
static_assert(Weapon == 0 && Armor == 1 && Accessory == 2,
 "Do not change legacy equipped-code indices");
static_assert(SlotCount == 9, "Nine-slot contract changed");
}
