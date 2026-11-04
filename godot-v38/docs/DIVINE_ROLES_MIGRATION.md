# Issue #477: Z / Nyra god-and-boss role migration

## Stage 1 (this PR): new-game selection correction

- The main Godot v38 new-adventure screen no longer offers **NEW GAME AS Z**
  or **NEW GAME AS Nyra**.
- New games use the existing procedural custom avatar creator, preserving
  its keyboard/controller focus and no additional avatar artwork.
- The getting-started Tips Codex no longer encourages playing as either god.
- Original `assets/heroes/Z_Original.webp` and `Nyra_Original.png` are
  completely unchanged.

## Backward compatibility and known limitations

**Historical v2 saves with `hero: "z"` or `hero: "nyra"` are still
loadable**, and retain their old abilities, legacy sprites and gameplay data.
This avoids silently deleting or rewriting existing progress. Do not rename
the legacy `hero` field until all derived scripts, the checkpoint codec and
boss encounters have been upgraded and migration tests pass.

This is **not the full issue #477 implementation**. Z/Nyra are not yet
implemented as genuine world encounters by this patch, and existing
save-based appearances and a number of inherited class/combat text references
still need a migration. This PR changes the current *new game entry point*,
not the inherited character semantics or other game distributions.

## Subsequent required stages

1. Add real Z/Nyra gods/bosses as distinct world actors with original artwork
   and existing telegraph, combat, defeat and save mechanisms.
2. Introduce a role-aware save migration preserving prior checkpoint snapshots
   and existing progress; ensure old data are not silently converted.
3. Remove inherited player Z/Nyra class and appearance assumptions, update
   HUD/menus/quests/tutorials, and preserve a valid new-game and load path.
4. Check full CI, real Windows gameplay, boss phases, gamepads, accessibility
   and original artwork byte hashes before closing #477.

## Validation

`python -m unittest tests.test_divine_roles_menu_stage1` from
`godot-v38` checks menu and checkpoint compatibility. It is a source-level
regression, not proof of a successful interactive playthrough.
