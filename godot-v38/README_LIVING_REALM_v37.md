# Moonfall v37 | Living Realm (source preview, not a verified Windows release)

The active game scene is `res://scripts/native_v37_0.gd`, inheriting v36.3 and the existing native action RPG. **The packaged game's installer version stays v34.4 until gameplay testing and Windows export succeed.** The GitHub repository's older HTML version is not changed by this ZIP.

## Implemented in source

1. **3D lake surface traversal:** Players can enter the centers of all six existing lakes, with camera-relative 3D swim movement at 62% of on-land movement and a reusable moonlit surface wake. No stamina, oxygen or survival clock, and no new map. This is *surface swimming*, not a full volumetric underwater world with diving or underwater caves. Original terrain collisions and hero Sprite3D assets are preserved.
2. **Living ecosystems:** Twelve small, non-colliding 3D moonhares roam around water areas, turn away as the hero approaches, and can be tracked with **G** outside combat for existing Fine Hide and skinning skill. Tracked wildlife returns next in-game day; at most 12 wildlife props and 6 wake meshes exist. Animals are not additional enemy AI or independent combat units.
3. **Professions and gathering contracts:** Three additional recipes in the Professions Atlas: Riverguard Salve, Aquifer Moonsteel Charm, and Marshside Provisions. Six regional, one-time gathering deliveries reuse existing fish/reeds/ore/herbs and pay existing Moon Sigils and gold. No new inventory currency. Recipes and contracts cannot trigger in combat.
4. **World consequences and performance:** Six small ward-beacon meshes visibly react to existing rifts and active lunar invasions. Clearing a real rift grants a modest 5% damage mitigation while in that region, disabled during a fresh invasion. Visual updates are capped at 4 Hz and only update animated wildlife in the player's current region; nonessential props obey the Smooth graphics setting. The existing invasions and bosses are unchanged, without a new army/NPC faction combat system.

## Persistence and compatibility

The existing schema-2 `water_site_days` save dictionary now accepts ids 0 to 29: 0-17 retain existing fish/herb/ore depletion, 18-23 track daily wildlife, and 24-29 store the one-time regional gathering contract completions. Old saves load with no migration. No new save file, schema or currency.

## Unimplemented / pending

- True underwater cave exploration, diving, swim animation rigging, and realistic fluid simulation.
- Fully modeled animated Z and Nyra. **Their original supplied artwork has not been changed**; the existing hero Sprite3D remains active.
- New faction armies, intelligent NPC defenders or world bosses beyond the game's existing enemies/invasions.
- **Actual Godot 4.7.2 engine test**: unavailable in this execution environment. The engine executable is absent and download URL cannot be resolved.
- **Windows 10/11 executable installer and 4K frame-rate validation**: still require the GitHub Windows workflow and a physical/virtual Windows test.

## Build and tests

Run `python tests/verify_v37_living_realm.py` plus `python tests/verify_engine_test_parity.py` for offline checks. With Godot 4.7.2 installed, run `python tools/run_tests.py v37_0_living_realm`, followed by `python tools/run_tests.py --all`. On Windows, `BUILD_WINDOWS.bat` runs all registered suites and exports the game only after passing. `PUBLISH_NATIVE_TO_GITHUB.bat` prepares a separate native branch, leaving the existing DirectX/HTML prototype untouched.

No building, no survival stopwatch, no additional gameplay modes, no change to original Z/Nyra art.
