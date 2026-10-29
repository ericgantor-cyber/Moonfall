# Moonfall v30 test and release verification | 2026-10-27

- Godot 4.7.2 stable Linux x86-64 headless: `--editor --import --quit` exited 0 with no reported parse/script errors.
- **22 / 22 tests passed** when executed in independent Godot processes. Test suites: `smoke_v11`, `v13_features`, `v14_visuals`, `v15_living_art`, `v16_visuals`, `v17_release`, `v18_grid`, `v18_stability`, `v19_world_art`, `v20_guardians`, `v21_grand_art`, `v22_art`, `v23_animation`, `v24_overhaul`, `v24_1_accessibility`, `v25_legacy_modernization`, `v26_modular_core`, `v27_system_modules`, `v28_ai_scheduler`, `v29_performance`, `v30_release`, and `live_v11`.
- Full long-running runner timed out after three suites, and a parallel batch stalled after seventeen. All remaining suites then passed in independent runs. No claim of a single uninterrupted all-suite execution.
- v29 CPU microbenchmark exercised 96 direct creature-update calls under v30 (11.37 ms total for this run), not actual FPS and not an apples-to-apples calibrated baseline comparison.
- Exported Windows PCK via Godot `--export-pack` and launched matching **packed Linux** runtime for 300 frames; exit 0, no reported errors.
- SHA-256 of all **29** original PNG/WebP files matches v29 exactly.
- Embedded EXE: Windows PE source, PCK header, pack length footer, ZIP test integrity checks passed. Windows binary NOT executed in Windows.
- Experimental Windows installer: Windows Go executable compiled with native cross-compiler; appended fixed-allowlist ZIP verified for size and CRC using matching Linux installer code's `--verify`. Installation/shortcut/uninstall operations NOT run on Windows.

Not tested: visual rendering on GPU, input device compatibility on Windows, Windows FPS, Windows runtime start-to-finish gameplay, installer or uninstall behavior, code signing.
