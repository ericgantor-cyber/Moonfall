# Moonfall Native v13 | Verification Summary

Engine: Godot 4.7.2 stable Linux x86_64. Tested in a Linux environment without Windows GUI or controller hardware.

- Import test: **PASS**, 11 unchanged source image assets imported.
- Existing six-region / rift / boss / loot / weather regression (`smoke_v11.gd`): **PASS**.
- Gameplay progression and real save/load regression (`live_v11.gd`): **PASS**, isolated user data directory.
- New v13 tests (`v13_features.gd`): **PASS**. Minimap state and script, power recommendations and scaling caps, three companion stances, controller button dispatch simulation, schema-2 persistence, hero switch, 180 simulated physics frames.
- PCK export: **PASS**.
- Linux Godot runtime plus embedded v13 PCK: **PASS**, 240-frame headless main-scene launch.
- Windows PE x64 executable: **PASS structural checks**, embedded PCK byte hash and Godot footer.
- Source ZIP and Windows ZIP: **PASS ZIP checks**.

**Not yet verified:** A real Windows execution/playthrough; hardware gamepads; visual quality and FPS on Windows; installer compilation; whether Windows SmartScreen warns about this unsigned editor-based executable. These require Windows playtesting.

Original untouched hero asset SHA-256:

- Z: `81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666`
- Nyra: `64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9`

No changes were pushed to GitHub.
