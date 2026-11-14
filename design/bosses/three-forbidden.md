# The Three Forbidden (boss concept and phase-controller profile)

Status: **Phase-controller data implemented only**. No spawn integration, encounter AI, wing animation, texture import, UE5 conversion, or playable build is included here.

## Visual reference
User-provided reference: three vertically arranged winged skeletons on a black smoky backdrop. Preserve their grayscale cracked skulls, layered feathered wings, and the top covering ears, middle covering mouth, bottom covering eyes. Source image is a visual reference, **not** yet imported as an asset; confirm rights/permission before distributing image-based game assets. Do not change original Z or Nyra artwork.

## Combat
The `three_forbidden` profile in `boss-phases.js` has four states (phase indices 0-3), switching at 75%, 50%, and 25% of max HP.

| HP | Name | Phase controller attack ID | Intended encounter behavior |
| --- | --- | --- | --- |
| >75% | Phase 1: Hear No Evil | `echo-rupture` | Soundwave rings and wing sweeps |
| <=75% | Phase 2: Speak No Evil | `silencing-curse` | Cursed vocal blasts and marked silence zones, without disabling essential player inputs |
| <=50% | Phase 3: See No Evil | `shadow-wing-strike` | Darkness, sudden movement, and visible shadow-wing strikes |
| <=25% | Phase 4: All Three at Once | `eclipsed-trinity` | All three skeletons attack together: `echo-rupture`, `silencing-curse`, and `shadow-wing-strike` are returned in `concurrentAttacks` |

In phase 4, the boss controller reports the three attack IDs **together** for use by an encounter scheduler. The live combat system does not currently execute simultaneous attacks from this data alone. The encounter implementation must schedule each attack independently in the same attack window, without sequentially replacing one with another, and retain visible safe lanes.


## Encounter event scheduler (implemented, not yet hooked into gameplay)
`Moonfall_7_6_Windows_Launcher_Fixes/three-forbidden-encounter.js` provides a separate pure event scheduler with `createEncounter(maxHP, frame)` and `encounterTick(state, hp, frame)`. Phase 4 activates **all three powers in one frame**, creating three independent warning windows. Their attack events then fire at separate warning deadlines (30, 36 and 42 frames); the scheduler never waits for one power to complete before enabling another. Ongoing cooldowns are independent, so their attack windows can overlap later.

Events are `phase-change`, `warning`, `attack`, and `defeat`. Event IDs: `echo-rupture`, `silencing-curse`, `shadow-wing-strike`. This is logic only: an integration adapter still must turn these events into visible tells, hitboxes, damage, animation and sounds. Test with `node Moonfall_7_6_Windows_Launcher_Fixes/test-three-forbidden-encounter.js`. The scheduler does not alter the existing generic boss combat pipeline.


## Attack pattern overhaul (source complete; not yet wired to a live arena)
`three-forbidden-patterns.js` converts scheduler warning and attack events into renderer-neutral geometry. Sonic `echo-rupture` uses a silver ring-sector with a 70-degree safe opening; `silencing-curse` uses a crimson circular hazard centered on the provided player position; `shadow-wing-strike` uses two crossing rectangular wing lanes leaving corners as safe zones. Warning geometry always has zero damage, and the matching attack geometry exposes nominal damage values for a future combat adapter.

**Critical integration detail:** lock a curse target position when its warning is issued, and replay that same position when the attack resolves; do not recalculate from the moving player. For the ring and wing lanes, keep the warning's chosen geometry unchanged until resolution. The current event scheduler emits event identifiers; a future adapter must cache telegraph geometry, handle collision and dodging/parrying, and enforce the existing global projectile/FX budgets.

The `test-three-forbidden-patterns.js` script checks geometries and a deterministic 4,000-tick logical stress scenario. This is **not** a Windows/UE5 compile, measured performance benchmark, or proof of playability.

## Encounter implementation checklist (not implemented)
- Spawn this as a distinct boss in a dark cathedral/crypt encounter appropriate to Moonfall's unified world.
- Assign HP, contact damage, stagger resistance, arena boundaries, and difficulty scaling via existing game systems.
- Hook the phase state transitions into combat AI; this module currently **returns attack IDs**, including the phase-four `concurrentAttacks` array, not executable attacks.
- Animate each skeleton and wing pair and make every hitbox match the visible animation.
- Display readable telegraphs before damage and offer accessibility-friendly contrast/reduced flashing.
- Make attacks dodgeable/parryable with stamina rules; never rely on sound cues alone.
- Award `forbidden-wing-mantle`, `trinity-bone`, and `fallen-choir-essence` exactly once on defeat, mapping IDs to the inventory/loot table.
- Add unit tests for threshold boundaries, non-regression on healing, defeat rewards, and unknown boss fallback.
- Verify builds and encounter balance on Windows before merging.

## Compatibility
Retains existing dragon, phoenix, screamer, and fallback profiles. The Unreal port needs a separate implementation after the UE5 boss integration point is identified.
