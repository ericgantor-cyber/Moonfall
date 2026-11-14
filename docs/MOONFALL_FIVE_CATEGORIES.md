# Moonfall: five game categories

Moonfall remains **one connected action RPG**, not five separate games, modes,
repositories, or unrelated worlds. The five categories below are the shared
feature and code-ownership structure for the Godot game and the Unreal Engine 5
migration. Features may depend on one another, but each should have **one
primary owning category**.

| Category | Owns | Player-facing examples |
| --- | --- | --- |
| **1. Characters** | Heroes, playable races, character details, identity and appearance | Z and Nyra, race selection, character info (C), equipment appearance |
| **2. Combat** | Attacks, spells, defensive actions, enemies and bosses | Melee, dodge, parry, stamina consumption, boss phases |
| **3. World** | Connected overworld, zones, biomes, dungeons, exploration and quests | Mech Foundry, caves, fog-of-war map, shrines and quest locations |
| **4. Progression** | Rewards, XP, leveling, loot, gear, crafting and permanent improvements | XP, automatic stat growth, inventory (B), weapons and gear sets |
| **5. Interface & Systems** | HUD, UI, input, accessibility, saves, performance and builds | HUD orbs, atlas (J), controller, mouse, Windows release pipeline |

Moonfall is **classless**, and leveling does **not** present upgrade cards or random reroll choices. Progression uses earned XP, automatic base-stat growth, **earned points spent on a connected talent tree**, equipment, loot and crafting. This policy applies to new work in both engines; existing gameplay implementations require separate audits and verified migrations.

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
2. Race selection with original Moonfall character designs.
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
5. Automatic base-stat growth plus earned talent points spent on an open skill tree, without upgrade cards.

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


## Executable dependency-ordered work plan

Use the new tool to get a concrete, topologically sorted development sequence
instead of manually picking arbitrary upgrades:

```powershell
python tools/moonfall_category_plan.py
python tools/moonfall_category_plan.py --category combat
python tools/moonfall_category_plan.py --category interface --format json
python tools/moonfall_category_plan.py --output moonfall-plan.md
```

A filtered plan includes cross-category prerequisites (marked
**Prerequisite**), even when they live outside the requested category.
For example, the interface's atlas UI depends on world-map discovery and
inventory data. The tool schedules these requirements ahead of the menu
and never calls planned features implemented or tested.

The generator uses the validated registry, stable milestone ordering and
catalog tie-breaks. It does not change game files or saves. CI checks the
full JSON plan and a filtered interface plan, plus positive and negative
regression tests. Actual UE5.4 native compilation, Godot smoke testing,
playable controls, and merge readiness remain separate requirements.


## Original classless character stats: Enshrouded + Valhalla references

Moonfall uses the **design principles**, not copied game assets or identical
balance numbers:

- **Enshrouded reference:** six attributes linked to melee, ranged, magic,
  health, mana and stamina.
- **Assassin's Creed Valhalla reference:** earned power, gear improvement and
  open combat playstyles, without locking a character into a class.
- **Moonfall implementation direction:** automatic growth at every level.
  No classes, upgrade-card drafts or rerolls. **Talent points are player-selected** in an open tree. Hero identity and original
  Z/Nyra artwork are preserved.

The new versioned `config/moonfall_stats.json` establishes original Moonfall
stats: **Might, Agility, Arcana, Vitality, Resolve and Endurance**. These feed
melee/ranged/magic damage and health/mana/stamina. Level growth is automatic
and deterministic. Equipment bonuses apply separately, while overall Power
is calculated from level and gear score. All coefficients are provisional
balancing values, **not yet gameplay integration**. Talents provide **separate, player-selected** bonuses on top of these automatic values.

Run `python tools/moonfall_stats.py` for a sample character and
`python tools/test_moonfall_stats.py` to validate automatic progression,
resource values, gear bonuses and invalid inputs. The calculation is a
side-effect-free Python reference model, not a C++/GDScript implementation;
it does not touch saves or existing equipment. Integrating the model with
Godot and Unreal requires native behavior tests, HUD verification and
save migration review. Keep the UI character panel bound to the game's
actual stats until integration is explicitly complete.

## Talent tree correction: inspired by Enshrouded and AC Valhalla

The intended reference is the **branching talent/skill-tree design**,
not replacing talent decisions with automatic stats. Use an original Moonfall
tree with three interconnected themes: **Might, Agility and Arcana**. All
branches are accessible to every hero. There are **no character classes,
upgrade cards, random draft screens or rerolls**. Every level provides one
talent point to spend deliberately on a node, subject to level gates,
prerequisites and the available point budget. Automatic baseline stats and
separate gear bonuses still apply.

The versioned data file `config/moonfall_talents.json` defines a first
**12-node prototype**. The pure Python reference calculator in
`tools/moonfall_talents.py` validates branches, prerequisites, levels, ranks,
point budgets and bonuses; `spend` allocates a point, while `respec`
refunds the allocations. Run `python tools/test_moonfall_talents.py`
for regressions. These are original Moonfall node names and balance
values, not copied artwork or a claim that either engine has a working
talent UI. Live integration requires GDScript/UE5 code, persistence
migration and gameplay/Windows verification.
