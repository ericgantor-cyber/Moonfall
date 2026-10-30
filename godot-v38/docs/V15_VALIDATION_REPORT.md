# Moonfall Native v15: verified test coverage

- Environment: user-supplied Godot 4.7.2 stable Linux x86_64, headless.
- Godot asset re-import: passed; 11 images imported.
- `tests/smoke_v11.gd`: PASS, checks region, loot, bosses and game data.
- `tests/v13_features.gd`: PASS, checks controller events, companion state and save regressions.
- `tests/v14_visuals.gd`: PASS, checks procedural fauna and HUD.
- `tests/live_v11.gd`: PASS, checks gameplay simulation, checkpoint state and hero switching.
- `tests/v15_living_art.gd`: PASS, checks 62 creature tracks, 8 patterned species, five guardian coronas, 30 wisps, movement animation, wing flaps, graphics-quality controls and original hero references.
- `--export-pack` to v15 PCK: PASS; packed Linux Godot 4.7.2 runtime successfully started the main scene headless and ran 240 frames.
- SHA-256 of all 11 source artwork images matches v14 source exactly.

### Limitations

Windows game launch, GPU rendering/performance, input hardware and Inno Setup installer not validated in this Linux workspace. Single-file Windows build combines the user-provided Godot 4.7.2 Windows editor runtime with the v15 PCK. It is unsigned, larger than an optimized export-template build and may trigger OS warnings. The visual polish is procedural prototype art rather than fully authored models and animation clips.
