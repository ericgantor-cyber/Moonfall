# v17 Validation Report

- 7/7 native Godot 4.7.2 headless suites passed: `smoke_v11`, `v13_features`, `v14_visuals`, `v15_living_art`, `v16_visuals`, `v17_release`, `live_v11`.
- Godot import and Windows Desktop pack export completed successfully on Linux.
- Combined Windows PE `MoonfallNative_v17.exe`: embedded PCK header/trailer and file lengths verified.
- Linux Godot 4.7.2 runtime with the same embedded PCK ran for 300 frames headlessly without reported script errors.
- Original 11 image assets (Z, Nyra and nine beast images) match v16 byte-for-byte.
- No Windows OS installation or GPU/input hardware test was conducted here.
- Native Inno installer *recipe* included; a compiled installer has not been produced here.
- Not code-signed; Windows SmartScreen may warn.
