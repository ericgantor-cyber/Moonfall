# Witchwood Wilds: Cinematic 3D vertical slice

This is a high-impact **AAA-inspired** Godot scene improvement in the existing six-region Moonfall world, not a claim that the game now contains commercial AAA assets or full 3D character models.

## New 3D visuals

- 48 deterministic moonwood trees, with 96 layered conical canopies, batched in two `MultiMeshInstance3D` draw groups.
- 32 moss-covered stones batched together and 28 emissive fireflies in another batch.
- A small moonwood ruin gate built from native 3D stone geometry, with a cold moon rune and one bounded accent light.
- A small fill light attached to the existing Z/Nyra player node, only when the player is in Witchwood. Both original `Sprite3D` textures stay exactly unchanged.
- New art is parented under the **existing Witchwood region node** to reuse region-level visibility and culling. Existing roads, shrines, encounters and colliders are untouched.

## Quality / performance

- Existing graphics presets control foliage density, environment detail, LOD, shadows, stone scatter, particles and accent lighting.
- On low settings, optional forest geometry and fireflies disappear; high quality shows the full composition.
- Tree shadows are enabled only when environmental detail is 8+ and shadow quality is High/Ultra. Fireflies and the gate use no additional shadow maps.
- Deterministic seed `2026384` uses an independent `RandomNumberGenerator`. It cannot disturb gameplay spawns or loot.
- The hero light state is checked no more often than four times per second. No scene meshes are created in `_process`.
- All added terrain and shrine pieces are visual-only. There are **no added collision shapes, NPCs, save keys, game modes or currencies**.

## Verification

The v38 regression suite now checks Witchwood root parenting, MultiMesh budgets, empty collision descendants, low/high graphics profiles, and a hero-attached light that disappears when leaving Witchwood.

## Next fidelity steps

To move from the stylized procedural scene to near-photorealistic dark fantasy, commission or author high-quality original trees, rocks, ruins, foliage textures, terrain decals and appropriately optimized 3D characters, and test lighting and frame time on real gaming hardware. Z/Nyra's original artwork must remain preserved. Further art-heavy upgrades should be tested one region at a time.

## v38.5: Original 3D material and silhouette upgrade

- Replaced the primitive Witchwood trunk cylinders with **single reusable branched, tapered, curving trunk meshes**.
- Replaced the conical crowns with **organic overlapping foliage lobes** and remodeled the low-poly stone clusters.
- Applied original 64×64 texture detail to bark, leaves and mossy stone, with generated bump/normal detail through Godot PBR.
- The same four GPU batches are reused by all 48 trees; no individual tree draw-call explosion.
- At low detail, normal-map samples are disabled; high detail uses the richer materials. Existing fog, shadow, foliage and draw-distance settings still apply.
- No original hero art changes, commercial asset imports, new game modes, new collision, or game-save changes.
- This is an original procedural asset foundation, **not a substitute for artist-authored photo-scanned AAA geometry**. Camera framing, readability and GPU performance still need hands-on testing on a real Windows gaming PC.

## Phase 3: Witchwood forest-floor detail

The original-art-only scene now includes **four additional GPU batches**: 96 translucent moss patches, 72 modeled fern clumps, 16 fallen moonwood logs and 24 exposed tree roots. The moss texture is generated once (64×64 RGBA), using a softened uneven edge; fern geometry is generated once and instanced across Witchwood.

The scenery is placed with an independent deterministic seed (`2026386`) and avoids main roads, the shrine area, the central combat approach and the existing moonwood monument. All pieces remain decorative and have no physics bodies. Trees, hero textures, loot/encounters and save data are unchanged.

Existing `environment_detail`, `ground_clutter`, `foliage_quality` and `scenery_lod` preferences control the new density and draw range. On low settings the optional forest floor disappears, while high settings show all 208 additional instances. Mesh and texture resources are shared by their batches, with no per-frame spawn work.

This creates material variation and forest-depth cues without falsely claiming displacement terrain or physically simulated fallen trees. Future realism requires original environment meshes/terrain sculpting and real-hardware profiling.
