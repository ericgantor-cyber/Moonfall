#include "MoonfallEquipmentSlots.h"
#include <cassert>
#include <string>
int main() {
 using namespace MoonfallEquipmentSlots;
 constexpr LegacyCodes oldCodes{{101, 20102, 30203}};
 constexpr ExpandedCodes migrated = FromLegacy(oldCodes);
 static_assert(migrated[Weapon] == 101, "weapon changed");
 static_assert(migrated[Armor] == 20102, "armor changed");
 static_assert(migrated[Accessory] == 30203, "accessory changed");
 static_assert(ToLegacy(migrated)[0] == oldCodes[0] &&
               ToLegacy(migrated)[1] == oldCodes[1] &&
               ToLegacy(migrated)[2] == oldCodes[2], "legacy round trip changed");
 for (int slot = 0; slot < SlotCount; ++slot) {
  assert(IsKnownSlot(slot));
  assert(IsLegacySlot(slot) == (slot < LegacyCount));
  if (slot >= LegacyCount) assert(migrated[slot] == 0);
 }
 assert(!IsKnownSlot(-1) && !IsKnownSlot(9));
 assert(!IsLegacySlot(-1) && !IsLegacySlot(3));
 constexpr const char* expectedNames[SlotCount] = {
  "Weapon", "Armor", "Accessory", "Head", "Shoulders",
  "Hands", "Legs", "Feet", "Ring"
 };
 for (int slot = 0; slot < SlotCount; ++slot) {
  assert(std::string(SlotName(slot)) == expectedNames[slot]);
  assert(IsFutureSlot(slot) == (slot >= LegacyCount));
 }
 assert(std::string(SlotName(-1)) == "Unknown");
 assert(std::string(SlotName(SlotCount)) == "Unknown");
 assert(!IsFutureSlot(-1) && !IsFutureSlot(SlotCount));

 // Future-only equipment cannot contaminate the existing serialized fields.
 ExpandedCodes expanded = migrated;
 expanded[Head] = 40001;
 expanded[Ring] = 80001;
 assert(ToLegacy(expanded) == oldCodes);
 // Exercise empty, negative (invalid/stale), and extreme legacy values without
 // normalizing or re-encoding them. Migration must be lossless even when a
 // separate runtime validator later rejects an item code.
 constexpr LegacyCodes cases[] = {
  {{0, 0, 0}},
  {{-1, -100, -999999}},
  {{2147483647, -2147483647 - 1, 42}},
  {{901, 0, 70007}}
 };
 for (const LegacyCodes& before : cases) {
  const ExpandedCodes after = FromLegacy(before);
  assert(ToLegacy(after) == before);
  for (int slot = LegacyCount; slot < SlotCount; ++slot) {
   assert(after[slot] == 0);
  }
  ExpandedCodes futureOnly = after;
  for (int slot = LegacyCount; slot < SlotCount; ++slot) {
   futureOnly[slot] = 100000 + slot;
  }
  assert(ToLegacy(futureOnly) == before);
 }
}
