# Imported creature animation performance (Moonfall)

Moonfall polls native creature combat/movement states for approved 3D GLB rigs, while
Godot's `AnimationPlayer` continues playing each imported animation normally.

## Adaptive state polling

- Near actors (up to 16m) are checked at the existing 80ms polling interval.
- High quality checks distant unchanged idle/move states every second tick beyond 32m.
- Balanced quality polls medium-distance idle/move states every second tick and far states every third tick.
- Eco polls medium-distance idle/move every second tick and far states every fourth tick.
- **Every state change**, including movement starts/stops, attack, hurt, death, spawn and telegraph, is processed on the next normal 80ms tick regardless of distance. Critical states stay active on every tick. A stable instance ID distributes other updates across ticks.
- Guardian visibility and the existing 45m range remain enforced. All quality tiers share the same combat logic, physics, damage, enemy counts and loot.

This is a **polling** optimization, not an animation frame-rate reduction, and
not a performance guarantee. The engine still updates playing imported animations.
If GPU skinning or high-poly meshes remain expensive, review the separate
`ai_art_budget.py` report and profile GPU frame time on actual Windows hardware.

## Verification

`tests/navigation_atlas_integration.gd` checks near/far frequency rules,
adapting to quality tiers, deterministic scheduling and unthrottled combat
transitions. The Godot and Windows CI pipelines must pass before merging.
Z and Nyra's original artwork and gameplay are not changed.
