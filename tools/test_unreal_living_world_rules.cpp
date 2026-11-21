#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallLivingWorldRules.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallResidentCatalog.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallMoralityRules.h"
#include <cassert>
#include <set>
#include <iostream>
using namespace MoonfallLivingWorld;
int main() {
 // Morality scores are bounded; help and extortion are distinct choices.
 using namespace MoonfallMorality;
 assert(Apply(95, 10) == 100);
 assert(Apply(-95, -10) == -100);
 assert(Apply(0, ResidentActionDelta(true)) == 10);
 assert(Apply(0, ResidentActionDelta(false)) == -10);
 assert(CouncilDelta(0) == 10 && CouncilDelta(1) == 8 && CouncilDelta(2) == 0);
 assert(QuestCompletionDelta() == 4);
 assert(ResidentSalvageDelta(true) == -10);
 assert(ResidentSalvageDelta(false) == 5);
 assert(ResidentBondDelta(true) > 0 && ResidentBondDelta(false) < 0);
 assert(ResidentReputationDelta(true) > 0 && ResidentReputationDelta(false) < 0);
 assert(IsValidAction(1) && IsValidAction(-1) && !IsValidAction(0));
 assert(Alignment(-60)[0] == 'D' && Alignment(60)[0] == 'E');

 // All 111 catalog designs must be distinct and valid under the current
 // 3-slot / 5-rarity / 25-tier inventory encoding.
 static_assert(MoonfallResidentCatalog::Count == 111, "Catalog must contain exactly 111 entries");
 std::set<int> CatalogCodes;
 for (int i = 0; i < MoonfallResidentCatalog::Count; ++i) {
  const auto Item = MoonfallResidentCatalog::Get(i);
  assert(Item.valid && Item.slot >= 0 && Item.slot < 3);
  assert(Item.rarity >= 1 && Item.rarity <= 5);
  assert(Item.tier >= 1 && Item.tier <= 25);
  assert(Item.code == Item.slot * 10000 + Item.rarity * 1000 + Item.tier);
  assert(Item.merchantCost > 0 && Item.tinkerCost > Item.merchantCost);
  assert(CatalogCodes.insert(Item.code).second);
  assert(MoonfallResidentCatalog::NamePrefix(i)[0] != '\0');
  assert(MoonfallResidentCatalog::NameSuffix(i)[0] != '\0');
  assert(MoonfallResidentCatalog::CraftLevelRequired(i) >= 1);
  assert(MoonfallResidentCatalog::CraftLevelRequired(i) <= 5);
  assert(MoonfallResidentCatalog::Next(i, 1) == (i + 1) % 111);
  assert(MoonfallResidentCatalog::Next(i, -1) == (i + 110) % 111);
 }
 assert(CatalogCodes.size() == 111);
 assert(!MoonfallResidentCatalog::Get(-1).valid);
 assert(!MoonfallResidentCatalog::Get(111).valid);
 assert(MoonfallResidentCatalog::CraftLevelRequired(-1) == 0);
 assert(MoonfallResidentCatalog::CraftLevelRequired(111) == 0);
 assert(MoonfallResidentCatalog::CraftLevelRequired(0) == 1);
 assert(MoonfallResidentCatalog::CraftLevelRequired(72) == 5);
 assert(MoonfallResidentCatalog::CraftLevelRequired(75) == 1);

 // NPC memories and choices are bounded and witness-dependent.
 assert(RelationshipAfterChoice(95, 1000, true) == 100);
 assert(RelationshipAfterChoice(-15, 30, false) == -15);
 assert(RelationshipRank(-50) == BondRank::Hostile);
 assert(WillOfferPersonalQuest(30, false, false));
 assert(!WillOfferPersonalQuest(30, true, false));
 assert(!WillOfferPersonalQuest(30, false, true));
 assert(!WillOfferPersonalQuest(-40, false, false));
 // Weapons cannot evolve on invalid progress, rarity, or zero upgrades.
 assert(WeaponEvolution(-1, 9, 5) == WeaponStage::Dormant);
 assert(WeaponEvolution(999, 9, 6) == WeaponStage::Dormant);
 assert(WeaponEvolution(9, 1, 5) == WeaponStage::Dormant);
 assert(WeaponEvolution(10, 1, 0) == WeaponStage::Awakened);
 assert(WeaponEvolution(35, 4, 4) == WeaponStage::Ascendant);
 assert(WeaponEvolution(100, 8, 5) == WeaponStage::Mythic);
 // Boss-resonance stages are bounded by the persisted 30-boss world ledger.
 assert(BossResonance(-1, 25, 5) == WeaponStage::Dormant);
 assert(BossResonance(31, 25, 5) == WeaponStage::Dormant);
 assert(BossResonance(30, 0, 5) == WeaponStage::Dormant);
 assert(BossResonance(30, 25, 6) == WeaponStage::Dormant);
 assert(BossResonance(0, 25, 5) == WeaponStage::Dormant);
 assert(BossResonance(1, 2, 0) == WeaponStage::Awakened);
 assert(BossResonance(5, 5, 4) == WeaponStage::Ascendant);
 assert(BossResonance(15, 9, 5) == WeaponStage::Mythic);
 assert(BossResonance(15, 8, 5) == WeaponStage::Ascendant);
 // World consequences must not ignore raids.
 assert(SettlementStatus({0, 0, 0, false}) == SettlementTier::Endangered);
 assert(SettlementStatus({3, 3, 0, true}) == SettlementTier::Recovering);
 assert(SettlementStatus({6, 12, 0, true}) == SettlementTier::Flourishing);
 assert(SettlementStatus({6, 12, 12, true}) == SettlementTier::Endangered);
 // The shrine is a prerequisite; Trophy/Council/Portal require achievements.
 assert(!SanctuaryRoomUnlocked(SanctuaryRoom::Atlas, false, 20, 20));
 assert(SanctuaryRoomUnlocked(SanctuaryRoom::Armory, true, 0, 0));
 assert(!SanctuaryRoomUnlocked(SanctuaryRoom::TrophyHall, true, 0, 0));
 assert(SanctuaryRoomUnlocked(SanctuaryRoom::TrophyHall, true, 1, 0));
 assert(!SanctuaryRoomUnlocked(SanctuaryRoom::Council, true, 2, 2));
 assert(SanctuaryRoomUnlocked(SanctuaryRoom::Council, true, 2, 3));
 assert(!SanctuaryRoomUnlocked(SanctuaryRoom::Portal, true, 4, 3));
 assert(SanctuaryRoomUnlocked(SanctuaryRoom::Portal, true, 5, 3));
 // Pairing is order-independent. Menus and mana limits block weaving.
 for (int a = 0; a < 3; ++a)
  for (int b = 0; b < 3; ++b) assert(Weave(a,b) == Weave(b,a));
 assert(Weave(-1, 2) == SpellPair::Invalid);
 assert(Weave(3, 2) == SpellPair::Invalid);
 assert(CanWeave(0, 1, 20, false));
 assert(!CanWeave(0, 1, 19, false));
 assert(!CanWeave(0, 1, 50, true));
 // Council impacts remain constrained even at faction bounds.
 assert(ApplyCouncilDelta(100, Decide(CouncilDecision::ProtectResidents).factionDelta) == 100);
 assert(ApplyCouncilDelta(-100, Decide(CouncilDecision::FortifyBorders).factionDelta) == -100);
 std::cout << "Living world six-system rules: PASS\n";
}
