# Moonfall loot art + combat HUD integration

**Status:** Source changes on a dependent draft integration PR. Not verified
in Unreal Engine 5.4 or a Windows packaged build.

## Compatibility and dependencies

This branch is built on the combined HUD layout editor and AAA combat
branch (#1056), itself dependent on the AAA combat PR (#1055).
It independently incorporates the original loot art work from #1053.
None of these draft PRs is merged to `main`.

- Preserves **all six original source PNG atlas blobs exactly**, the gallery,
  importer, asset selection policy, source C++17 tests, and native
  `MoonfallGroundLoot` silhouettes from #1053.
- Merges `MoonfallCombatHUD.cpp` by retaining #1056's movable resource orbs,
  world quest tracker and boss warnings; 11 Esc/Options settings rows; and
  enemy-only floating combat text with player-silhouette protection.
  Selected equipment and inventory rows gain framed, rarity-marked loot icons.
- Does **not** alter equipment codes, collected-item ownership, XP, player HP,
  loot RNG, save format, boss damage, original Z and Nyra artwork, or Godot.
- The atlases are **source PNGs, not cooked .uasset textures**. Run
  `MoonfallUnreal/Build/Import-Loot-Art.py` inside a working UE5.4 Editor
  for the six textures to display. If an atlas is unavailable, deterministic
  built-in Canvas silhouettes remain as fallback.

## Verification matrix

GitHub CI performs these **source-only** checks:
- `python3 tools/check_unreal_source.py` for existing engine/source contracts.
- `python3 -m unittest -v tools.test_unreal_loot_art` for PNG chunk integrity,
  importer rules and native pickup art source.
- `python3 -m unittest -v tools.test_unreal_loot_hud_integration` for
  11-row Options, collision-free pickups, inventory icon validation,
  original HUD layout, enemy-anchored PARRY/DODGE/BLOCKED/MISS labels,
  and zero extra enemy scans.
- `c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
  tools/test_unreal_loot_art_rules.cpp -o /tmp/moonfall_loot_art_rules`
  and the resulting binary for equipment/atlas mapping.
- `node tools/test_moonfall_loot_art_gallery.mjs` for offline art gallery
  behavior.

**Native Unreal verification is mandatory before merging:** Install UE5.4,
run the importer, compile Editor and Win64 game targets, confirm six atlases
cook, inspect inventory rows and ground pickups at 720p/1080p/4K,
test controller/mouse/keyboard HUD editing, keep floating results above
attackers and never covering the player, measure FPS during heavy loot and
combat, and test saving, loading, closing, restarting and Windows packaging.

CI passing its source-only or Windows **preflight** jobs cannot replace
an actual Unreal Editor compile and gameplay test.
