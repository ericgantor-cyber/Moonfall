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

## Stage 2 (PR for real world encounters)

The live `native_v38_8.gd` scene adds two actual god/boss actors,
Z in Shattered Ruins and Nyra in Eclipse Citadel. Both use the exact original
images as spatial billboards; native boss collision, targeted attacks, warning
telegraphs, existing boss health display, phase behavior, rewards and persistent
`boss_id` records are reused. The encounters are preallocated once at the
end of the beast catalog, not spawned as waves and not attached to the hero
character. Resetting a new adventure reactivates them; reloading defeated
boss IDs despawns them without changing legacy checkpoint keys.

These are first playable encounters using existing phase patterns, not yet
customized Z/Nyra-specific spells, arena staging, or dedicated cinematic
boss animations. Names and original portraits are distinct from player art.
Historical `z`/`nyra` playable saves remain legacy-compatible on purpose.
Issue #477 stays open until full class/save migration and real Windows
combat, input, screen-reader/accessibility and art-fidelity verification.

Source regression test: `python -m unittest tests.test_divine_boss_encounters`.
Real frame pacing and visible on-screen boss placement still require gameplay
verification and are not asserted by this source test.

## Permanent divine blessings (issue #477)

Defeating each god for the **first time in the current adventure** grants a
separate permanent benefit:

- **Z, Void Reaper:** +15% outgoing attack damage, applied through the shared
  combat hit handler (including attacks and damaging spells).
- **Nyra, Eclipse Queen:** +30 maximum health and an immediate +30 health heal
  (capped at the new maximum).

Blessings do not expire on death, reload, leaving the zone or changing gear.
They derive from the pre-existing `defeated_boss_ids` checkpoint list; no
new save schema or artwork is required. Divine victories force an immediate
checkpoint save even if standard autosave is on cooldown. New adventures
start with neither blessing; historical checkpoints without divine defeat
records receive neither. The Nyra restore path reapplies the maximum health
increase after the parent system restores vigor and health.

The bonuses are deliberately bounded and non-stacking. Actual boss rewards,
damage and checkpoint round-trip still require the Godot/Windows CI plus
interactive verification before this can be considered released.

## Enhanced divine encounter tuning

- **Z:** 2.4x the original inherited guardian health, three normal boss
  phases, plus **Void Judgment**, a marked 3m player-position blast
  with a visible 1.7s-to-1.2s windup. Move away from the marked
  position or dodge to avoid damage.
- **Nyra:** 2.7x the inherited guardian health, three normal phases,
  plus **Eclipse Pulse**, a 4.1m ring around Nyra's position with
  the same readable windup and standard dodge mitigation.
- Both signature attacks start with a longer gap, accelerate moderately
  in later phases, use a single persistent warning mesh per boss,
  and pass through the standard incoming damage pipeline. There are
  no additional encounter modes, procedural spawn waves or art edits.
- **Permanent Z blessing:** +15% outgoing damage and +3 HP on enemy
  kills, non-stacking for the adventure.
- **Permanent Nyra blessing:** +30 max HP and 8% multiplicative incoming
  damage mitigation, non-stacking for the adventure. The killing blow
  also gives +30 immediate healing.
- HUD summarizes earned blessings, and existing checkpoint boss IDs
  are authoritative after reloading. Boss deaths immediately persist
  their rewards.

This is still an incremental integration. No live Windows player-input,
framerate, accessibility, or multi-hour checkpoint soak test has been
completed merely by committing this code.

## Phase-three finale: alternating attack geometry

Both divine bosses now switch signature geometry every second cast in
phase three. Z alternates player-position Void Judgment with a larger
boss-centered Void Collapse; Nyra alternates boss-centered Eclipse Pulse
with a larger player-position Eclipse Mark. Each uses a fixed ground
marker, visible full windup and existing dodge/incoming damage rules.
The pattern counter resets on a fresh adventure or encounter reset;
phase-three attacks remain bounded with no minions, waves or per-frame
mesh allocations. These combat upgrades require Windows gameplay
verification before release.
