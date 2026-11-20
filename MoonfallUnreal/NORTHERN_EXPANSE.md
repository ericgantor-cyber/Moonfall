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

## The other two gods: Z and Nyra

Z and Nyra now have **proposed additional Northern Expanse sanctums**, alongside Morvella. These are new narrative destinations, not changes to their current boss locations or art.

- **Z, the Void Reaper:** **Starveil Vale** (province 8), at **The Void Observatory**. His existing encounter ID remains `z_void_reaper`.
- **Nyra, the Eclipse Queen:** **Nightspire Dominion** (province 9), at the **Eclipse Throne of Nyra**. Her existing encounter ID remains `nyra_eclipse_queen`.
- **Morvella, the Crimson Reaper:** **Gravewind Barrens** (province 4), at the **Moonlit Necropolis**, unchanged from her planned relocation.

These sanctums are optional divine destinations, independent of the regular region dungeon bosses. Their placement catalog and new `MoonfallNorthDivine.h` policy refuse invalid discovery masks and only report a site as accessible when the province and an unbroken route from Shiverwake Coast are discovered. Existing victory state can be supplied to prevent repeat availability, but NO victory persistence, new boss actors, Atlas destinations, geometry or save fields are added here. The policy is not live gameplay.

**Important:** Z and Nyra keep their original approved artwork and appearances, their divine identities, and their existing world/arena encounters. Avoid duplicate instances until designers explicitly choose a migration strategy and verify all save/load and victory behavior. Morvella's old sixth-region spawn remains disabled by default in the separate PR #1075.

### Sanctum layout foundations

A new portable layout catalog defines three independently bounded, accessible encounter spaces: Morvella's 32-meter approach with a 12.5-meter clear arena radius; Z's 28-meter approach and 14-meter radius; Nyra's 36-meter approach and 15-meter radius. All main entrances are at least 5 meters wide, all provide a return path, and static cover budgets are capped. A regression test rejects blocked returns, narrow entrances, and placement in the wrong province.

**These are design dimensions, not placed UE5 geometry or verified traversal.** Before a playable implementation, author actual landscapes, collision, entrance roads, enemy AI, visible warning cues, shrines, respawn logic and navigation meshes. Test mouse/controller accessibility and emergency exit behavior at all phases. Never duplicate an existing god actor or replace original Z/Nyra art.

### Divine sanctum geometry authoring plans

`MoonfallNorthSanctumGeometry.h` now emits bounded, deterministic local block plans for each of the three divine sanctums. Each has a broad connected entrance floor, an unobstructed arena floor, two side walls, a back wall and nonblocking entrance/boss position markers. The front edge remains open for a safe retreat. Geometry plans are tested for floor continuity, minimum width and nonblocking markers in Linux/Windows C++17 CI.

This is **authoring data, not instantiated UE5 collision or a navigable landscape**. The next implementation step is to consume these plans within real level-building code, place them into World Partition with authored zone coordinates and validate navmesh, animation, performance and actual boss spawn rules. Existing boss actors must not be duplicated and original divine art must stay unchanged.

### Editor-placeable UE5 sanctum geometry prototype

The new `AMoonfallNorthSanctumActor` is a UE5 actor that consumes the three tested native-free layouts and constructs five instanced static-mesh collision blocks: entrance corridor floor, arena floor, two side walls, and back wall. Place one actor in a real level, choose `DivineSiteIndex` 0 (Morvella), 1 (Z), or 2 (Nyra), and position/rotate the actor in the target province. The editor rebuilds instances on construction without a Tick. It exposes entrance and boss marker positions but deliberately spawns no gods, creates no saves and does not modify their artwork.

**Not complete:** No actor has yet been placed in a packaged northern map; no world streaming, navmesh baking, travel connection, or live boss integration is proven. The generated grey-box rectangles are square and their floor contact requires native collision/walk tests. Approved game assets and visuals must be added separately.

### Nine-zone physical prototype overhaul

A new `AMoonfallNorthZoneActor` provides **editor-placeable UE5 greybox geometry for each of the nine provinces**. Designers choose `ProvinceIndex` (0-8) and place the actor in the appropriate World Partition level. `MoonfallNorthZoneGreybox.h` defines a 24-meter-wide town-hub plaza, connected eight-meter-wide northern and southern road sections, a 24-meter dungeon approach forecourt and a broad southern entrance pad. Roads meet exactly at floor boundaries. A nonblocking world-space sign displays the province name, hub and dungeon title. Geometry uses five static mesh instances per province, no Tick, and actor functions expose the dungeon-approach and southern-entrance positions.

The project now includes separate C++17 floor-continuity regression tests and a Python source-policy test in Linux/Windows CI. **These are standalone actors and greybox surfaces, not an interconnected playable continent.** No provinces have been added to the existing six-zone world map, saved discovery ledger, Atlas, roads between individual placed province actors, or World Partition map assets. Designers must place actors at tested coordinates, align roads between zones, add terrains/navmesh and quest/AI/world content, then validate packaged Windows UE5 gameplay.

### One connected continent prototype

In addition to individual province actors, `AMoonfallNorthContinentActor` is a new **editor-placeable UE5 connected-continent greybox actor**. It consumes `MoonfallNorthContinentLayout.h` and builds a single collision-enabled floor pad for each of the nine provinces, plus one rotated road segment per undirected zone link (13 roads in the current graph). The pads and roads share the same top elevation and physically overlap at each end. Geometry is static, uses two instanced-mesh components, and does not Tick. A Blueprint-callable getter exposes province center positions so designers can later attach shrines, settlements, dungeon entrances and sanctums.

This is the first codepath that can produce a connected *physical* nine-zone road network inside a real Unreal map when an editor user places the actor. It **does not** mean the asset is already placed in a committed .umap or available in a packaged game. Terrain materials, biome landmarks, authored collision/navmesh validation, streaming, enemy/quest populations, Atlas discovery, and final coordinate separation from the legacy world remain outstanding. Use a separate Northern Expanse test map and never spawn a second copy of Z, Nyra or Morvella from this geometry actor.

### Biome landmark pass

The optional continent greybox now generates **18 biome landmark placeholders**, two in each of the nine provinces. Their static heights and widths vary by region: coastal ice formations, towering timberland silhouettes, tundra boulders, Gravewind grave spires, Verdant Rift columns, mountain pillars, Sunken Crown ruins, Starveil crystals and Nightspire monoliths. The existing hub pads and 13 connecting road segments remain intact. Landmarks use a separate instanced mesh component with collision and overlap events disabled so they cannot block player movement or road travel.

These are intentionally untextured cube placeholders, not finished biome artwork or playable environmental hazards. Follow-up UE5 work must replace them with original appropriately licensed art, define terrain/foliage/weather/audio, author distinct silhouettes and test navigation and visibility on Windows packaged builds. C++17 regressions enforce 18 bounded placements away from the nine hub pads. Native engine verification remains outstanding.

### Provincial wayfinding signage

The connected-continent UE5 prototype now has nine **static, nonblocking in-world signs**, one at each provincial hub. Each displays its region name, suggested level and settlement hub, making the separate biomes readable while traversing the greybox. Sign components are created once, updated in editor construction, and do not Tick. They deliberately omit undiscovered dungeon lore, divine sanctum names and other quest spoilers. Source regression checks assert nine components, no collision, and absence of divine site labels.

These signs only become visible after a designer places the continent actor in an actual Unreal map. No world save, quest unlock, fog-of-war, dynamic weather, or packaged map placement is implemented by this update. Level and controller readability must be tested in UE5 before merge.

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
