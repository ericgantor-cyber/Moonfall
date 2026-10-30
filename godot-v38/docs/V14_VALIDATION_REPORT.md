# Moonfall Native v14: validation

Environment: uploaded Godot 4.7.2 stable Linux x86_64 engine.

- Godot editor asset import passed for 11 original images.
- `tests/smoke_v11.gd` passed after adjusting the wildlife count for the six new owls.
- `tests/v13_features.gd` passed controller-event and save/hero-switch regressions.
- `tests/live_v11.gd` passed the live physics simulation and checkpoint/hero-switch tests.
- `tests/v14_visuals.gd` passed eight species, five bosses, biome geometry, preserved colliders, bars, Smooth/full graphics quality, original art references, loot markings, and auto-expiring VFX.
- Godot packed-game export produced `MoonfallNative_v14.pck`.
- The Windows Godot 4.7.2 engine from the original user upload was combined with the PCK, and its payload was verified byte for byte.
- All 11 original art images match the v13 source ZIP SHA-256 checksums.

**Limits:** Native Windows launch, graphics/hardware controller tests, performance profiling in actual rendering and installer compilation have not been completed. This portable build uses the Godot editor runtime with an embedded game pack; it is not a smaller optimized export-template build. The new art is procedural low-poly geometry, not textured production assets.
