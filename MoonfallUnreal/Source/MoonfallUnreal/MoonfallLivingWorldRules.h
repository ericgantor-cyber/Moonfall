#pragma once
// Moonfall original social-world design rules. Engine-independent C++17.
// Pure calculations: no save writes, actor spawns, artwork, or input side effects.
#include <algorithm>
#include <array>
#include <cstdint>

namespace MoonfallLivingWorld {
constexpr int Clamp(int x, int lo, int hi) {
 return x < lo ? lo : (x > hi ? hi : x);
}
constexpr int ClampRelationship(int value) { return Clamp(value, -100, 100); }
enum class BondRank { Hostile, Wary, Neutral, Trusted, Devoted };
constexpr BondRank RelationshipRank(int bond) {
 return bond <= -50 ? BondRank::Hostile : bond < -10 ? BondRank::Wary
  : bond < 30 ? BondRank::Neutral : bond < 75 ? BondRank::Trusted
  : BondRank::Devoted;
}
constexpr bool WillOfferPersonalQuest(int bond, bool questAlreadyAccepted,
 bool questFinished) {
 return RelationshipRank(bond) >= BondRank::Trusted
  && !questAlreadyAccepted && !questFinished;
}
constexpr int RelationshipAfterChoice(int current, int trustDelta,
 bool witnessed) {
 return witnessed ? ClampRelationship(ClampRelationship(current) +
  Clamp(trustDelta, -25, 25)) : ClampRelationship(current);
}

// Existing Moonfall equipment codes remain untouched. This is a cosmetic/effect
// stage derived from owned weapon progress, not a new save format.
enum class WeaponStage { Dormant, Awakened, Ascendant, Mythic };
constexpr WeaponStage WeaponEvolution(int verifiedVictories, int upgrades,
 int32_t rarity) {
 if (verifiedVictories < 0 || upgrades < 0 || rarity < 0 || rarity > 5)
  return WeaponStage::Dormant;
 if (rarity >= 5 && verifiedVictories >= 100 && upgrades >= 8)
  return WeaponStage::Mythic;
 if (rarity >= 4 && verifiedVictories >= 35 && upgrades >= 4)
  return WeaponStage::Ascendant;
 if (verifiedVictories >= 10 && upgrades >= 1)
  return WeaponStage::Awakened;
 return WeaponStage::Dormant;
}

// This separate resonance display uses the already persisted dungeon boss
// ledger, which is GLOBAL progress, not a per-weapon kill counter.
// WeaponEvolution above remains reserved for real per-weapon milestones.
constexpr WeaponStage BossResonance(int defeatedDungeonBosses, int gearTier, int rarity) {
 if (defeatedDungeonBosses < 0 || defeatedDungeonBosses > 30
     || gearTier < 1 || gearTier > 25 || rarity < 0 || rarity > 5)
  return WeaponStage::Dormant;
 if (defeatedDungeonBosses >= 15 && gearTier >= 9 && rarity >= 5)
  return WeaponStage::Mythic;
 if (defeatedDungeonBosses >= 5 && gearTier >= 5 && rarity >= 4)
  return WeaponStage::Ascendant;
 if (defeatedDungeonBosses >= 1 && gearTier >= 2)
  return WeaponStage::Awakened;
 return WeaponStage::Dormant;
}

// A settlement is rebuilt through verified objective completions, not mere
// proximity. Harm may lower prosperity but never fabricate completed quests.
struct Settlement {
 int restoredSites = 0;
 int completedContracts = 0;
 int hostileRaids = 0;
 bool shrineDiscovered = false;
};
enum class SettlementTier { Endangered, Recovering, Flourishing };
constexpr SettlementTier SettlementStatus(Settlement s) {
 const int sites = Clamp(s.restoredSites, 0, 6);
 const int contracts = Clamp(s.completedContracts, 0, 12);
 const int raids = Clamp(s.hostileRaids, 0, 12);
 const int score = sites * 2 + contracts - raids * 3
  + (s.shrineDiscovered ? 2 : 0);
 return score >= 15 ? SettlementTier::Flourishing
  : score >= 5 ? SettlementTier::Recovering : SettlementTier::Endangered;
}

// Physical Sanctuary rooms complement, rather than replace, direct mouse/
// controller accessible inventory, Atlas, and character menus.
enum class SanctuaryRoom { Atlas, Armory, Wardrobe, TrophyHall, Council, Portal };
constexpr bool SanctuaryRoomUnlocked(SanctuaryRoom room,
 bool startingShrineDiscovered, int victories, int completedContracts) {
 if (!startingShrineDiscovered) return false;
 switch (room) {
 case SanctuaryRoom::Atlas:
 case SanctuaryRoom::Armory:
 case SanctuaryRoom::Wardrobe: return true;
 case SanctuaryRoom::TrophyHall: return victories >= 1;
 case SanctuaryRoom::Council: return completedContracts >= 3;
 case SanctuaryRoom::Portal: return victories >= 5;
 }
 return false;
}

// Original Moonfall spell schools remain numeric; these indices are suggestions
// for named hybrids once the UE5 combat event is integrated.
enum class SpellPair { Invalid, SameSchool, Stormfire, Frostfire, Stormfrost };
constexpr SpellPair Weave(int left, int right) {
 if (left < 0 || right < 0 || left > 2 || right > 2)
  return SpellPair::Invalid;
 if (left == right) return SpellPair::SameSchool;
 const int a = left < right ? left : right;
 const int b = left < right ? right : left;
 return a == 0 && b == 1 ? SpellPair::Stormfire
  : a == 0 && b == 2 ? SpellPair::Frostfire
  : SpellPair::Stormfrost;
}
constexpr bool CanWeave(int left, int right, int mana, bool menusOpen) {
 return !menusOpen && mana >= 20 && Weave(left, right) != SpellPair::Invalid;
}

// A council choice applies symmetric constrained faction and region effects.
// Implementation intentionally has no ability to overwrite world saves.
struct CouncilOutcome { int factionDelta; int prosperityDelta; int threatDelta; };
enum class CouncilDecision { ProtectResidents, FundReconstruction, FortifyBorders };
constexpr CouncilOutcome Decide(CouncilDecision decision) {
 switch (decision) {
 case CouncilDecision::ProtectResidents: return {10, 2, -1};
 case CouncilDecision::FundReconstruction: return {5, 4, 0};
 case CouncilDecision::FortifyBorders: return {-3, 1, -4};
 }
 return {0, 0, 0};
}
constexpr int ApplyCouncilDelta(int oldValue, int delta) {
 return Clamp(ClampRelationship(oldValue) + delta, -100, 100);
}
static_assert(RelationshipRank(75) == BondRank::Devoted);
static_assert(WeaponEvolution(100, 8, 5) == WeaponStage::Mythic);
static_assert(SettlementStatus({6, 12, 0, true}) == SettlementTier::Flourishing);
static_assert(SanctuaryRoomUnlocked(SanctuaryRoom::Atlas, true, 0, 0));
static_assert(Weave(0, 1) == Weave(1, 0));
static_assert(Decide(CouncilDecision::FortifyBorders).threatDelta == -4);
} // namespace MoonfallLivingWorld
