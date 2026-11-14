# Moonfall fantasy race roster (UE5)

This is an **original Moonfall implementation**, drawing on broad fantasy race archetypes, not an officially licensed Lord of the Rings adaptation. No Tolkien art, characters, place names, logos, or copyrighted game assets have been imported. If you plan to market this as LOTR, acquire the appropriate licensing.

## Race catalog
The native Blueprint library `UMoonfallRaceLibrary` provides 30 stable race IDs:
- Free peoples: `human`, `smallfolk`, `riverfolk`
- Elven: `high_elf`, `wood_elf`, `dark_elf`, `half_elf`
- Dwarven: `dwarf`, `hill_dwarf`, `mountain_dwarf`
- Shadowkin: `orc`, `war_orc`, `goblin`, `cave_goblin`
- Giants: `troll`, `swamp_troll`, `ogre`, `giant`
- Nature: `treekin`, `forest_guardian`
- Sky: `eaglekin`
- Draconic: `dragon`, `drake`
- Beasts: `warg`, `spiderkin`, `beastkin`
- Spirits: `wraith`, `ancient_spirit`
- Infernal: `fire_demon`
- Elemental: `stonekin`

## Blueprint integration
- Call `GetRaceIds` to populate character-selection or spawn UI. Use `FindRaceProfile` for translated display names/families and trait multipliers.
- `bPlayableCandidate` flags only **potential** playable choices. It does not automatically make any race selectable or change Z/Nyra. The original protagonists' identities and artwork remain exactly as provided.
- On an `AMoonfallEnemy` Blueprint, set `RaceId` to one of the IDs. At spawn, a known race modifies **health and attack damage only** with bounded multipliers, preserving elite/boss level scaling. Unknown or empty IDs preserve previous behavior. `ResolvedRaceFamily` exposes the family for UI, spawning and loot rules.
- `SpeedMultiplier` now affects enemy **chase movement**, clamped to 0.65×–1.35× and resolved once at spawn as `RaceMovementMultiplier`. It does not change attack cadence, navigation query frequency, or stored `ChaseSpeed`. Unassigned/unknown races use exactly 1×.
- Assign original mesh, animation rig, sound and portrait assets per Blueprint. Nothing automatically creates art, skeletons, race-specific AI, or character selection.

## Verification still required
Native UE5 Win64 compilation and UHT parsing, actual Blueprint setup, skeleton/animation integration, balancing and gameplay tests are not yet performed. This work stays in draft PR #910; do not merge based on source contract CI alone.
