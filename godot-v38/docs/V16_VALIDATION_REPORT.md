# Moonfall Native v16 engine test report

Test engine: Godot 4.7.2 stable Linux x86_64 supplied in this conversation.

- Asset import completed without script/parse errors.
- Godot headless regression suites PASS: `tests/smoke_v11.gd`, `tests/v13_features.gd`, `tests/v14_visuals.gd`, `tests/v15_living_art.gd`, `tests/live_v11.gd`, and `tests/v16_visuals.gd`.
- The original 11 artwork image files match v15 source file SHA-256 digests byte-for-byte.
- PCK pack built through the Godot 4.7.2 engine with Windows Desktop export-pack preset.
- Windows PE x86-64 architecture, embedded GDPC data, footer length, and PCK SHA-256 verified.

Limitations: The Windows executable bundles the Godot **editor** runtime and is unsigned. Actual Windows GPU/graphics performance, Windows launch, keyboard/controller hardware and installer execution have not been tested; headless performance is not an FPS or GPU benchmark. Procedural visuals are not professional authored models, motion capture or PBR assets.
