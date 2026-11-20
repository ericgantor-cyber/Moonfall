# Moonfall major overhaul: stability-first integration

This draft begins a comprehensive Moonfall overhaul while preserving the current
Godot version and working Unreal features. It does **not** claim the entire game
has already been rebuilt. New gameplay systems must not replace or merge over
untested features or the original Z/Nyra/Morvella artwork.

## First implemented slice: world bootstrap isolation

The six-region, 30-dungeon UE5 game mode now exposes independent settings:
`bGenerateLegacyArena=true` and `bGenerateDungeonWorld=true`. Both default
to the original behavior, but a level-specific game mode can omit the arena
while keeping dungeons, omit dungeon layouts while keeping the arena, or omit
both for a dedicated continent. The existing authored-arena detection also
ignores actors being destroyed. The dungeon batch timer is cleared on EndPlay.
The portable `MoonfallWorldBootstrap.h` policy is tested across all startup
combinations, the 30-layout budget and boundary cases on Windows and Linux.

**This does not create a full unified-world feature.** There is no migration of
Godot assets, no new region streaming, no automatic PR merge and no native UE5
runtime verification here. PR #1074 still provides Northern Expanse's isolated
test environment separately and remains unmerged.

## Second implemented slice: input conflict visibility

`tools/audit_unreal_input_conflicts.py` checks essential input mappings and
prints every exact-key ActionMappings conflict. This preserves existing binds
until manual gameplay tests can determine which collisions are deliberate
context-sensitive menu actions and which are bugs. In particular, a button
serving Jump and a character-menu action must not inadvertently open a panel
while exploring. Shared shoulder buttons used for casting and inventory
operations must be proven properly gated by menu focus. Merely recording these
conflicts does **not** fix them. The audit intentionally does not fail CI just
because an established overlapping binding exists.

## Next integration priorities

1. **Input focus and accessibility.** Eliminate unintended multi-trigger
   actions in gameplay, inventory and options; validate keyboard, mouse and
   XInput controller contexts with a native Unreal Editor build.
2. **Save and respawn.** Verify two-second autosave behavior, shrine discovery,
   safe save migrations, defeated-boss flags, checkpoint respawn and rollback.
3. **Combat.** Verify dodge, parry, guard, stamina regeneration, warning timers,
   chained attack cues and combat feedback, with reduced-motion settings.
4. **World.** Validate six original regions plus Northern Expanse in isolated
   and connected maps, collision continuity, navmesh, map/quest UI and enemies.
5. **Progression.** Verify item-code validation, gear upgrading, equipped stats,
   random loot, quest XP, talent point allocation and Atlas discovery.
6. **Presentation.** Author environment-quality meshes, PBR materials, lighting,
   weather and 4K scalable graphics without modifying approved god artwork.
7. **Performance and shipping.** Measure frame pacing, spawn budgets, memory
   and actor counts; run native Win64 UE5 compilation, packaging, accessibility
   regression and actual Windows installer tests.

## Verification and merge gate

Portable C++17 and static source checks are regression guards, **not** UE5
compilation or playtesting. Keep this and other dependent PRs unmerged until
CI is green, native Windows compilation and gameplay tests are documented,
conflicts are evaluated against the current main branch, and the existing
Godot app and character art remain intact. Do not assert feature parity until
it can be demonstrated in a packaged build.
