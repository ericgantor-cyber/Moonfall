# Moonfall v37.5 Bloat and Bug Audit

## Measured before cleanup
- 141 Markdown files in the source tree, including 9 exact duplicate files
  that also appeared in `docs/history/`.
- Long gameplay GDScript inheritance chain. Retained until engine tests can
  prove that a refactor does not break combat, save/load, art, or interactions.
- 78 grotto collision bodies. They remain because each collider describes a
  functional cave surface or pillar; removing them merely to reduce counts
  would reduce collision correctness.

## Code improvements in this release
- Swimmer sprite motion respects the original combat pose and resets through
  the inherited animator when the hero surfaces.
- Ward beacon materials update on clean/tainted changes, not every 0.25 s.
- Water shader night-glow uniforms update only on day/night transitions.
- Flowers do not re-assign the same material each 0.25 s.
- Trim legacy release-note clutter from the lean ZIP; historical notes are
  available separately and original artwork/source files are preserved.

## Not claimed
- No measured FPS/VRAM improvement (Godot runtime unavailable).
- No functioning/tested Windows installer yet.
- No deletion of gameplay systems, item inventories, professions, or hero art.
- No new upgrade mechanics just to raise feature counts.
