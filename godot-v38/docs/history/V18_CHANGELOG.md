# Moonfall Native v18: invisible gameplay grid

- New 60×40 × 3m logical terrain grid for six-region maps, implemented only as memory arrays.
- Blocking terrain sampled from the world StaticBody3D collision shapes; no artificial ground grid lines or visible tiles.
- Enemy avoidance predictive steering uses the logical grid while Godot still performs physical collision resolution.
- Grid lookup for region discovery with world-boundary fallback.
- Eight headless Godot tests include v18-specific test coverage.
- Updated Windows build scripts, CI and Inno Setup spec for v18.
- No changes to original Z, Nyra, or creature images. Compatible with existing v17 saves.
- Windows binary / installer not certified by Windows hardware tests.
