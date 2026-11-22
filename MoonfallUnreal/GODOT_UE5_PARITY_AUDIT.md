# Godot v38 → Unreal Engine 5 feature-parity audit

Audited 2026-11-22 against the active Godot scene `godot-v38/scenes/main.tscn` (`native_v38_9.gd` inheritance chain) and the Unreal source under `MoonfallUnreal/`. This is a **source inspection**, not an Unreal runtime test. **A feature in an open PR is not merged into `main`**. Source implementation does not imply a native UE5 editor build or Windows playtest.

## Functional parity map

| Feature in playable Godot code | Unreal `main` source | Evidence / required UE5 follow-through |
| --- | --- | --- |
| Walk, jump, dodge, sprint, stamina, regeneration | **Source present** | `MoonfallHeroPawn.cpp` movement/combat; native UE verification needed |
| Melee combo, guard/parry, posture and telegraphed enemies | **Partial** | `MoonfallHeroPawn.cpp`, `MoonfallEnemy.cpp`; UE5 animation/VFX/gameplay validation pending, improvements in draft #1087 |
| Aimed hostile spell and directional lock-on | **Missing on main; added in draft #1088** | Godot `native_v38_7.gd`; Unreal draft adds T/middle-click, context D-pad Up, raycast-safe spell targeting |
| Independent left/right spell selection and dual-cast | **Partial** | `MoonfallHeroPawn.cpp` and `MoonfallPersistentSave.h`; MB5, no Z/X combat shortcuts in draft #1088 |
| HP, mana, stamina, XP HUD | **Source present** | `MoonfallCombatHUD.cpp`; require resolution and controller playtests |
| Character creation and equipment stats | **Partial** | `MoonfallCharacterCreationWidget.cpp`, hero stats; rules/character customization not equivalent |
| Large classless constellation/talent web and 2 points per level | **Missing** | Godot `moonfall_talent_web.gd`, `native_v38_0.gd` and talent runtime; no equivalent UE5 talent allocation/save/runtime UI in `main` |
| Nine equipped gear slots, deep item/vault management | **Partial** | Godot equipment inventory/catalog; Unreal hero and save currently declare **three equipped codes**; stacked #844+ PRs are not merged |
| Random loot, rarity, affixes, auto-pickup | **Partial** | `MoonfallGroundLoot.cpp` and hero; compare roll tables, ownership, capacity and drop feedback in native gameplay |
| Forging, equipment upgrades, reforge, crafting | **Partial** | `MoonfallHeroPawn.cpp` implements salvage forging; Godot materials/recipes and station functionality require reconciliation |
| Three-denomination money, shops, trading | **Missing/partial** | Godot `money_rules.gd`, shop UI; Unreal uses salvage but no equivalent gold/silver/copper wallet in hero/save |
| Six-region living overworld, roads, landmarks, AI ecology | **Partial** | Godot `native_v11.gd`, `living_world_v363.gd`; Unreal arena and 30 dungeon foundations exist; streamed provinces in draft #1084 |
| Dungeon objectives, shrines and checkpoint respawn | **Partial** | `MoonfallDungeonLayout.cpp`, `MoonfallCheckpointShrine.cpp`, `MoonfallPersistentSave.h`; native world traversal and load tests pending |
| Fog-of-war atlas and discovered shrines | **Source present** | `MoonfallCombatHUD.cpp`, hero atlas masks/hidden locations; real terrain/art parity unverified |
| NPC interaction, quests and reputation | **Partial** | `MoonfallWorldNPC.cpp`, hero quest/faction/bond records; Godot morality state and broader contract systems not fully present |
| Seven tracked quests with region objectives | **Missing on main** | Draft #1071 has UE5 quest-tracker work; not included in current UE5 release |
| Day/night, changing weather, world cycles | **Missing** | Godot `world_cycle.gd`, `native_v30_3.gd`; no comparable UE5 hero/world runtime subsystem confirmed |
| Swimming, oxygen, deep-water movement and water professions | **Missing** | Godot `oxygen_v372.gd`, `deep_water_v373.gd`, `water_professions_v362.gd`; UE5 water actors, movement, and warnings need real integration |
| Mounts, world travel and walkable ferry | **Missing/partial** | Godot mount/world-travel systems; UE5 gothic ferry is draft #1077, not part of `main` |
| Original Z and Nyra visuals | **Not ported/verified** | Preserve original asset bytes and character appearance exactly; never approximate, generate over, or silently replace |
| Accessibility: mouse/controller navigation, visual warnings, reduced motion, 4K | **Partial** | UE5 Options/HUD/mouse inputs and reduced motion are source-implemented; full controller/assistive Windows testing pending |
| Windows downloadable game and installer | **Unreal build unverified** | UE5 requires trusted Windows Unreal 5.4 native compile + packaging; successful Godot CI is not UE5 proof |

**Status meaning:** Source present = relevant code exists, not necessarily runtime-tested. Partial = overlapping mechanics but important mismatches/untested integration. Missing = no corresponding implemented UE5 runtime path found in inspected sources. A draft PR is explicitly **not** a released feature.

## Migration order and completion gates

1. **Native UE5 build baseline:** compile editor C++, package Win64, launch clean Windows 10/11 install and record logs before marking features playable.
2. **Core controls/combat:** finish draft #1088 (MB5 dual-cast and target lock), native-test mouse/controller, then integrate validated defense/animation from #1087 without input conflicts. T toggles enemy lock; racial power uses N; controller D-pad Up prioritizes nearby NPCs.
3. **Progression parity:** implement and test Godot's classless talent web with two points/level, validated respec and save migration, then migrate nine gear slots and vault without damaging existing three-slot saves.
4. **Real world traversal:** unify six zones and overworld, then swimming/oxygen, mounts/ferry, day/night/weather, and performance-scaled AI in deliberate subsystems.
5. **Quests/economy:** integrate the seven-quest tracker, real gold/silver/copper, vendors, crafting recipes and morality decisions, all with save compatibility.
6. **Artwork/presentation:** import only approved original Z/Nyra assets **unchanged**, verify bosses/portraits, add production animation/VFX and accessible UI without changing character designs.

## Non-negotiable acceptance checks

- **No blanket merge:** audit dependencies, overlapping files, base refs, tests and native Windows compilation individually.
- **No double-bound action:** MB5 is dual-cast only, T/middle-click lock-on; existing left/right mouse, wheel, controller defense, inventory and NPC interaction remain usable.
- **No false parity:** code on draft branches, passing source grep/CI, and actual native runtime are three separate statuses.
- **No changed original art:** Z and Nyra's original files and appearances must stay identical.
- **No fragile saves:** checkpoint, gear, world masks and talent progression require backward-compatible load and corruption safeguards.

Reference entry points: `godot-v38/scripts/native_v38_9.gd`, `godot-v38/scripts/native_v38_7.gd`, `godot-v38/scripts/native_v38_0.gd`, `MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp`, `MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.h`, `MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp`, and `MoonfallUnreal/Source/MoonfallUnreal/MoonfallPersistentSave.h`.
