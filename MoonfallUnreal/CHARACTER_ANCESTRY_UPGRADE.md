# UE5 character sheet and racial passives

## New C-page layout
- **C** opens the combined **Stats & Gear** character sheet from gameplay. C again opens **Ancestry & Passives**, then **Reputation**; further presses cycle these three pages. **Tab** remains Atlas Chronicle, never a character-page shortcut.
- Controller **D-pad Left/Right** cycles backward/forward through the three pages while the character sheet is visible. These inputs continue their existing roles everywhere else.
- Mouse users can click separate **STATS & GEAR**, **ANCESTRY & PASSIVES**, and **REPUTATION** tabs below the level line. The selected tab is highlighted and clicking it again leaves the page unchanged.
- **Reputation** displays the exact five saved world faction standings: Wanderers, Verdant Circle, Stone Hearth, Moon Coven, and Ashen Clans. Each shows a live -100 to +100 score, NPC-aligned Hostile/Wary/Neutral/Friendly/Trusted rank, next rank milestone, and a WoW-style reputation bar with a neutral midpoint. This tab never grants reputation or mutates saves; old saves with no faction key show 0 (Neutral). Hostile standings can prevent receiving/turning in resident quests.
- **Stats & Gear** is one read-only page. Primary and secondary stats, legendary perks, Force/Ward/Focus set progress, equipment power and dungeon campaign sit above three quality-colored cards for real **Weapon**, **Armor**, and **Accessory** equipment. Each card uses an owned-only validated tier, rarity, trait, and item power; tall layouts also show live Health/Mana/Stamina mini-meters. Compact windows prioritize essential stats and the equipped slot summary. Invalid or unowned items never appear as equipped, and no imaginary gear slots or paper-doll assets are created.
- All character pages are **read-only**, and page state is transient (not persisted). Gear continues to be equipped only through Inventory; attacking and game movement remain blocked while browsing C. UI geometry is mirrored in Canvas rendering and mouse hit testing, with compact labels for narrow windows.
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

## Creator-to-gameplay ancestry bridge
The world encyclopedia still has **30 stable race IDs**. The native creator
now offers **15 eligible choices** (11 existing plus Orc, Goblin, Troll and
Treekin), each with a deliberate, documented mapping into the **12 existing**
saved hero ancestry IDs. **No save migration and no new 13th ID** are needed.

| Creator choice | Saved hero gameplay ancestry (ID) |
| --- | --- |
| Human | Human (0) |
| High Elf, Half Elf | High Elf (1) |
| Wood Elf | Wood Elf (2) |
| Dwarf, Hill Dwarf, Mountain Dwarf | Dwarf (3) |
| Smallfolk | Halfling (4) |
| Orc | Orc (5) |
| Goblin | Goblin (6) |
| Troll | Troll (7) |
| Treekin | Entkin (8) |
| Twilight Elf (dark_elf) | Shadowborn (9) |
| Riverfolk | Moonborn (10) |
| Beastkin | Draconid (11) |

**Regional variants share the saved ancestry's *actual* existing gameplay
bonuses**. Their creation-screen vitality/power/speed multipliers are
illustrative previews, not independent hidden buffs; the creator shows the
real gameplay lineage, Might/Insight/Resolve and regeneration passives, and
racial-power name before selection. The on-screen choice is therefore
explicit, not a silent conversion.

When a live locally controlled UE5 hero owns the character creation UI, it
can now confirm by calling the existing `ChooseRace(int32)`. That method
persists the race using the established checkpoint format and rolls it back
if writing fails. The **Continue** callback and the new saved-ID delegate
fire *only* after successful save; missing/uncontrolled heroes, nonplayable
creatures, previously chosen ancestries, and save failures display a readable
error. The creator automatically tries to find the current player pawn, and
exposes `BindHeroForConfirmation` for authored player-controller flows.
The preview actor, art and scene-capture rig remain unchanged.

## Remaining integration and validation
The character-creation widget still needs to be instantiated and presented
by the actual UE5 game flow or a Blueprint in builds where it is not already
present. Source tests do **not** establish that a launch-time selection menu,
native UE5 Windows compilation, or a live creator-to-checkpoint playthrough
has succeeded. Verify selection, confirmation, failed save/rollback, reload,
controller and mouse input, and exact previews in Unreal Editor before release.

No save schema, original Z/Nyra art, world NPC race IDs, WASD/arrow mapping, or
existing hero race bonus formulas were altered.
