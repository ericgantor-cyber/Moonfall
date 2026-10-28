# v29 performance and Windows build fixes

- Removed an invalid `v28_system_modules.gd` test filename in the Windows build; correctly invokes `v27_system_modules.gd`.
- Added `native_v29.gd` creature tick scheduler with cached per-frame position/time, dead actor early-out, and sampled CPU timing diagnostics.
- Changed repeated combat-threat radius checks to squared-distance comparisons, maintaining the same LOS raycast and radius.
- Added 96-tick headless integration/performance suite (`tests/v29_performance.gd`); CPU timings are not frame rate benchmarks.
- Updated export script, CI workflow and Inno Setup configuration to v29.
- Retained v28 idle safety rules, existing save schema, terrain grid and original artwork.
- Windows installer remains unvalidated on Windows until real machine/CI run confirms installation and uninstall.
