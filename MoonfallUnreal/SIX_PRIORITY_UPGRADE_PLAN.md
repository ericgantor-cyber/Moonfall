# Moonfall six-priority upgrade plan (audited 2026-11-17)

This plan is based on a source inspection of the current `main` Unreal prototype, its build scripts and regression contracts. **Source code presence is not proof of successful Unreal compilation or playable feature parity.** Keep the original Z and Nyra art unchanged and preserve the Godot project during migration.

## Order of work and proof required

| Priority | Domain | Present in inspected source | Next milestones | Acceptance gate |
| --- | --- | --- | --- | --- |
| 1 | Windows builds and performance | UE 5.4 project, source regressions, PowerShell preflight, map generator, compile/package and NSIS scripts, optional launch smoke test; native Unreal toolchain on public CI not established | Keep Build Center responsive with long compiler logs; provision authorized UE 5.4 runner; run actual Editor/Game Win64 compile, native dungeon tests, cook, packaged launch and Windows 10/11 installer checks; profile AI/render cost before removing performance guards | Real engine compile + packaged startup evidence for exact commit, attributable logs, installer checksum, game launch, profiler measurements |
| 2 | Combat and boss encounters | Existing hero combat input/resources, enemy attack windups, chamber-local ally coordination, bosses and combat HUD | Test combo cancels, parry/dodge windows, tell readability, shield stamina, boss phases, animation continuity and gamepad/mouse controls; prioritize correctness over visual effects | Native UE automation where feasible, manual controller/mouse combat matrix, no infinite or double-hit warnings |
| 3 | Dungeons and enemy AI | 30 seeded dungeon placements across six-region world, 14-chamber layout, seeded 5–6 regular mobs per combat room, solo elites, 25% optional three 5–11 bonus packs, retries, and atomic normal/bonus spawns | Stress-test room entry/re-entry, interrupted spawns, passage collisions, spawn performance across 30 dungeons and boss persistence; improve room variety without a fixed global 60/69–111 enemy cap | Deterministic spawn tests, failure-rollback tests, native collision test, bounded frame time and memory |
| 4 | Six-zone world | Existing six-region arena/world foundation and starting sanctuary hooks | Verify transitions, location discoverability, persistent checkpoints, quests, settlements, distinct biomes and regional NPC behavior; replace placeholder environmental assets only with original approved artwork | Continuous exploration QA, resume-from-save checks, zone transitions and streaming/profiling evidence |
| 5 | Talent progression and loot | Hero XP/level, equipment, inventory, crafting and save logic are present; a complete large talent web was **not** established in inspected Unreal hero files | Design a persistent branching talent graph with two points per level; build tooltips, respec rules, rank validation, equipment/set interactions and save migration | Tests for points spent/refunded, legal dependencies, persistence and mouse/controller traversal |
| 6 | UI and accessibility | Combat HUD, atlas, inventory, input actions and Windows Build Center are present | Consistent mouse-first inventory, C character, B inventory, J atlas, right-click equipment; readable menus, adjustable scaling, controller focus and full keyboard remapping; avoid single-switch game mode | Keyboard/mouse/controller accessibility audit, visual contrast/4K checks, no trapped focus |

## Immediate PR scope

**First fix:** eliminate whole-file log rereads in `MoonfallUnreal/Build/Build-Center.ps1`, which previously occurred every 350 ms while UE build logs grew. Instead, read only new appended bytes in bounded chunks with stateful UTF-8 decoding. Drain remaining bytes after process exit, reset state on truncation, and keep the UI textbox bounded while preserving full diagnostic log files on disk.

**What this verifies:** source regression assertions and Windows PowerShell parser/UI checks (when GitHub Actions runs them). **What it does not verify:** an actual Unreal 5.4 Editor/Game C++ compile, performance on the user's PC, a produced installer, or in-game behavior.

## Merge rules

Do not overwrite the original Z/Nyra art. Never merge stale PRs wholesale when their changes overlap with `main`. Check the exact PR head SHA, compile/source checks, conflict state and regression results before merging; explicitly report native-test gaps. Do not claim native UE5 build success from a Windows prerequisite or source-structure check.
