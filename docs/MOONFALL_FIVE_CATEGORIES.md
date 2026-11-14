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
It assigns category IDs, actual existing owner files, and **25 proposed upgrade tracks** across five categories. The validator checks unique ownership, valid paths, milestone dependencies and cycles. Run `python tools/check_game_categories.py` and `python tools/test_game_categories.py`.

**This is a first, nonbreaking classification layer.** It does not move the
existing engine source directories, rewrite save formats, modify gameplay,
or claim full modular separation. Follow-up refactors should move one
system at a time, preserve imports and public interfaces, then verify
Godot and native UE5/Windows builds before merging.

## Larger upgrade architecture: all five domains

The registry now contains five named tracks per domain. All tracks have
`verification: not_verified`; that is a **planning and review state** and
does not mean the feature is absent, newly implemented, or passing a native
build. Existing source-file ownership is recorded separately from feature
completeness. Every major upgrade must move from design to code review and
test evidence before its verification status can change.

### Characters

1. Preserve Z and Nyra as original character identities and art.
2. Race and class creation with original Moonfall choices.
3. Equipment appearance and nine equipped slots.
4. Stats and equipment set-effect integration.
5. Character information and inspection on **C**.

### Combat

1. Attacks, weapon handling and spell casting.
2. Dodge, blocking and parry timing.
3. Mob, elite and NPC combat AI.
4. Multi-phase bosses with readable attack cues.
5. Damage indicators, VFX and hit feedback.

### World

1. A single connected world with coherent zones and biomes.
2. Dungeons and Foundry encounters integrated with combat AI.
3. Quest NPCs and exploration.
4. Discovered shrines and respawn.
5. Terrain exploration, atlas data and fog of war.

### Progression

1. XP gain and level progression.
2. Loot, equipment and inventory on **B**.
3. Rarity, RNG, gear sets and item stats.
4. Crafting and professions.
5. Level-up cards, choices and rerolls.

### Interface & Systems

1. Mouse, keyboard, controller settings and rebinding.
2. Health, mana, XP and stamina displays.
3. Menus and atlas display on **J**.
4. Save integrity, restore and checkpoints.
5. Windows builds, performance tests and accessibility checks.

## Dependency and migration rules

The 25 tracks declare `depends_on` IDs, which must exist, be acyclic and
not reference a later milestone. They are arranged as:
**foundation → integration → polish**, but implementation can proceed
incrementally in Godot and Unreal without forcing a giant merge.

Cross-category contracts matter most:
- **Characters → Combat:** playable character stats feed combat.
- **Combat → Progression:** enemy rewards generate experience and loot.
- **World → Combat:** dungeon encounter placements invoke combat AI.
- **Progression → Characters:** equipped items update hero stats/appearance.
- **World → Interface:** discovered terrain feeds map and atlas displays.
- **Interface → all:** input, HUD and saving consume existing domain APIs.

Source examples now include **Godot inventory/equipment**, **Unreal ground
loot and enemy logic**, **world NPCs**, and **Godot navigation map**.
These assignments do not physically move or rewrite those files.

**Integration gates before merging actual gameplay changes:** update ownership
and contracts, validate Godot scenes and save compatibility, compile native
UE5.4 Windows C++/UHT where relevant, verify keyboard/controller/mouse
interaction, run performance checks and test packaging. Source CI alone is
not sufficient for engine feature parity.

The registry validator has positive and negative tests for missing
categories, duplicate owners, missing/unsafe paths, duplicate roadmap
tracks, unknown dependencies, dependency cycles, out-of-order milestones,
and unsupported completion claims. Do not mark work complete based solely
on planning documents.
