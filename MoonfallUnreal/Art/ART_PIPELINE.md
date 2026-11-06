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
