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
// Each slot has a distinct named design for every one of the 37 ranks.
constexpr const char* Epithets[37] = {
 "Ashen", "Moonlit", "Thornbound", "Raven", "Ember", "Silver",
 "Grave", "Storm", "Iron", "Dusk", "Starfall", "Vesper",
 "Hollow", "Frost", "Eclipse", "Wildwood", "Bloodroot", "Nightglass",
 "Runic", "Obsidian", "Sunken", "Wraith", "Gloom", "Dawnbreak",
 "Cinder", "Umbral", "Ghostlight", "Wolf", "Vigil", "Astral",
 "Blackstone", "Warden", "Serpent", "Aether", "Ruin", "Nocturne",
 "Evermoon"
};
constexpr const char* SlotTitles[3] = {"Blade", "Plate", "Charm"};
constexpr const char* NamePrefix(int index) {
 return index < 0 || index >= Count ? "Unknown" : Epithets[index / 3];
}
constexpr const char* NameSuffix(int index) {
 return index < 0 || index >= Count ? "Item" : SlotTitles[index % 3];
}
// Crafting level applies to tinkers only. Merchants can still trade any
// affordable item, while crafting higher-tier gear requires progression.
constexpr int CraftLevelRequired(int index) {
 const Entry item = Get(index);
 return item.valid ? 1 + (item.tier - 1) / 5 : 0;
}
constexpr int Next(int selected, int direction) {
 if (selected < 0 || selected >= Count) selected = 0;
 if (direction < 0) return (selected + Count - 1) % Count;
 if (direction > 0) return (selected + 1) % Count;
 return selected;
}
} // namespace MoonfallResidentCatalog
