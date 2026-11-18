# Moonfall Unreal: AAA-quality gameplay polish slice (priorities 2–5)

This branch begins the **combat, world/dungeon presentation, RPG progression,
and engineering polish** priorities. It is a coherent **source implementation**,
not a complete AAA-quality game or a verified Windows build.

## 2. Combat feel

- Original **12-hit same-target combat, guard, parry, dodge and posture**
  rules are untouched. Successful combat now produces brief, readable
  textual cues: PERFECT DODGE, PERFECT PARRY / COUNTER READY, and new
  posture-chain milestones at hits 4, 8, and 12.
- A cue is recorded **only on an earned event**, not from opening an
  inventory, an invalid/missed spell, repeated dodge input or guard idle.
  Only damage to a real opponent advances a combo milestone.
- The Canvas overlay is static text without flashing, forced camera shake,
  transient widgets, audio dependencies, or extra actors. The existing
  combat text scaling applies. On defeat cues are cleared.
- Authored UE animation montages and quality impact sound effects still
  require actual asset creation/import and playtesting; the existing hooks
  remain. This patch does not claim new motion-captured animations.

## 3. Dungeon/world presentation

- Every 14-room Unreal dungeon now gets two visual rune pillars per
  chamber, giving the 30-dungeon connected world up to **840 additional
  instanced decorative primitives**, rather than 840 extra ticking actors.
- Pillars use the existing engine cube mesh, non-colliding, shadow-free,
  culled at distances 9–16k units. Height changes with room archetype;
  a deterministic visual seed varies height without touching the streams
  used for hazards, treasure, combat or bonus encounters.
- Props occupy chamber side alcoves, outside the central 360-unit route.
  They do not create traps, mobs, loot, cover collision or altered
  checkpoint triggers. The 30 dungeons and six regions stay connected.
- An optional, **unassigned by default** `ChamberEntrySound` property
  can be connected to a cooked `USoundBase` by an Unreal designer.
  A sound plays at most once on *successful first entry* to an interior
  room and never on a failed encounter-spawn attempt or re-entry. Without
  an assigned sound asset the game remains silent for this cue.
- The separate draft **PR #1054** adds six regional waystations with NPC
  contracts and shrines. This PR does not merge or copy that work.

## 4. RPG gear and crafting progression

- Added `MoonfallForgeForecast.h`, a **pure, deterministic C++17**
  pricing and canonical equipment validator for the existing five-tier
  salvage forge. No new item code, currency, tier, loot probability,
  equipment-slot encoding or saved property.
- A selected inventory item now shows a read-only **Forge 5**
  forecast with the number of affordable upgrades, projected final tier
  and exact total salvage. For invalid gear, display a clear error;
  at tier 25, display **MAX TIER**.
- The real `ForgeInventoryItemLevels` path consults the same forecast,
  then invokes existing validated, transactional one-tier upgrades.
  Equipment sync still uses existing equip logic; a failed craft stops
  without spending salvage on subsequent tiers.
- This is an affordable **upgrade path planner**, not a newly invented
  recipe tree. Broader professions, loot-set expansions and major talent
  systems require their own design and verified gameplay work.

## 5. Performance, accessibility and verification

- Source tests check earned feedback, early-exit guards, menu/defeat
  cleanup, lack of HUD VFX dependencies, optional sound gating and
  first-visit semantics, seed isolation, narrow collision-free side
  landmark placement, and existing forge rollback.
- **C++17 offline test** validates canonical item-code handling and
  forge prices against an independent per-step reference across all
  valid trait/slot/rarity/tier combinations, multiple salvage budgets
  and requested levels. It is compiled with `-O2 -Wall -Wextra -Werror`.
- Small viewports do not display the full cue banner; the underlying
  combat rules remain accessible. The existing reduce-motion setting
  is not bypassed by this text-only feature.
- No original Z or Nyra art, Godot implementation, XP save format,
  serialized loot, authored quest state or World NPC AI logic is changed.

## Native UE5.4 / Windows 10/11 verification still required

1. Compile **Unreal Engine 5.4** for Windows, then open the Entry map.
   Existing source/check workflows are *not* native UE builds.
2. Land successful parries, perfect dodges, 4/8/12-hit sequences.
   Check cues at 720p, 1080p, 4K and with reduced motion enabled;
   verify invalid/missed casts do not advertise a milestone.
3. Check generated and authored dungeons. Verify side decorations
   never obstruct enemy pursuit, line of sight, collision, spawn points,
   navigation, shrine triggers, pickups or room objectives.
4. Assign a valid sound cue to one authored dungeon, verify a single
   first-entry playback and silence on repeated or failed entries.
   With no sound configured, there should be no new audio.
5. Test Forge 5 with insufficient salvage, invalid/duplicate item
   codes, tier 24 and tier 25, equipped items, multiple budgets, the
   actual confirmation controls and save/reload.
6. Benchmark frame time and draw-call impact of 840 instances in
   the full continuous world, with 30 dungeons and six regions.
7. Native-test gameplay and Windows packaging before any merge.

**Merge gate:** draft PR only until those native checks are verified.
