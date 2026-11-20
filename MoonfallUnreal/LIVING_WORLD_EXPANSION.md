# Moonfall Living World: Six-System Expansion

This is an **original Moonfall implementation inspired by systemic fantasy RPG design**, not imported Fable assets, dialogue, maps, or copyrighted character designs.

## Existing verified source touchpoints

The UE5 project already contains:
- Resident bond values, named NPC interaction and persistent resident quest states.
- Faction reputation plus persistent saves.
- Starting sanctuary anchor and discoverable shrine logic.
- Equipment rarity/tier data, inventory, and upgrade actions.
- Left/right spell assignments and dual-cast input.

These pieces are **not a complete version** of the six proposed systems.

## Added in this first implementation

`MoonfallLivingWorldRules.h` provides independent, bounded C++17 rules:

1. **NPC relationships:** relationship ranks, witness-dependent trust changes, and one-time personal quest gates. Does not change existing ResidentBond persistence.
2. **Evolving weapons:** deterministic four-stage weapon evolution from *verified* victories, upgrade count, and rarity. No art swap, weapon mesh, or new save fields yet.
3. **Living settlements:** settlement status calculated from restored sites, completed contracts, discovered shrine and hostile raids. No population simulation or constructed settlements yet.
4. **Sanctuary:** room-unlock policy for Atlas, Armory, Wardrobe, Trophy Hall, Council and Portal. These are proposed rooms, **not placed level geometry**. The mouse/controller UI must remain directly accessible.
5. **Spell weaving:** symmetric, validated elemental-pair classifications, mana threshold and menu guard. **Proposed** mappings are 0/1/2; integration must first validate actual Moonfall spell-school IDs. No new damage or VFX yet.
6. **Council decisions:** three bounded choices with faction, prosperity and threat consequences. Does not mutate actual save data or factions.

`tools/test_unreal_living_world_rules.cpp` verifies boundaries, relationships, reversibility, combat eligibility and progression gates without Unreal headers. CI executes this as a standalone C++17 test.

## First UE5 gameplay integration committed

- `AMoonfallHeroPawn::GetResidentRelationshipRank` now exposes read-only Hostile/Wary/Neutral/Trusted/Devoted tiers to Blueprint and UI, derived from saved `ResidentBonds`.
- Named world NPCs now recognize Trusted and Devoted bonds in dialogue **after an existing one-time contract is completed**. Talking cannot farm trust, and the existing quest/salvage transaction and save format remain unchanged.
- Python source regressions verify the bridge reads persisted bonds and does not save or award points while talking. Native UE5 compilation and runtime gameplay testing are still needed.

## Read-only community progression adapter

- `AMoonfallHeroPawn::GetWorldCommunityOutlook()` now derives Endangered, Recovering or Flourishing from the **already saved** completed resident contracts and whether any shrine has been discovered. This is a world-level summary, not yet a per-settlement simulator.
- The query never changes persistent data, spawns actors, grants rewards or fabricates raids/restored buildings. `SettlementStatus` can later accept those events once their data is reliably recorded.
- An additional source regression test protects read-only behavior and authentic save-field wiring. No native Unreal build is claimed.

## Equipped-weapon milestone integration

- A read-only `GetEquippedWeaponResonance()` now shows Dormant, Awakened, Ascendant or Mythic in the UE5 character sheet. This uses only an owned, validated equipped weapon's rarity/tier and the **global saved dungeon-boss defeat ledger** (up to 30 unique bosses).
- This is **global boss resonance**, not per-weapon victory tracking or physical weapon evolution. It does not change stats, visuals, ownership, item codes, or save versions.
- Unreal-independent C++ rule tests cover impossible and boundary progression; Python source checks verify the exact saved-ledger, equipped-item and HUD wiring.

## Next integration milestones (not completed by this PR)

- **NPC relationships:** map the relationship outcomes to verified named NPC choices and save updates with transactional rollback. Add time-stamped quest history only after save-schema migration and corruption tests.
- **Weapon evolution:** use owned weapon codes; capture distinct boss milestones durably, prevent duplicated credit, and bind preapproved unique mesh/effect variants. Ensure ownership and accessibility.
- **Settlement simulation:** introduce stable region identifiers, bounded scheduled updates, player proximity/streaming policy, and regression coverage for persistence and performance.
- **Sanctuary rooms:** author real UE5 room actors and routes, preserve current sanctuary and direct-access inventory/Atlas. Validate controller and mouse interaction.
- **Spell weaving:** match real spell schools, implement cost/damage balancing, guard against repeat inputs, and test equipped spell switching.
- **Council choices:** create explicit confirmation UI, transactional choices and save migration, NPC/settlement effects, and reversible or permanent choice messaging.

## Integration and release safety

- The existing Godot version is preserved. Unreal 5 remains the long-term engine.
- Original Z and Nyra artwork must remain pixel-for-pixel unchanged.
- No original source images or Unreal texture assets are added or edited here.
- Any native build, playable scene integration, boss behavior, graphics, packaging, Windows installer, and controller playtesting remain **unverified**.
- Keep as a draft PR until native tests and the first actual in-game integration are reviewed.
