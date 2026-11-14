# The Three Forbidden (boss concept and phase-controller profile)

Status: **Phase-controller data implemented only**. No spawn integration, encounter AI, wing animation, texture import, UE5 conversion, or playable build is included here.

## Visual reference
User-provided reference: three vertically arranged winged skeletons on a black smoky backdrop. Preserve their grayscale cracked skulls, layered feathered wings, and the top covering ears, middle covering mouth, bottom covering eyes. Source image is a visual reference, **not** yet imported as an asset; confirm rights/permission before distributing image-based game assets. Do not change original Z or Nyra artwork.

## Combat
The `three_forbidden` profile in `boss-phases.js` has four states (phase indices 0-3), switching at 75%, 50%, and 25% of max HP.

| HP | Name | Phase controller attack ID | Intended encounter behavior |
| --- | --- | --- | --- |
| >75% | Hear No Evil | `bone-wing-sweep` | Sweeping feathers and directional bone strikes |
| <=75% | Echo of Hearing | `echo-rupture` | Expanding sonic rings with clear gaps |
| <=50% | Speak No Evil | `silencing-curse` | Marked curse zones; visible signal before burst; avoid removing core player input |
| <=25% | See No Evil: Eclipsed Trinity | `eclipsed-trinity` | Three synchronized patterns with readable danger zones and safe lanes |

## Encounter implementation checklist (not implemented)
- Spawn this as a distinct boss in a dark cathedral/crypt encounter appropriate to Moonfall's unified world.
- Assign HP, contact damage, stagger resistance, arena boundaries, and difficulty scaling via existing game systems.
- Hook the phase state transitions into combat AI; this module currently **returns attack IDs**, not executable attacks.
- Animate each skeleton and wing pair and make every hitbox match the visible animation.
- Display readable telegraphs before damage and offer accessibility-friendly contrast/reduced flashing.
- Make attacks dodgeable/parryable with stamina rules; never rely on sound cues alone.
- Award `forbidden-wing-mantle`, `trinity-bone`, and `fallen-choir-essence` exactly once on defeat, mapping IDs to the inventory/loot table.
- Add unit tests for threshold boundaries, non-regression on healing, defeat rewards, and unknown boss fallback.
- Verify builds and encounter balance on Windows before merging.

## Compatibility
Retains existing dragon, phoenix, screamer, and fallback profiles. The Unreal port needs a separate implementation after the UE5 boss integration point is identified.
