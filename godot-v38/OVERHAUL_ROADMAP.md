# Moonfall integrated overhaul roadmap

This is a staged implementation plan, not a claim that every feature is completed. Keep Moonfall one connected world, preserve the original Z and Nyra art assets exactly, and do not add single-switch controls to gameplay.

## Release gates
- Every change must pass Godot headless scene loading, relevant gameplay regression suites, and Windows build/installer CI.
- Do not call a Windows release playable until it has been manually tested on Windows 10 and 11 with mouse/keyboard and controller.
- Protect player saves with versioned backup and rollback before any save migration; never delete saves during repair.
- Measure frame time, enemy counts, and memory before/after each optimization. Avoid visual-feature bloat and per-frame full-list scans.

## Integrated workstreams
| Workstream | Next deliverable | Validation |
| --- | --- | --- |
| Combat | Consistent target selection and visible hits, readable damage numbers | Target-cycle and attack/spell gameplay tests |
| Enemy AI | Profile pursuit/search behavior; bounded update budgets and fair boss attack telegraphs | AI tests at dense encounters |
| Connected world | Keep one world; stream/reuse region props and add navigable transitions | Atlas/nav and world-transition suites |
| Progression | Audit XP, loot drops, crafting and profession economy for overflow/exploits | Save/loot/progression tests |
| Performance | Profile target loops, markers, enemy update and UI allocation | 60-enemy baseline and stress measurement |
| Controls and HUD | Maintain M map, Tab target-cycle, right-stick target lock, 0 camera reset; readable health/mana/XP | Input tests with controller and mouse |
| Windows delivery | Finish portable ZIP, verified installer and recovery instructions | CI artifacts, checksums, hands-on Win10/11 |

## Current batch
- Target feedback uses indexed lock check instead of full-list lookup (committed earlier).
- Target cycling uses indexed collection traversal, avoiding repeated full-list lookups for candidates (committed, CI pending).
- Overhaul foundation has targeted source checks for both changes (committed, CI pending).

## Acceptance for a finished overhaul
A feature is only complete when gameplay is working, automated tests pass, performance impact is checked, and a Windows release has been built and exercised. Until then label work as planned, implemented but unverified, or verified in CI.
