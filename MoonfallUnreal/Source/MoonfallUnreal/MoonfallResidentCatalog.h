#pragma once
// Deterministic 111-entry catalogue using the existing 3-slot item encoding.
// Pure C++17 to allow regression testing outside the Unreal editor.
namespace MoonfallResidentCatalog {
constexpr int Count = 111;
struct Entry {
 int code;
 int slot;
 int rarity;
 int tier;
 int merchantCost;
 int tinkerCost;
 bool valid;
};
constexpr Entry Get(int index) {
 if (index < 0 || index >= Count) return {0,0,0,0,0,0,false};
 const int slot = index % 3;
 const int step = index / 3;             // 37 ranks per slot
 const int tier = step % 25 + 1;         // existing inventory tier bounds
 const int rarity = 1 + step / 25;       // uncommon and rare, never invalid
 const int code = slot * 10000 + rarity * 1000 + tier;
 const int baseCost = 8 + tier * 2 + rarity * 5;
 return {code, slot, rarity, tier, baseCost, baseCost + 8, true};
}
constexpr int Next(int selected, int direction) {
 if (selected < 0 || selected >= Count) selected = 0;
 if (direction < 0) return (selected + Count - 1) % Count;
 if (direction > 0) return (selected + 1) % Count;
 return selected;
}
} // namespace MoonfallResidentCatalog
