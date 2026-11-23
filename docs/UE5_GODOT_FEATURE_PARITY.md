# Moonfall: Unreal Engine 5 design goals in the production Godot game

Last reviewed: 2026-11-22. **Feature parity is not engine equivalence.** This maps
UE5 *outcomes* onto existing Moonfall Godot systems, then marks real gaps.
The experimental `MoonfallUnreal/` C++ project is **not** a validated replacement
for Moonfall's production Windows Godot build.

| UE5 reference | Moonfall Godot implementation | Verified today / remaining gap |
|---|---|---|
| Nanite geometry detail | `native_v31_2.gd` MultiMesh foliage, proximity-based LOD, streamed decorative meshes | Batched geometry and LOD in source; **no Nanite virtualized geometry** or equivalent automatic triangle micropolygons |
| Lumen GI/reflections | Forward+ Environment lighting, shadow and fog systems; graphics options expose SSAO/SSIL | Godot supports SDFGI/VoxelGI/SSIL but Moonfall has **not validated** a comparable fully dynamic global-illumination pipeline; no claim of Lumen parity |
| World Partition | `native_v32_0.gd` decorative streaming, zone visibility and existing connected-region logic | World visibility and staging exist; **no verified UE-style async cell streaming and HLOD memory budgets** |
| Niagara VFX | Existing pooled combat impact visuals in `native_v31_2.gd`; native weather/FX rules | Bounded reusable impact cues; no Niagara-equivalent node graph or GPU VFX authoring workflow |
| UE temporal super-resolution | `graphics_rules.gd` + `native_v30_9.gd` with selectable FSR 1.0 and **FSR 2.2** | **This PR adds actual Godot 4 Forward+ temporal FSR2 mode** and renderer/scale fallback. Not Unreal TSR, DLSS, frame generation or a measured 4K FPS gain |
| Chaos physics and destruction | Godot CharacterBody3D collision and gameplay-driven damage | Actor collision works; no Unreal Chaos geometry collection/destruction equivalence |
| Control Rig / Motion Matching | Imported GLB AnimationPlayer/Skeleton3D state mapping in `ai_art_animation_runtime.gd` | Combat clip switching in source; no Control Rig or motion-matching solver |
| Mass AI | Fixed-slot actor reuse, species-aware tactics, optimized/throttled AI and navigation hooks | Real gameplay rules; not Unreal MassEntity ECS or crowd-scale navigation equivalence |
| Blueprints / Gameplay Ability System | Modular GDScript, Godot scenes/resources, combat rules, native talent web | Functional game-specific systems, not drop-in Blueprint or GAS interoperability |
| Win64 high-end renderer | Forward+ with configured D3D12 and Vulkan fallback in `godot-v38/project.godot` | Automated Windows executable/installer smoke; **hardware-specific 4K, controller, GPU and Win10/11 validation still pending** |

## First delivered slice in this draft

- A player-selectable **FSR 2.2** option, with real viewport scaling mode on
  Forward+, while retaining FSR 1.0 and bilinear modes.
- On Mobile/Compatibility, or at render scales over 100%, choose bilinear
  *at runtime*, keeping the player's saved choice for a compatible session.
- Avoid running separate built-in TAA over FSR2's own temporal AA.
- Restore the user's explicit MSAA preference *after* the inherited graphics
  quality routine would otherwise overwrite it.
- Show a precise renderer/fallback note in the Raven Atlas Graphics menu.
- Scene regression tests for choices, renderer fallback, scale, MSAA and
  reset. Godot CI and Windows runner tests are separate from on-device visuals.

## Prioritized follow-up work (not implemented by this document)

1. **Profile real GPUs at 1080p, 1440p and 4K**, comparing native, FSR1 and
   FSR2, including camera motion ghosting, UI sharpness and accessibility.
2. Add **optional**, separately benchmarked dynamic GI lighting presets.
   Preserve existing art and dark-fantasy palette; avoid turning on SDFGI
   globally without performance measurements.
3. Benchmark decor and vegetation **streaming cell budgets** plus occlusion
   culling in the six-region connected world. Prefer reusing nodes over
   allocations on every player movement.
4. Add GPU-friendly **VFX/animation budget counters** for crowded combat,
   but never hide boss/elite hit cues or menu-visible warnings.
5. Validate controller navigation, remapping, dodge timing, saves and actual
   Windows 10/11 installer operation before any merge or engine migration.

## Verification and merge boundaries

The Godot project is the production reference until an Unreal-native build
passes *real gameplay parity*, not just source checks. No Z/Nyra original
images, visual proportions or files may be altered. A successful CI pass is
not proof of UE5-level photorealism, Nanite, Lumen, 4K/60 FPS or manual testing.

References:
- Godot 4.7 feature overview: https://docs.godotengine.org/en/4.7/about/list_of_features.html
- Godot rendering/scaling modes: https://docs.godotengine.org/en/4.5/classes/class_viewport.html
- Godot FSR2 and scale guidance: https://docs.godotengine.org/en/4.5/tutorials/3d/resolution_scaling.html
- Unreal 5 overview: https://www.unrealengine.com/unreal-engine-5
- Current Moonfall Unreal target: `engine-comparison/UE5_MIGRATION_PARITY.json`
