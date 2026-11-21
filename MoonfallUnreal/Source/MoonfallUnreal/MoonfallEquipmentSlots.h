#pragma once
// Nine-slot equipment migration contract. This file deliberately changes no
// runtime equipment, loot encoding or serialized save schema.
#include <array>
namespace MoonfallEquipmentSlots {
constexpr int LegacyCount = 3;
constexpr int SlotCount = 9;
enum Slot : int {
 Weapon = 0, Armor = 1, Accessory = 2,
 Head = 3, Shoulders = 4, Hands = 5, Legs = 6, Feet = 7, Ring = 8
};
using LegacyCodes = std::array<int, LegacyCount>;
using ExpandedCodes = std::array<int, SlotCount>;
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
static_assert(Weapon == 0 && Armor == 1 && Accessory == 2,
 "Do not change legacy equipped-code indices");
static_assert(SlotCount == 9, "Nine-slot contract changed");
}
