# v30 changelog

- **AI performance:** stagger distant idle creature updates by stable list index, rather than sharing the same zero/default boss ID slot. Keep active attacks and nearby enemy behavior at full update frequency.
- **Diagnostics:** capture average/worst in-loop CPU timings and sleep/active counts for repeatable headless diagnostics. CPU timings are not gameplay FPS.
- **Camera accessibility:** 0 key / controller right-stick press to reset camera zoom.
- **Release stability:** new dedicated regression test, all earlier 21 suites retained, and build script test references repaired.
- **Windows build:** updated game pack, embedded PCK executable, v30 installer recipe and optional experimental custom installer with payload integrity checks.
- **Preservation:** six regions, invisible grid, old save format, and every inherited image kept intact.

**Deferred:** full authored 3D creature rigs, large dungeon expansion, real GPU profiling, real Windows install/uninstall/hardware-controller verification, code signing. No new game mode.
