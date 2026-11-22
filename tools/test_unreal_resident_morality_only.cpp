// Native-free C++17 contract for Moonfall's proposed vendor catalog and ethics.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallResidentCatalog.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallMoralityRules.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

namespace Catalog = MoonfallResidentCatalog;
namespace Ethics = MoonfallMorality;

static_assert(Catalog::Count == 111);
static_assert(Catalog::Get(0).valid && !Catalog::Get(-1).valid);
static_assert(Catalog::CraftLevelRequired(72) == 5);
static_assert(Catalog::CraftLevelRequired(75) == 1);
static_assert(Ethics::Apply(100, INT_MAX) == 100);
static_assert(Ethics::Apply(-100, INT_MIN) == -100);
static_assert(Ethics::Apply(1, INT_MAX) == 100);
static_assert(Ethics::Apply(-1, INT_MIN) == -100);
static_assert(Ethics::AdjustShopPrice(100, 60) == 90);
static_assert(Ethics::AdjustShopPrice(100, -60) == 120);

int main() {
    std::set<int> codes;
    std::set<std::string> names;
    int slots[3] = {0,0,0};
    for (int index = 0; index < Catalog::Count; ++index) {
        const auto item = Catalog::Get(index);
        assert(item.valid);
        assert(item.slot >= 0 && item.slot < 3);
        assert(item.rarity >= 1 && item.rarity <= 5);
        assert(item.tier >= 1 && item.tier <= 25);
        assert(item.code == item.slot * 10000 + item.rarity * 1000 + item.tier);
        assert(item.merchantCost > 0 && item.tinkerCost > item.merchantCost);
        assert(codes.insert(item.code).second);
        const std::string name = std::string(Catalog::NamePrefix(index)) + " " + Catalog::NameSuffix(index);
        assert(names.insert(name).second);
        assert(std::strlen(Catalog::NamePrefix(index)) >= 3);
        assert(std::strlen(Catalog::NameSuffix(index)) >= 3);
        assert(Catalog::CraftLevelRequired(index) >= 1);
        assert(Catalog::CraftLevelRequired(index) <= 5);
        assert(Catalog::Next(index, 1) == (index + 1) % Catalog::Count);
        assert(Catalog::Next(index, -1) == (index + Catalog::Count - 1) % Catalog::Count);
        assert(Catalog::Next(index, 0) == index);
        ++slots[item.slot];
    }
    for (int slot : slots) assert(slot == 37);
    assert(codes.size() == Catalog::Count);
    assert(names.size() == Catalog::Count);
    assert(!Catalog::Get(Catalog::Count).valid);
    assert(Catalog::CraftLevelRequired(-1) == 0);
    assert(Catalog::CraftLevelRequired(Catalog::Count) == 0);
    assert(std::strcmp(Catalog::NamePrefix(-1), "Unknown") == 0);
    assert(std::strcmp(Catalog::NameSuffix(Catalog::Count), "Item") == 0);
    assert(Catalog::Next(-1, 1) == 1);
    assert(Catalog::Next(Catalog::Count, -1) == Catalog::Count - 1);

    assert(Ethics::Clamp(-1000) == -100 && Ethics::Clamp(1000) == 100);
    assert(Ethics::Apply(95, 10) == 100);
    assert(Ethics::Apply(-95, -10) == -100);
    assert(Ethics::Apply(20, -5) == 15);
    assert(Ethics::Apply(-20, 5) == -15);
    assert(Ethics::Apply(INT_MAX, INT_MIN) == -100);
    assert(Ethics::Apply(INT_MIN, INT_MAX) == 100);
    assert(Ethics::Apply(0, 0) == 0);
    assert(Ethics::ResidentActionDelta(true) == 10);
    assert(Ethics::ResidentActionDelta(false) == -10);
    assert(Ethics::ResidentSalvageDelta(true) == -10);
    assert(Ethics::ResidentSalvageDelta(false) == 5);
    assert(Ethics::ResidentBondDelta(true) == 6);
    assert(Ethics::ResidentBondDelta(false) == -6);
    assert(Ethics::ResidentReputationDelta(true) == 2);
    assert(Ethics::ResidentReputationDelta(false) == -4);
    assert(Ethics::CouncilDelta(0) == 10 && Ethics::CouncilDelta(1) == 8 && Ethics::CouncilDelta(2) == 0);
    assert(Ethics::QuestCompletionDelta() == 4);
    assert(Ethics::IsValidAction(-1) && Ethics::IsValidAction(1) && !Ethics::IsValidAction(0));
    assert(std::strcmp(Ethics::Alignment(-60), "Dreaded") == 0);
    assert(std::strcmp(Ethics::Alignment(-20), "Cruel") == 0);
    assert(std::strcmp(Ethics::Alignment(20), "Kind") == 0);
    assert(std::strcmp(Ethics::Alignment(60), "Exalted") == 0);
    assert(std::strcmp(Ethics::Alignment(0), "Unaligned") == 0);
    assert(Ethics::WorldTier(-60) == -2 && Ethics::WorldTier(-25) == -1);
    assert(Ethics::WorldTier(-24) == 0 && Ethics::WorldTier(24) == 0);
    assert(Ethics::WorldTier(25) == 1 && Ethics::WorldTier(60) == 2);
    assert(Ethics::AdjustShopPrice(100, -25) == 110);
    assert(Ethics::AdjustShopPrice(100, 25) == 95);
    assert(Ethics::AdjustShopPrice(100, 0) == 100);
    assert(Ethics::AdjustShopPrice(1, 60) == 1);
    assert(Ethics::AdjustShopPrice(0, 60) == 0);
    assert(Ethics::AdjustShopPrice(1000001, 0) == 0);
    assert(Ethics::AdjustShopPrice(1000000, -60) == 1200000);
    assert(std::strcmp(Ethics::WorldState(60), "Beacon of Hope") == 0);
    assert(std::strcmp(Ethics::WorldState(-60), "Under Shadow") == 0);
    assert(std::strcmp(Ethics::GearTheme(60, true), "Radiant") == 0);
    assert(std::strcmp(Ethics::GearTheme(-60, true), "Dreadbound") == 0);
    assert(std::strcmp(Ethics::GearTheme(60, false), "None") == 0);
    assert(std::strcmp(Ethics::GearVisualPromise(60, true), "Gold motif") == 0);
    assert(std::strcmp(Ethics::GearVisualPromise(-60, true), "Crimson motif") == 0);
    assert(std::strcmp(Ethics::GearVisualPromise(60, false), "No equipped gear") == 0);

    std::cout << "MOONFALL RESIDENT CATALOG + MORALITY POLICY PASS (111 entries)" << std::endl;
    return 0;
}
