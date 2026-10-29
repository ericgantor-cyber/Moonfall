# Moonfall v29 engine test report

Engine: Godot 4.7.2 stable Linux x86-64, headless; test run: 2026-10-27.

All 21 test scripts in `tests/*.gd` exited 0 with no reported test failure or script parse error. The tests include save/load and live world simulation.

```
PASS live_v11
PASS smoke_v11
PASS v13_features
PASS v14_visuals
PASS v15_living_art
PASS v16_visuals
PASS v17_release
PASS v18_grid
PASS v18_stability
PASS v19_world_art
PASS v20_guardians
PASS v21_grand_art
PASS v22_art
PASS v23_animation
PASS v24_1_accessibility
PASS v24_overhaul
PASS v25_legacy_modernization
PASS v26_modular_core
PASS v27_system_modules
PASS v28_ai_scheduler
PASS v29_performance
TOTAL 21 PASS 0 FAIL
```

CPU microbenchmark: V29 CPU PROFILE: 96 direct creature-update calls in 20.31 ms; sampled mean 207.08 us, peak 495 us; idle skips 72, live steps 696 (headless, not FPS)

**These CPU timings are not live gameplay FPS or GPU measurements.**

Godot exported the v29 `.pck` game data pack, a matching Linux runtime launched it headlessly for 300 frames, and the Windows editor-runtime `.exe` contains the pack with a verified footer. The Windows binary was not run on a Windows host.

All 29 inherited PNG/WebP images match v28 byte-for-byte, including Z and Nyra.
