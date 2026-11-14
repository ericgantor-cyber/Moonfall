# Moonfall: five game categories

Moonfall remains **one connected action RPG**, not five separate games, modes,
repositories, or unrelated worlds. The five categories below are the shared
feature and code-ownership structure for the Godot game and the Unreal Engine 5
migration. Features may depend on one another, but each should have **one
primary owning category**.

| Category | Owns | Player-facing examples |
| --- | --- | --- |
| **1. Characters** | Heroes, races, classes, character details, identity and appearance | Z and Nyra, race selection, character info (C), equipment appearance |
| **2. Combat** | Attacks, spells, defensive actions, enemies and bosses | Melee, dodge, parry, stamina consumption, boss phases |
| **3. World** | Connected overworld, zones, biomes, dungeons, exploration and quests | Mech Foundry, caves, fog-of-war map, shrines and quest locations |
| **4. Progression** | Rewards, XP, leveling, loot, gear, crafting and permanent improvements | Upgrade cards, XP, inventory (B), weapons, sets and upgrades |
| **5. Interface & Systems** | HUD, UI, input, accessibility, saves, performance and builds | HUD orbs, atlas (J), controller, mouse, Windows release pipeline |

## Practical boundaries

- **Characters** defines who the hero is. **Progression** owns the inventory
  data and rewards. Equipping a reward integrates both systems but should not
  duplicate the same inventory model.
- **Combat** owns hit resolution and enemy actions. **World** places enemies and
  manages arenas; dungeon geography is not combat code.
- **World** owns map discovery and travel locations. **Interface & Systems**
  owns how the minimap and atlas are shown and how controls are bound.
- **Interface & Systems** owns loading/saving and performance instrumentation.
  Other categories expose serializable state but do not write incompatible
  competing save files.
- **Original Z and Nyra artwork remains unchanged.** This reorganization
  does not import, edit, rename or replace game art.
- Both Godot and Unreal implementations can be mapped to the same five
  categories. **Do not delete or replace Godot** until Unreal feature parity
  is built and tested.

## Implementation status

The machine-readable registry is `config/moonfall_game_categories.json`.
It assigns category IDs and sample existing owner files where known, and
`tools/check_game_categories.py` enforces five distinct categories,
unique ownership, valid paths and required features. The validator can be
run with `python tools/check_game_categories.py`.

**This is a first, nonbreaking classification layer.** It does not move the
existing engine source directories, rewrite save formats, modify gameplay,
or claim full modular separation. Follow-up refactors should move one
system at a time, preserve imports and public interfaces, then verify
Godot and native UE5/Windows builds before merging.
