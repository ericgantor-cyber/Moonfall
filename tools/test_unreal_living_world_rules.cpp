#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallLivingWorldRules.h"
#include <cassert>
#include <iostream>
using namespace MoonfallLivingWorld;
int main() {
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
