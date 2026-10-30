# Moonfall: 1,111-improvement engineering roadmap

**Status:** Planning backlog, not 1,111 completed upgrades. Work only on draft PR #88 until build, gameplay and installer verification pass.
**Art contract:** Never modify Z_Original.webp or Nyra_Original.png, or replace their in-game appearance.
**Quality contract:** No filler features or redundant systems. Prefer measurable fixes, small changes and regression tests.

## Workstreams
1. **Build, installer and CI** — 101 scoped improvement slots. Fix Godot headless tests; verify Windows 10/11 packaging, signed/unsigned warnings, shortcuts, clean install/uninstall.
2. **Combat feel and controls** — 101 scoped improvement slots. Input buffering, hit confirmation, attack pacing, responsiveness, remapping and controller parity.
3. **Enemies and boss AI** — 101 scoped improvement slots. Telegraphs, fair damage windows, navigation, pathing, phases, attack variety and spawn pacing.
4. **World and dungeon rooms** — 101 scoped improvement slots. Room transitions, exploration, streaming, hazards, biomes and performance-conscious world detail.
5. **Character progression** — 101 scoped improvement slots. XP pacing, leveling, talents, readable upgrades, rerolls and build diversity.
6. **Items, loot and crafting** — 101 scoped improvement slots. Loot tables, drop balancing, recipes, economy, professions, shops and inventory reliability.
7. **Graphics, animation and VFX** — 101 scoped improvement slots. Lighting, animation clarity, graphics presets, effects, readability, art preservation.
8. **Performance and stability** — 101 scoped improvement slots. Memory bounds, pooling, LOD, frame-time budgets, leak detection and save integrity.
9. **UI and accessibility** — 101 scoped improvement slots. HUD, readable labels, scalable settings, input flexibility, menu navigation and feedback.
10. **Audio and game feel** — 101 scoped improvement slots. Original audio assets, mixing, impact cues, accessibility options and atmosphere.
11. **Testing, documentation and release** — 101 scoped improvement slots. Automated regressions, Windows smoke tests, crash reports, changelog and reproducible releases.

**Capacity:** 11 workstreams x 101 candidate improvements = 1111 planned slots. A slot is not complete until a tangible change is committed, tested and documented.

## Immediate release blockers and priorities
1. Capture exact Godot/Windows CI failure from the latest run, fix the actual cause, and rerun.
2. Complete all import, headless gameplay, graphics, AI, saving and inventory tests.
3. Confirm no save-test can write into live player profile folders.
4. Confirm third-person mouse-wheel zoom, camera-relative movement, controls and menu guards.
5. Revalidate original hero asset hashes and visuals.
6. Generate Windows executable and installer, confirm installer artifact exists.
7. Verify clean installation, desktop shortcut, launching, save/load and uninstall on Windows 10 and 11.
8. Investigate performance bottlenecks with a reproducible profile before expanding enemy/world counts.
9. Upgrade combat, bosses, and environments in focused, independently testable batches.
10. Only merge when CI succeeds and deliverables are checked.

## Definition of done per improvement
- Describes a specific player or developer problem and acceptance criteria.
- Implements the smallest maintainable fix without duplicating systems.
- Keeps original hero art untouched and preserves existing save compatibility.
- Adds a meaningful regression test or documented manual verification as appropriate.
- Passes source checks and applicable Godot/Windows CI; if not, mark as unverified.

## Reporting
Track **committed**, **CI-verified**, **installer-verified** and **planned** counts separately. Existing camera regression assertions count as tests, not new gameplay features. Never imply all 1,111 are shipped.
