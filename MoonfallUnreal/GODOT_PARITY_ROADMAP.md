# Bring Godot Moonfall gameplay into Unreal Engine 5

Updated 2026-11-23. **Godot remains the production reference**. This roadmap
covers the separate `MoonfallUnreal` UE5.4 C++ project; another experimental
`engine-comparison/unreal/` prototype is not the production migration.

**Do not merge or replace Godot** until equivalent systems are independently
verified in an actual UE5 Windows build and exercised by players using mouse,
keyboard and controller. Never recreate or modify the original Z and Nyra art.

| Godot feature | Unreal source position | Next acceptance gate |
|---|---|---|
| 141-node classless talent constellation (2 points per level; Might, Agility, Arcana) | **This draft PR:** exact catalog, rank/prerequisite/point calculator, Blueprint spend/respec/stat queries, V13 save fields | Wire Godot's bonus effects into live health/damage/resource calculations; build mouse/controller-accessible pan/zoom web; UE editor compile and save round-trip |
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
