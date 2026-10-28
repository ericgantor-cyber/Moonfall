# Moonfall Native v11: Godot engine validation

Test environment: user-provided Godot 4.7.2 stable Linux x86_64, headless.

- Official engine reports version `4.7.2.stable.official.ed1daf0bf`.
- `--headless --editor --path PROJECT --import --quit`: PASS, all 11 art assets imported.
- `--headless --path PROJECT --script res://tests/smoke_v11.gd`: PASS, native smoke assertions (six regions, boss phases, rifts, weather, world events, hero resets, etc.).
- `--headless --path PROJECT --quit-after 300`: PASS, 300-frame scene startup.
- `--headless --path PROJECT --script res://tests/live_v11.gd`: PASS, 120 active + 60 loaded hero physics frames, XP/level-up, checkpoint save/read and Z/Nyra switch-and-restore.
- `python tests/verify_v11.py`: PASS, includes immutable original art SHA-256.

**Regression fix:** `tests/smoke_v11.gd` previously checked the initial loot list after simulating event rewards; this falsely failed. The assertion now runs before reward generation.

**Testing boundaries:** Headless simulation does not verify visuals, mouse/controller feel, gameplay difficulty, Windows platform compatibility, Windows export templates, or installer. The live checkpoint test writes to `user://`; run under a dedicated test OS profile or isolated `XDG_DATA_HOME` so personal save files are not overwritten. The ordinary smoke test does not write saves.

Original hero SHA-256:
- Z_Original.webp: `81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666`
- Nyra_Original.png: `64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9`
