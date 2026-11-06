# Moonfall Unreal art integration

## Safety and source-of-truth
- **Z and Nyra's original art is immutable.** Do not regenerate, restyle, overwrite, or replace `godot-v38/assets/heroes/Z_Original.webp` or `Nyra_Original.png`.
- Original Godot game assets are not implicitly converted into Unreal skeletal meshes.
- Do not import copyrighted Phantom Blade Zero models, shaders, animation, sounds or textures. The reference is behavioral atmosphere, not an asset license.
- Current Unreal environment and crow visuals remain **prototype geometry**, not imported production artwork. No UE 5.4 compile / packaged-build check has been performed.

## Crow asset integration (native fallback retained)
1. Import original Moonfall crow meshes and materials into Unreal Content Browser. For wings, use compatible mirrored pivots and ensure the wing mesh is oriented consistently with the native prototype.
2. Create a Blueprint subclass of `AMoonfallEnemy` (e.g. `BP_CorruptedCrow`).
3. In the subclass class defaults, set `CrowBodyArt`, `CrowWingArt`, `CrowFeatherMaterial`, and `CrowAccentMaterial`. These are optional; the existing built-in primitive meshes remain if unassigned.
4. Create a Blueprint subclass of `AMoonfallArena` and set `CrowEnemyClass` to the crow Blueprint.
5. Create a Blueprint subclass of `AMoonfallUnrealGameMode`, set `ArenaClass` to the arena Blueprint, and select that GameMode for the play map. This prevents accidental use of native-only art defaults.
6. Tune imported mesh scales and collision in Unreal. Bird wing, beak, tail, claw and eye components use native positions/scales until art-specific adjustments are made. Verify the primary body component still blocks suitable spell traces and the player.
7. Replace the temporary primitives with an authored skeletal crow and real flight/dive animation before claiming production-quality creature art. Native component flapping is a fallback only.

## Art direction and asset checklist
| Domain | Art needed | Runtime state |
|---|---|---|
| Corrupted crow | Authored body, layered feathers, talons, glowing eyes, rigged wings, dive and retreat animation | Native multi-piece silhouette with warning light; import slots available |
| Arena | Ruined stone structures, cliffs, vines, ground variation, foliage | Six non-colliding monoliths, two low-cost colored lights |
| Terrain and caves | Sculpted/streamed 3D terrain, caves, navigation and occlusion | **Not yet implemented** |
| Hero portraits / character models | Z and Nyra, exactly matching supplied source images | Original source art preserved; Unreal model migration outstanding |
| Enemy families / bosses | Unique authored meshes, materials, animations and VFX | Existing native placeholders |
| Loot / equipment | Meshes, pickups, rarity VFX and icons | Gameplay data code only |
| HUD / maps | Production textures, accessible icons, legible typography, terrain atlas | Primitive Canvas UI and visited-cell discovery |
| Audio | Original bird calls, attack warnings, environmental ambience | **Not yet integrated** |

## Performance budgets and validation
- Starter encounter: **two corrupted crow mobs** and the existing enemies, no new AI actor per wing/feather.
- Warning light: disabled between attacks, limited radius, no shadow casting.
- Arena atmosphere: **two** point lights without dynamic shadow casting; decorative monoliths have collision disabled.
- Crow near AI: 0.05 s tick; far AI: 0.4 s tick. Verify with Unreal Insights before changing limits.
- Before merging: compile in UE 5.4, test player and bird collision, animation/pivot orientation, all three attacks and interrupts, camera visibility, Windows 10/11 performance, save/restart, and packaged build.

This document is an integration contract, not evidence that finished 3D art was generated or imported.

## Instanced ruins and future environment replacement
The native starter arena now uses a **single instanced static-mesh component** called `RuinPieces` for monoliths, fallen supports, and bounded ground rubble. This reduces component and draw-call overhead versus the old one-component-per-prop approach. `RuinStoneMaterial` can be set on an `AMoonfallArena` Blueprint; `RubbleCount` defaults to 24 and is clamped between 0 and 48. The distribution is deterministic and prop collision/shadows remain disabled, so enemies and the player do not snag on scenic rubble.

Production-art follow-up: replace the cube mesh with authored fractured stones and architecture using the Blueprint component/mesh settings; verify instancing materials, mesh scale, GPU cost and hero readability in Unreal Editor. These are **geometry defaults and art slots**, not imported hero/monster/biome textures. Do not overwrite or restyle Z and Nyra.

## Art-pass expansion: independent floor, walls and enemy family slots

The starter arena now has four additional configurable art references in `AMoonfallArena`:
- `GroundMeshArt` and `GroundMaterial`: the walkable floor.
- `BoundaryMeshArt` and `BoundaryMaterial`: the four enclosing walls.

The `CreateBlock` helper distinguishes ground from boundaries, preserving their original gameplay collision and sizes. The environment already has independent `RuinMeshArt` and `RuinStoneMaterial`, so decorative ruins can be replaced without affecting floor/walls.

The `AMoonfallEnemy` class now also exposes `RegularEnemyArt`, `RegularEnemyMaterial`, `BossEnemyArt`, and `BossEnemyMaterial`, applied only when the enemy is **not** a corrupted crow. Crow-specific overrides and attack behavior remain independent.

**Import checklist:** For each chosen original Moonfall model, verify authoring scale (Unreal centimetres), correct world-up and facing direction, collision response to spells, floor contact, materials, shadow and LOD cost. These properties are editable in an Unreal Blueprint subclass. Hero assets Z and Nyra must remain exactly as supplied. No original source artwork has been modified.

This adds reliable hooks for production meshes and materials, not actual finished materials, models or animation assets. No Unreal runtime compile or Windows packaged test has yet confirmed the overrides.

## Dead-forest and ritual-site art layer

The Unreal starter arena now renders a bounded border of stylized dead-tree silhouettes and seven ritual stones, each category in its own `UInstancedStaticMeshComponent` and all **without collision or shadows**. The forest defaults to ten trees (maximum sixteen); native cube placeholders get crooked branches, while an authored `DeadTreeMeshArt` draws one mesh per tree to avoid repeating placeholder branches. Independent `DeadTreeMaterial`, `RitualStoneMeshArt`, and `RitualStoneMaterial` asset references let artists replace these independently of ruins, ground, walls, enemies, and crows. The center is deliberately left clear for readable boss attacks and quest markers.

This remains source-level prototype set dressing rather than imported biome assets or real foliage wind animation. Check pivots, LODs, bounding volumes, and GPU costs in Unreal before adding any heavier art. Z and Nyra art stays identical.

## Moonlit atmosphere layer

The arena now owns a cool `UDirectionalLightComponent` called `Moonlight` and a blue-black `UExponentialHeightFogComponent` called `GroundFog`. These give the original forest-and-ruins prototype an atmospheric backdrop, independent of the two low-cost colored point lights and red crow attack warning glow. They are **scene components, not ticking AI actors**. Moonlight casts no dynamic shadows by default.

The Blueprint-editable `bEnableMoonlight`, `bEnableLowFog`, and `GroundFogDensity` settings control the look and performance. Fog is bounded to 0–0.025, starting at 0.012. Fog and light must be validated with actual Unreal 5.4 gameplay at dark/bright monitors, ensuring critical crow warning rings, hero silhouettes, loot and quest markers remain readable. Dial fog down or disable it when accessibility or GPU constraints require.

These are native lighting/fog adjustments. Production sky, volumetric clouds, atmosphere textures, PBR materials, finished 3D assets, day/night cycles, and GPU profiling are still not delivered.
