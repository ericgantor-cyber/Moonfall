// Portable C++17 tests for pure living-world RPG policy.
// No Unreal headers, actors, save files, UI, or player state are involved.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallLivingWorldRules.h"
#include <cassert>
#include <iostream>

namespace W = MoonfallLivingWorld;
static_assert(W::ClampRelationship(-101) == -100);
static_assert(W::ClampRelationship(101) == 100);
static_assert(W::RelationshipRank(75) == W::BondRank::Devoted);
static_assert(W::Weave(0, 1) == W::Weave(1, 0));
static_assert(W::Weave(1, 2) == W::SpellPair::Stormfrost);
int main() {
    // Relationships are bounded, witness-dependent and respect quest history.
    assert(W::RelationshipRank(-50) == W::BondRank::Hostile);
    assert(W::RelationshipRank(-49) == W::BondRank::Wary);
    assert(W::RelationshipRank(-10) == W::BondRank::Neutral);
    assert(W::RelationshipRank(29) == W::BondRank::Neutral);
    assert(W::RelationshipRank(30) == W::BondRank::Trusted);
    assert(W::RelationshipRank(74) == W::BondRank::Trusted);
    assert(W::RelationshipRank(75) == W::BondRank::Devoted);
    assert(W::RelationshipAfterChoice(95, 100, true) == 100);
    assert(W::RelationshipAfterChoice(-95, -100, true) == -100);
    assert(W::RelationshipAfterChoice(50, 10, false) == 50);
    assert(W::RelationshipAfterChoice(0, 50, true) == 25);
    assert(W::WillOfferPersonalQuest(30, false, false));
    assert(!W::WillOfferPersonalQuest(29, false, false));
    assert(!W::WillOfferPersonalQuest(100, true, false));
    assert(!W::WillOfferPersonalQuest(100, false, true));

    // Invalid progress never grants weapon or boss-resonance tiers.
    assert(W::WeaponEvolution(-1, 99, 5) == W::WeaponStage::Dormant);
    assert(W::WeaponEvolution(100, 99, 6) == W::WeaponStage::Dormant);
    assert(W::WeaponEvolution(9, 1, 5) == W::WeaponStage::Dormant);
    assert(W::WeaponEvolution(10, 1, 1) == W::WeaponStage::Awakened);
    assert(W::WeaponEvolution(35, 4, 4) == W::WeaponStage::Ascendant);
    assert(W::WeaponEvolution(100, 8, 5) == W::WeaponStage::Mythic);
    assert(W::BossResonance(31, 25, 5) == W::WeaponStage::Dormant);
    assert(W::BossResonance(1, 2, 0) == W::WeaponStage::Awakened);
    assert(W::BossResonance(5, 5, 4) == W::WeaponStage::Ascendant);
    assert(W::BossResonance(15, 9, 5) == W::WeaponStage::Mythic);

    // Villages recover through quests and can be damaged by raids.
    assert(W::SettlementStatus({0, 0, 0, false}) == W::SettlementTier::Endangered);
    assert(W::SettlementStatus({3, 3, 0, true}) == W::SettlementTier::Recovering);
    assert(W::SettlementStatus({6, 12, 0, true}) == W::SettlementTier::Flourishing);
    assert(W::SettlementStatus({6, 12, 12, true}) == W::SettlementTier::Endangered);

    // The sanctuary opens progressively, never before shrine discovery.
    using Room = W::SanctuaryRoom;
    for (auto room: {Room::Atlas, Room::Armory, Room::Wardrobe,
                     Room::TrophyHall, Room::Council, Room::Portal}) {
        assert(!W::SanctuaryRoomUnlocked(room, false, 100, 100));
    }
    assert(W::SanctuaryRoomUnlocked(Room::Atlas, true, 0, 0));
    assert(W::SanctuaryRoomUnlocked(Room::Armory, true, 0, 0));
    assert(W::SanctuaryRoomUnlocked(Room::Wardrobe, true, 0, 0));
    assert(!W::SanctuaryRoomUnlocked(Room::TrophyHall, true, 0, 0));
    assert(W::SanctuaryRoomUnlocked(Room::TrophyHall, true, 1, 0));
    assert(!W::SanctuaryRoomUnlocked(Room::Council, true, 2, 2));
    assert(W::SanctuaryRoomUnlocked(Room::Council, true, 2, 3));
    assert(!W::SanctuaryRoomUnlocked(Room::Portal, true, 4, 3));
    assert(W::SanctuaryRoomUnlocked(Room::Portal, true, 5, 3));

    // Three schools give symmetric ordered pairing, guard mana and menus.
    for (int left = 0; left < 3; ++left)
        for (int right = 0; right < 3; ++right) {
            assert(W::Weave(left,right) == W::Weave(right,left));
            assert(W::CanWeave(left,right,20,false));
            assert(!W::CanWeave(left,right,19,false));
            assert(!W::CanWeave(left,right,100,true));
        }
    assert(W::Weave(-1,0) == W::SpellPair::Invalid);
    assert(W::Weave(1,3) == W::SpellPair::Invalid);
    assert(!W::CanWeave(1,3,100,false));
    assert(W::Weave(0,1) == W::SpellPair::Stormfire);
    assert(W::Weave(0,2) == W::SpellPair::Frostfire);
    assert(W::Weave(1,2) == W::SpellPair::Stormfrost);

    // Council choices are immutable values, and delta application clamps.
    using Decision = W::CouncilDecision;
    assert(W::Decide(Decision::ProtectResidents).factionDelta == 10);
    assert(W::Decide(Decision::FundReconstruction).prosperityDelta == 4);
    assert(W::Decide(Decision::FortifyBorders).threatDelta == -4);
    assert(W::ApplyCouncilDelta(99,10) == 100);
    assert(W::ApplyCouncilDelta(-99,-10) == -100);

    std::cout << "MOONFALL PORTABLE LIVING WORLD RULES PASS" << std::endl;
    return 0;
}
