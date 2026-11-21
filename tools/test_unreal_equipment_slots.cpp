#include "MoonfallEquipmentSlots.h"
#include <cassert>
int main() {
 using namespace MoonfallEquipmentSlots;
 constexpr LegacyCodes oldCodes{{101, 20102, 30203}};
 constexpr ExpandedCodes migrated = FromLegacy(oldCodes);
 static_assert(migrated[Weapon] == 101, "weapon changed");
 static_assert(migrated[Armor] == 20102, "armor changed");
 static_assert(migrated[Accessory] == 30203, "accessory changed");
 static_assert(ToLegacy(migrated) == oldCodes, "legacy round trip changed");
 for (int slot = 0; slot < SlotCount; ++slot) {
  assert(IsKnownSlot(slot));
  assert(IsLegacySlot(slot) == (slot < LegacyCount));
  if (slot >= LegacyCount) assert(migrated[slot] == 0);
 }
 assert(!IsKnownSlot(-1) && !IsKnownSlot(9));
 assert(!IsLegacySlot(-1) && !IsLegacySlot(3));
 // Future-only equipment cannot contaminate the existing serialized fields.
 ExpandedCodes expanded = migrated;
 expanded[Head] = 40001;
 expanded[Ring] = 80001;
 assert(ToLegacy(expanded) == oldCodes);
}
