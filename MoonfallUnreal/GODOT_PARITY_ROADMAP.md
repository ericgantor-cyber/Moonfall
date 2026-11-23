# Bring Godot Moonfall gameplay into Unreal Engine 5

Updated 2026-11-23. **Godot remains the production reference**. This roadmap
covers the separate `MoonfallUnreal` UE5.4 C++ project; another experimental
`engine-comparison/unreal/` prototype is not the production migration.

**Do not merge or replace Godot** until equivalent systems are independently
verified in an actual UE5 Windows build and exercised by players using mouse,
keyboard and controller. Never recreate or modify the original Z and Nyra art.

| Godot feature | Unreal source position | Next acceptance gate |
|---|---|---|
| 141-node classless talent constellation (2 points per level; Might, Agility, Arcana) | #1121 exact catalog and V13 saving; #1122 live Arcana damage; #1123 live resource caps; **this stacked PR:** native Canvas three-branch constellation with 141 entries, shared mouse/pad hit tests, tier panning, point spending and confirmed full respec | Still needs melee/ranged damage wiring, native UE Editor compilation, actual renderer review, Windows controller playfeel and physical save round-trip |
| Six connected regions, terrain, dungeons and streaming | UE world/dungeon/arena C++ scaffolds and navigation invoker exist | Traverse all six zones and dungeon entrances; compare boundaries, collisions, weather and LOD in packaged build |
| Wildlife, ecology, roaming foes and bosses | UE enemy AI, navigation, elites and guardian rules exist | Species-by-species behavior and encounter parity; benchmark dense mobs; test line of sight and attack marker accuracy |
| Live Raven Atlas, map and talent panels | UE inventory/Atlas/Options inputs and drawing exist | Ensure world clock, enemy AI, movement and incoming damage remain live when information panels open, without trapping user input |
| Shrine discovery, saved checkpoints and respawn | UE checkpoint and persistent save logic exists | Compare discovered shrines, safe teleports, death and resurrection paths; never erase progress |
| Phoenix Feather in-place revival | No verified UE equivalent | Item ownership, single-use lethal-save trigger, brief grace, queued melee cleanup, save compatibility and UI feedback |
| Combat reactions, stamina, dodge/parry, boss tells | UE hit reactions, posture, defense and boss/tell rules exist | Frame-accurate cooldown and telegraph tests against active Godot; real Windows input/animation tests |
| Loot, nine gear slots, rarity and 3D item art | UE gear/inventory, 3 equip slots and ground loot exist | Expand to approved nine slots only after save migration, UI and item/stat tests; no artwork changes without approval |
| Quests, NPC interactions, factions, professions | UE NPC, quest, relationship and reputation code exists | Test quest states, save round-trip, rewards, crafting and live UI flows |
| Camera, atlas fog, minimap, accessibility | UE camera, inventory, atlas/chronicle and settings exist | Mouse-wheel/MB5 action semantics, keybinding matrix, scaling, controller focus and legible warnings |
| 4K, native Windows 10/11 builds and launcher | UE graphics, scripted packaging and launchers exist | **Native Unreal Editor build, cooked Win64 package, installer and manual hardware testing**; source checks are not a game build |

## Native classless constellation interaction (new stacked draft)

The in-game talent web uses Moonfall's existing native `AMoonfallCombatHUD`
Canvas and exact original 141-node data. It draws four rows of nodes per
tier, with a bounded 3–5-tier horizontal window, known prerequisite edges,
three distinct talent tabs, current rank/maximum, minimum level and points
remaining. It does **not** allocate 141 Actor components or alter hero art.

- **K** opens/closes; **Esc** or **X** closes.
- **Arrow keys / D-pad** move among nodes and tiers.
- **Q/E** or gamepad **B/R3** cycle Might, Agility and Arcana.
- **Mouse wheel** pans along the tier progression. Left-click selects a
  node, then **SPEND POINT**, **Enter**, or gamepad **A** spends a point.
- **RESET ALL** or **Backspace / gamepad LT** requires a second confirmation
  within three seconds. Respec uses the safe save/cap-reconciliation code.
- Gamepad **D-pad Up** opens this web only if no valid nearby NPC can
  accept a context interaction. It continues to navigate nodes while open.
- Combat simulation and the world clock stay active behind the panel.
  Player movement, spell casting, dodging and autoloot cannot accidentally
  execute while the web owns input. The existing recovery-in-menus logic
  also applies to the talent web.

Actual UE5 Win64 compilation, native Canvas rendering and gamepad focus
testing are **not yet verified**. Portable C++17 tests validate deterministic
web geometry, hit testing, tier limits and branch navigation; the Unreal
source workflow asserts correct input routing, avoiding the shared
controller-A character-sheet collision.

## Scope of this first talent slice

Godot's canonical source is `godot-v38/data/moonfall_talents.json` with
141 exact nodes and 47 connected nodes per branch. The UE5 header
`Source/MoonfallUnreal/MoonfallTalentRules.h` is deliberately plain C++17
so its rules can be compiled and tested without the multi-gigabyte Unreal SDK.

- `Evaluate`: enforces level 1–100, points, rank caps and all prerequisites.
- `Spend`: fails atomically when a new rank is invalid or unaffordable.
- `Respec`: full-tree refund only; corrupt existing ranks are rejected.
- Talent points are derived from character level: `(level - 1) * 2`.
- `AMoonfallHeroPawn` exposes Blueprint spend, respec, rank, available
  points and per-stat bonus queries.
- Save schema V13 stores all 141 ranks or an empty, unspent array. V1–V12
  saves are read as unspent without lowering level or erasing other progress.
  Invalid new saves are rejected *before* the checkpoint teleport.

**Not yet implemented by this PR:** drawing and navigating the complete
constellation widget; connecting every calculated stat to live combat; Unreal
engine compilation; verifying UE5 saves on disk; playable Windows 10/11 build.
Avoid describing the catalog alone as the entire finished talent system.

## Testing

`tools/test_unreal_talent_parity.py` compares the compiled 141-node catalog
against every Godot JSON entry, and compiles/runs
`tools/test_unreal_talent_rules.cpp` with C++17. GitHub's
`Unreal source structure checks` workflow runs it and the existing source
regressions. **These are not native UE editor or packaging tests**.

Before promoting this PR, compile the actual `MoonfallUnreal` C++ target
with UE 5.4 and Visual Studio on Windows, exercise talents against saves
from older versions, then test the installer and input accessibility.
