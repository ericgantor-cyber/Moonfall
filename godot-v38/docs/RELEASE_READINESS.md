# Moonfall release-readiness checklist

This is the implementation and validation gate for the ongoing overhaul. It does **not** certify a release. Preserve the original Z and Nyra artwork and its asset-integrity manifests.

## Immediate automated gate

From `godot-v38` with the project-compatible Godot engine installed:

```sh
python tools/run_tests.py --release-smoke
```

This imports the project, then runs ten representative gameplay suites covering startup, accessibility, performance, inventory, dungeon chambers, death/restart, boss fairness, playability, core v38 combat, and the newest AI integration gate. Failure stops release readiness. It does not replace all suites:

```sh
python tools/run_tests.py --all
```

Opt-in stress suites may be sampled with `--combat-stress`, `--enemy-stress`, `--pursuit-stress`, and `--memory-stress`. Their `-full` variants are expensive and **must not be required for every PR**. A configured number of scenarios is not evidence of passing them.

## Feature acceptance gates

| Area | Required result before calling it complete | Evidence |
| --- | --- | --- |
| Stability | Godot project imports without parse errors; scene launches and key flows survive quit/restart | Run logs and Windows smoke |
| Combat feel | Basic attack, spells, dodge, hit feedback, damage numbers, stagger and attack telegraphs remain responsive; no attacks through walls | Automated combat rules + live playtest |
| Dungeon design | Room navigation, doors, collision, encounter/reward flow and room transitions work without traps or softlocks | Chamber tests + manual room walkthrough |
| Progression | XP, level-up cards, upgrades, loot, crafting, saving and loading do not duplicate or lose items | Inventory/gear tests + save/reload playtest |
| Boss encounters | Warning readable, phase changes behave, melee/ranged counterplay works, bosses do not spawn prematurely | Boss fairness tests + full encounter playtest |
| Performance & access | No regression in FPS, allocations or input latency on target PCs; controller navigation, remapping and readable HUD verified | Profiler measurements + keyboard/controller checks |
| Windows release | Windows 10 and 11 launch, installer, optional desktop shortcut, updates, save preservation and uninstall checked | CI export, checksum, installed smoke, physical user test |

## Priorities and integration order

1. Resolve the earliest failing real Godot/Windows CI test. Fix genuine gameplay defects and update source-spelling assertions only when runtime semantics remain correct.
2. Run release smoke, then the full normal test suite. Review and integrate stacked draft PR dependencies in their parent order, rather than merging isolated tips into `main`.
3. Profile before optimizing; prefer concrete slow paths over speculative refactors. Keep enemy and effects budgets measurable.
4. Improve combat telegraphs, impact feedback, controller accessibility, and dungeon encounter pacing in small reviewable changes with tests.
5. Validate progression, bosses and save/resume end to end. Do not expand item catalogs until current mechanics and saves are reliable.
6. Build the Windows executable and installer; preserve original art checksums. Collect downloadable CI artifacts only from a passing build.
7. Verify on Windows 10 and 11, including the actual installer, desktop shortcut, gameplay, backups and accessibility. Mark features completed only with evidence.

## Evidence tracking

For each step record: PR number, commit SHA, relevant CI run link and status, exact test command, result, outstanding defects, and whether human gameplay testing occurred. Treat `pending`, `failed`, and `not run` differently from `passed`. Do not claim the game or installer is production-ready solely from static tests.

The ongoing test-suite series (including opt-in 11-million-case suites) is not yet a substitute for the above functional gates.
