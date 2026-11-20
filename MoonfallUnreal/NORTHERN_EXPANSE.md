# MOONFALL: Northern Expanse

An original nine-province northern continent designed around Northrend-style geographic variety, without copying WoW locations, characters, artwork, or quests.

## Provinces and progression

1. Shiverwake Coast: glacial landfall, raider camps; hub Tideglass Haven; dungeon Saltglass Grotto; boss The Brine Warden.
2. Wolfsong Timberlands: haunted pines; Emberpine Lodge; Rootfang Den; The Thorn Matriarch.
3. Palehorn Tundra: roaming frost beasts; Greyhorn Outpost; The White Maw; Palehorn Colossus.
4. Gravewind Barrens: oathbound dead; Lantern Rest; Hushed Sepulcher; The Bellkeeper.
5. Verdant Rift: unnaturally warm basin; Bloomwatch Refuge; Heartseed Hollow; The Root Devourer.
6. Skyshard Summits: storm giants; Cloudbreak Hold; Thunderbrand Vault; The Skybreaker.
7. The Sunken Crown: ruined clockwork capital; Lastlight Enclave; Crownfall Archive; The Regent of Rust.
8. Starveil Vale: crystalline astronomy valley; Prismwatch; The Mirror Choir; The Prism Oracle.
9. Nightspire Dominion: final eclipse citadel; Dawn's Edge; Obsidian Meridian; The Uncrowned Star.

The two starting paths leave Shiverwake Coast for forest or tundra, reconnect through burial plains, basin and storm mountains, and converge toward the final fortress. Every province has reciprocal graph neighbors and the continent graph is connected.

## Morvella's relocated home

**Gravewind Barrens** is now the canonical home of **Morvella Velmora, the Crimson Reaper**. Her Moonlit Necropolis is a divine side-sanctum in this province, reached through a future authored graveyard approach from Lantern Rest. She remains the Goddess of Death, with the same four-phase encounter identity, original approved appearance, and separate permanent victory flag.

The Northern Expanse catalog binds her stable encounter ID (death_goddess_crimson_reaper) to Gravewind Barrens (catalog index 3); this does not spawn a boss or build a level. On the separate Death Goddess draft PR #1075, her former sixth-region world spawn has been disabled by default to prevent the boss appearing in two lands. Full playable relocation requires integrating both draft PRs, authoring accessible northern roads and necropolis terrain, and testing packaged UE5 Windows builds. The legacy test arena remains optional.

### Morvella discovery and victory policy

The portable `MoonfallNorthMorvella.h` access policy supports a future separate nine-bit northern discovery ledger. Morvella remains hidden until Gravewind Barrens itself is discovered and an entire route from Shiverwake Coast through other discovered provinces is connected. A disconnected discovery alone cannot unlock the encounter. When her existing permanent victory flag is set, the access policy returns defeated rather than available. Invalid masks fail closed.

This policy is **not connected to the active save schema, Atlas, UE5 actors, or world travel**. The encounter remains unavailable in the playable Northern Expanse until the continent's actual levels and backward-compatible discovery saves are implemented. All 512 discovery-mask combinations are covered by native-free regression tests.

## Current implementation

- Original read-only nine-zone C++17 catalog under Source/MoonfallUnreal/MoonfallNorthZones.h.
- Self-contained native-free tests at tools/test_unreal_north_zones.cpp.
- Preserves the legacy six zones, original 30 dungeon save bits, six-act Chronicle, and divine identities and original artwork of Z and Nyra.

## Not implemented

No actual UE5 landscapes, roads, enemies, encounters, dungeons, loot, quest NPCs, world navigation, Atlas tabs, save fields, or travel have been created by this catalog.

## Practical next steps

1. Run native-free C++17 test compilation on GCC, Clang and MSVC.
2. Introduce a versioned continent-specific identifier and backwards-compatible save migration, without reinterpreting old 30-bit dungeon masks.
3. Build a playable Shiverwake Coast World Partition prototype, road network, travel hub, exploration discovery, enemy patrols, and one real dungeon.
4. Implement travel links and gradually expand the rest of the continent, preserving the existing starting sanctuary and six original regions.
5. Test mouse/controller accessibility, map fog, performance at 30/60/120 FPS, 720p to 4K, save/load and packaged Windows UE5 builds before merge.

**PR remains draft until native UE5 gameplay verification.**
