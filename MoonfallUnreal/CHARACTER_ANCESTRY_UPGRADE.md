# UE5 character sheet and racial passives

## New C-page layout
- **C** opens the normal character stats panel from gameplay. Press **Tab** to switch between **Attributes** and **Ancestry & Passives**.
- Controller **D-pad Left/Right** selects the corresponding page while the character sheet is visible. These inputs continue their existing roles everywhere else.
- Mouse users can click the header **RACE & PASSIVES** / **ATTRIBUTES** tab.
- On the ancestry page, the hero's current **saved race**, base Might/Insight/Resolve bonuses, stamina/mana regeneration bonuses, racial power and its readiness/cooldown/active duration are read live. The page is strictly read-only.
- The character creation widget now compares vitality/power/speed profiles to baseline by percentage. These are **previews**, not newly enabled gameplay passives. Preview art/rigging is unchanged.

## The actual saved gameplay ancestries (stable 0-11 IDs)
| ID | Ancestry | Active racial power | Cooldown |
| --- | --- | --- | --- |
| 0 | Human | Recover 15% mana and stamina | 45s |
| 1 | High Elf | Recover 30% mana; +15% offensive spell damage for 8s | 45s |
| 2 | Wood Elf | Recover 35% stamina; 12s power window | 45s |
| 3 | Dwarf | Recover 10% stamina; 15% damage reduction for 10s | 45s |
| 4 | Halfling | Recover 45% stamina; 6s power window | 45s |
| 5 | Orc | Recover 8% health and 20% stamina | 45s |
| 6 | Goblin | Recover 22% mana and stamina | 45s |
| 7 | Troll | Recover 12% health; 15% damage reduction for 8s | 45s |
| 8 | Entkin | Recover 8% health; 15% damage reduction for 14s | 45s |
| 9 | Shadowborn | Recover 22% stamina; +15% offensive spell damage for 8s | 45s |
| 10 | Moonborn | Recover 38% mana; +15% offensive spell damage for 8s | 45s |
| 11 | Draconid | Recover 7% health and 18% mana | 45s |

Passive attribute/regen values are **not reimplemented**: the ancestry page queries the hero's canonical race profile and existing gameplay getters, so changes to the runtime stats remain consistent with what C displays.

## Known limitation
The separate 30-entry character-creator race browser is **not** a one-to-one mapping to the 12 saved hero gameplay ancestry IDs. This PR improves its preview text but does **not** silently remap, grant new racial abilities, or persist invalid IDs. The 12 saved abilities and effects remain the existing runtime behavior. Unifying both rosters and performing a full original-art/UE5 native Windows creator playthrough require explicit gameplay integration and validation.

This source change does not alter any save schema, gameplay race IDs, Z or Nyra original art, world map, or the existing WASD/arrow-only movement controls. GitHub source tests verify contracts but do not replace Windows UE5 Editor/UnrealBuildTool compilation.
