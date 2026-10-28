# Moonfall v24.1: Stability + VS Code development

- Fixed the Windows GitHub Actions workflow referencing the old v23 build/installer instead of the current project.
- Updated Inno Setup build recipe and Windows build batch for the v24.1 executable.
- Protected checkpoint-writing regression tests: they refuse to run unless Godot user data resolves inside the designated test profile.
- Corrected the offline Inno Setup batch which still pointed at v18 output.
- Added VS Code development tasks, recommended Godot Tools extension, and an automated 16-suite headless test runner.
- Added +/- keyboard camera zoom alongside mouse wheel (disabled when menus are open).
- Added lightweight always-visible hit readout even in Smooth graphics mode. No new physics, loot or save fields.
- Preserved original Z/Nyra art and all existing artwork, zones, AI, save format, and collision grid.

**Validation:** See `TEST_RESULTS_v24_1.txt` supplied after running the engine suite. Windows GPU, installer, and controller-hardware validation still need a Windows machine.
