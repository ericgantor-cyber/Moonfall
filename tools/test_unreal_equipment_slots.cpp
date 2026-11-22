#include "MoonfallEquipmentSlots.h"
// Tests must fail even under optimized/-DNDEBUG compilation.
#ifdef NDEBUG
#undef NDEBUG
#endif
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
 // V13 preflight must preserve all nine slots while refusing wrong-slot,
 // malformed, unowned and oversized save contents before any state is written.
 static_assert(LegacySaveVersion == 12 && ExpandedSaveVersion == 13,
               "Migration version contract changed");
 const LegacyCodes legacy{{1001, 11002, 22003}};
 const FutureCodes future{{31004, 42005, 53006, 64007, 75008, 81009}};
 const int inventory[] = {1001, 11002, 22003, 31004, 42005,
                          53006, 64007, 75008, 81009};
 const std::size_t inventorySize = sizeof(inventory) / sizeof(inventory[0]);
 ExpandedCodes v13{};
 assert(TryPrepareV13(legacy, future, inventory, inventorySize, v13));
 assert(ToLegacy(v13) == legacy);
 assert(v13[Head] == 31004 && v13[Ring] == 81009);
 assert(ValidateV13(v13, legacy, inventory, inventorySize));
 assert(IsCanonicalForSlot(31004, Head));
 assert(!IsCanonicalForSlot(31004, Shoulders));
 assert(!IsCanonicalForSlot(36004, Head)); // Unsupported rarity.
 assert(!IsCanonicalForSlot(100000 + 4, Weapon)); // Invalid common affix.
 assert(!IsCanonicalForSlot(31026, Head)); // Tier 26.
 assert(!IsCanonicalForSlot(-1, Head));
 assert(!IsCanonicalForSlot(31004, -1));
 assert(!IsCanonicalForSlot(31004, SlotCount));
 LegacyCodes wrongLegacy = legacy;
 wrongLegacy[Armor] = 11003;
 assert(!ValidateV13(v13, wrongLegacy, inventory, inventorySize));
 assert(!ValidateV13(v13, legacy, nullptr, inventorySize));
 assert(!ValidateV13(v13, legacy, inventory, MaxOwnedItems + 1));
 // A rejected preparation must not overwrite the prior equipment snapshot.
 const ExpandedCodes retained = v13;
 FutureCodes wrongSlot = future;
 wrongSlot[0] = future[1];
 assert(!TryPrepareV13(legacy, wrongSlot, inventory, inventorySize, v13));
 assert(v13 == retained);
 FutureCodes notOwned = future;
 notOwned[0] = 31024;
 assert(!TryPrepareV13(legacy, notOwned, inventory, inventorySize, v13));
 assert(v13 == retained);
 LegacyCodes staleLegacy = legacy;
 staleLegacy[Weapon] = 1002;
 assert(!TryPrepareV13(staleLegacy, future, inventory, inventorySize, v13));
 assert(v13 == retained);
 assert(!TryPrepareV13(legacy, future, nullptr, inventorySize, v13));
 assert(v13 == retained);
 // A downgrade is safe only if no future-slot equipment will be lost.
 LegacyCodes downgraded{{-1, -2, -3}};
 const LegacyCodes previousDowngrade = downgraded;
 assert(!TryDowngradeToV12(v13, downgraded));
 assert(downgraded == previousDowngrade);
 ExpandedCodes v12Ready = FromLegacy(legacy);
 assert(TryDowngradeToV12(v12Ready, downgraded));
 assert(downgraded == legacy);
 const LegacyCodes emptyLegacy{{0, 0, 0}};
 const FutureCodes emptyFuture{{0, 0, 0, 0, 0, 0}};
 ExpandedCodes emptyOutput{};
 assert(TryPrepareV13(emptyLegacy, emptyFuture, nullptr, 0, emptyOutput));
 assert(emptyOutput == FromLegacy(emptyLegacy));

 // Advisor compares all nine slots without ownership scans inside the
 // scoring function. Exact same item, invalid item and lower tier are safe.
 assert(AdvisorUpgradeDelta(81009, v13) == 0);
 assert(AdvisorUpgradeDelta(81010, v13) == 1000);
 assert(AdvisorUpgradeDelta(81008, v13) == -1000);
 assert(AdvisorUpgradeDelta(0, v13) == 0);
 assert(AdvisorUpgradeDelta(-1, v13) == 0);
 assert(AdvisorUpgradeDelta(91010, v13) == 0); // Unknown slot.
 assert(AdvisorUpgradeDelta(86010, v13) == 0); // Bad rarity.
 assert(AdvisorItemScore(81009, Ring, v13) == 9100);
 assert(AdvisorItemScore(81009, Head, v13) == 0);

 // Other gear slots, including the six new ones, contribute matching sets.
 // With three or more other matches the threshold stays at its cap.
 ExpandedCodes setGear{};
 setGear[Weapon] = 202010; // Ward weapon, tier 10 / rarity 2.
 setGear[Head] = 132010; // Force head.
 assert(AdvisorUpgradeDelta(102010, setGear) == 40);
 setGear[Shoulders] = 142010;
 assert(AdvisorUpgradeDelta(102010, setGear) == 80);
 setGear[Hands] = 152010;
 assert(AdvisorUpgradeDelta(102010, setGear) == 80);
 // Equip Best must prefer matching sets over an equally ranked off-set item.
 // Both ring candidates have the same tier, rarity and trait presence.
 const int forceRing = 182010;
 const int wardRing = 282010;
 assert(AdvisorItemScore(forceRing, Ring, setGear)
      > AdvisorItemScore(wardRing, Ring, setGear));
 setGear[Ring] = forceRing;
 assert(AdvisorUpgradeDelta(wardRing, setGear) < 0);
 setGear[Ring] = 0;
 // Invalid, wrongly slotted gear must not inflate the matching-piece count.
 setGear[Head] = 142010;
 setGear[Shoulders] = 0;
 setGear[Hands] = 0;
 assert(AdvisorUpgradeDelta(102010, setGear) == 0);

 // Portable contract: score computations never change equipped codes.
 const ExpandedCodes beforeAdvisor = setGear;
 (void)AdvisorUpgradeDelta(102010, setGear);
 assert(setGear == beforeAdvisor);
}
