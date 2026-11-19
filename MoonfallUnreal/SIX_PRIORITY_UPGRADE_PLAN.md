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

## Current cross-system hardening stream: draft PR #1051

This section reports **changes committed to a draft integration branch**, not changes
already merged into `main` or proved to work in a native Unreal build.
PR: https://github.com/ericgantor-cyber/Moonfall/pull/1051

The work is organized against the five Moonfall categories, with deliberately
separate source-level and engine-level claims:

| Category | Implemented in draft branch | Still needs native/runtime proof |
| --- | --- | --- |
| World | Bounded retry attempts for failed generated dungeon layouts; retries separated across timer ticks; no permanent slot consumption by destroying authored layouts | Real UE5.4 construction failure injection, all 30 distinct layouts, road/sanctuary navigation, streaming and startup frame times |
| Combat | Rejects invalid/dead boss and mob spawns, atomically rolls back damaged combat/bonus groups after Blueprint callbacks, and guards against synchronous overlap re-entry | Combo/parry/dodge feel, boss callback destruction, collision-blocked 5–6 and 5–11 mob formations, native AI and GPU profiling |
| Progression | Rechecks pending dungeon treasure; indexes persisted loot by GUID; adds weighted pools and a mouse-editable CSV/DataTable authoring path with guarded fallback and theoretical odds preview | UE5.4 actual DataTable import, runtime drop distributions, 512-drop stress, 30-dungeon reward recovery, all-level tuning |
| Characters | Guards NPC conversations and quest actions against invalid vitality/player ownership and conflicting Atlas/character overlays, while permitting intentional actions inside resident context | Mouse-only, keyboard and controller conversations; quests, reputation, relationship persistence; original Z/Nyra visual inspection |
| Interface & systems | Native dungeon test source now expects V12 and covers map-fog masks, 420 treasure states and pending loot; source tests cover updated safety contracts | Compiled Editor automation, packaged Windows builds and installer, readable 4K HUD/menu focus, accessible inputs |

### Weighted loot tables: rank, region and dungeon treasure

The Unreal hero now exposes an editor-configurable `LootTableEntries` array
(`FMoonfallLootTableEntry`) containing **Source**, **ZoneIndex** (-1 generic or
0–5 regional), **MinimumLevel**, **Rarity** (0–5), **Weight** (1–10000),
**SlotMask** (weapon/armor/accessory bits) and **ForcedTrait** (0 random or
Force/Ward/Focus). C++ constructs usable defaults, so an external asset is not
required to start. Authors can tune a Hero Blueprint's class defaults.

| Pool | Default weighted candidates | Guarantee |
| --- | --- | --- |
| Regular | Common 44, Uncommon 34, Rare 18, Epic 4; Emerald +2 at level 40, Legendary +1 at level 60 | Valid equipment ID, no forced rare drop |
| Elite | Rare 70, Epic 25; Emerald +5 at level 20, Legendary +1 at level 50 | Rare or better |
| Boss | Epic 75; Emerald +20 at level 30, Legendary +5 at level 60 | Epic or better |
| Dungeon treasure | Rare 60, Epic 30; Emerald +8 at level 20, Legendary +2 at level 50 | Rare or better |

Every one of the six dungeon regions adds its own weight-8 thematic row to
each source pool, selecting a region-specific combination of equipment slot
and legacy Force/Ward/Focus trait. These are **relative weights**, not fixed
percentages: level gates and regional entries change each pool's denominator.
World enemies without a dungeon owner use the generic rank pool; dungeon
mobs/bosses use their spawning dungeon's exact region.

Dungeon treasure uses `FRandomStream` with the existing saved room seed. A
previously created or collected reward is returned from the V12 ledger
**before any new roll**, so editing tables never rerolls existing rewards.
Enemy kills still create ground pickups and write persistence before
collection; the item code format and original artwork are unchanged.

### Loot authoring and editable probability preview

This draft also adds a real, mouse-editable Unreal **Data Table** pathway
without changing the item/save schema. The row struct derives from
`FTableRowBase`; a Hero Blueprint can assign `LootDataTable`, which imports
rows **once at BeginPlay**, sorted by row name so seeded dungeon treasure
is stable. An explicit Blueprint reload is supported. Bad/missing tables,
invalid ranks and empty level-one generic pools preserve built-in loot.
The import performs one-time snapshot allocation, not a new per-enemy scan.

The import-ready `MoonfallUnreal/Data/Moonfall_DefaultLootTable.csv` has
41 rows for regular/elite/boss/treasure and six regions. The offline,
mouse-and-keyboard accessible `Moonfall_LootEditor.html` loads those defaults,
accepts CSV, edits rows, previews percentages and exports a new CSV locally.
`tools/moonfall_loot_table.py` independently validates edited CSVs and
prints weight-based chances for chosen source, region and level. Blueprint
functions `GetLootPoolTotalWeight` and `GetLootRarityChancePercent` expose
the same selection weights without consuming RNG. See
`MoonfallUnreal/Data/LOOT_TABLE_EDITOR.md` for setup.

**Not yet implemented:** an *in-game* loot-table editor inside the playable
Moonfall UI; individually named/unique boss item IDs, material drop stacks
or a persistent Luck/pity stat. These require additional gameplay/UI design
and possibly item/save migration. The offline editor and CSV syntax tests
are not proof of actual UE5.4 compilation, imported asset cooking, live
drop frequencies, or packaged Windows behavior.

### Release checks (do these in order)

1. **Source gate:** Record the exact PR head SHA. All repository source tests,
   equipment/loot safety tests, racial/balance tests and Windows prerequisite
   checks must pass on that same SHA, not on a previous commit.
2. **Native gate:** On an authorized isolated Windows computer with UE5.4 and
   Visual Studio C++ tools, compile the Editor target, then run
   `MoonfallUnreal/Build/Test-DungeonNative.ps1`. Keep the complete Editor
   log and per-test PASS evidence. The normal pull-request Windows workflow
   **skips** the self-hosted native compile.
3. **Dungeon smoke gate:** Start in the sanctuary, enter all six regions,
   confirm 30 unique layouts, blocked-slot retry, boss phase/kill save, collision
   fallback, room re-entry, three bounded bonus packs, no duplicate loot,
   and no frame spikes during staged spawning. Include Blueprint callbacks
   that remove enemies, invalidate health, or re-enter overlaps.
4. **Persistence gate:** Test V12 saves across quit/relaunch/respawn including
   56-bit-per-zone Atlas fog, 420 treasure states, world drops and boss victories.
   Verify an actual historical V9 file with a backup; the synthetic memory
   fixture is **not** equivalent to a real historical save migration.
5. **Interaction/performance gate:** Exercise C/B/J/Escape, right-click
   equipment, NPC quest context, mouse-only and controller focus at 1080p
   and 4K. Benchmark item restoration at low and high drop counts and
   collect CPU/GPU frame-time percentiles on Windows 10/11.
6. **Packaging gate:** Build and install the actual Win64 game with NSIS,
   launch via shortcut, verify save location and uninstall, then review PR
   conflicts/dependencies before considering any merge.

Do not mark **UE compilation**, actual in-game behavior, stable 4K FPS,
or an installable Windows `.exe` as done merely because source CI is green.
No Z/Nyra artwork, Godot assets, save schema or engine priority is altered by
this source-level reliability work.

## Merge rules

Do not overwrite the original Z/Nyra art. Never merge stale PRs wholesale when their changes overlap with `main`. Check the exact PR head SHA, compile/source checks, conflict state and regression results before merging; explicitly report native-test gaps. Do not claim native UE5 build success from a Windows prerequisite or source-structure check.
