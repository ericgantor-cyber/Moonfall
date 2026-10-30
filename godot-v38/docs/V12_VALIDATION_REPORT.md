# Moonfall Native v12 Runtime and Source Validation

Validated by **Godot 4.7.2.stable.official.ed1daf0bf** (user-uploaded Linux engine) in headless mode:

- Project import (`--editor --import --quit`): PASS
- `tests/smoke_v11.gd` compatibility/regression suite: PASS
- `tests/live_v11.gd`: PASS (physics frames, level progression, save/load hero-switch, world state)
- `tests/verify_v12.py`: PASS (art hashes, source contracts, launcher/pck/installer references)
- `--export-pack Windows Desktop`: PASS, outputs MoonfallNative_v12.pck
- Packaged-game startup test: paired renamed **Linux Godot editor binary** with the PCK in a clean folder and ran for 300 frames (`--headless --quit-after 300 --verbose`); main scene and both hero texture resources loaded, without reported errors.

NOT verified: running the user-supplied Windows editor executable or its graphics/audio/input, Windows installer compilation, extended gameplay, performance benchmarks or Windows antivirus/SmartScreen behavior. Windows program uses same verified game pack and corresponding official Godot 4.7.2 Windows editor as a direct-launch runtime. This is not a template-exported standalone optimized game binary. The EXE requires its adjacent PCK.

**Character art unchanged (SHA-256)**

- Z_Original.webp: 81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666
- Nyra_Original.png: 64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9
