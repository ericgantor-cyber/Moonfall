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

## Mixed-school spell weaving integration

- The existing controller **DualCast** action now attempts a guarded offensive hybrid when the two equipped schools differ and both are in the current supported 0–2 set.
- A successful hybrid delivers **one** hostile `TryCast` using the left school's damage/targeting with a bounded 1.25 multiplier, spends its base 1.25× mana **plus 0.5× the right school cost**, and applies both hand cooldowns. Invalid targets charge nothing.
- Successful casts dispatch the new optional `OnSpellWeaved(LeftSchool, RightSchool, HybridType)` Blueprint VFX/UI hook. Specific bespoke hybrid particles, animations, sound, and secondary effects are **not bound yet**; a cast currently uses the left spell's visuals. Same-school doubled spells remain unchanged.
- Only three offensive schools are supported in this first implementation; healing and other schools cannot weave. Current IDs 0,1,2 are confirmed offensive, but the original conceptual hybrid names are placeholders until the visual school definitions are verified.
- New source regressions cover billing/cooldown order, the existing same-school path, and the no-write promise. Native UE5 compilation, visual testing, and balance playtesting remain unverified.

## Sanctuary progression and council UI adapters

- Blueprint/UI can call `IsSanctuaryRoomUnlocked(RoomId)` (0=Atlas, 1=Armory, 2=Wardrobe, 3=Trophy Hall, 4=Council, 5=Portal). Only the durable `sanctuary_firstlight` discovery unlocks the initial three; verified dungeon bosses and completed resident contracts determine later access. Invalid room numbers are rejected. These are **access policies only**, not placed architectural rooms.
- `PreviewCouncilDecision(DecisionId)` (0=Protect Residents, 1=Fund Reconstruction, 2=Fortify Borders) returns constrained faction/prosperity/threat outcomes for UI preview. **No choice is executed**, and no faction reputation, game save, or world simulation state is mutated.
- Native UE5 compilation, room geometry and navigation, UI bindings, controller focus, council confirmation and transactional persistence remain outstanding.

## Contextual Sanctuary HUD (first visible world integration)

- A read-only sanctuary ledger now appears on displays at least 1280×700 while the hero is near the starting sanctuary, with the six existing policy unlocks, real saved community outlook, and council availability.
- The panel does not intercept clicks, replaces no existing Atlas/quest controls, and is hidden while Atlas, inventory or options are open. On smaller displays, normal game HUD remains uncluttered.
- This is a **contextual progression interface**, not actual constructed sanctuary rooms or interactive council votes. World geometry, in-game choice confirmation, and transactional outcomes remain to be implemented and playtested.

## Controller-browsable council previews

- Existing D-pad left/right actions can cycle the three council previews while the hero is near the starting sanctuary, the Council is unlocked and no modal UI is active. Other D-pad input behavior is unchanged.
- The contextual HUD shows the selected decision title and the bounded, proposed faction/prosperity/threat effects. `SelectCouncilPreview` is also Blueprint-callable for a future fully clickable UI.
- **No decisions are executed**: selecting a preview never saves, mutates faction standings, creates buildings, or changes threats. Mouse controls and actual world consequences still require separate integration and native playtesting.

## F8 Council mouse previews

- At an unlocked Council in the starting sanctuary, press **F8** to show a mouse pointer and the three read-only preview buttons. Left-click Protect, Rebuild or Fortify to change the selected preview; **F8**, **Esc**, or right-click dismisses pointer mode. It closes automatically if the hero leaves the sanctuary or enters another menu.
- This pointer mode consumes left-clicks without casting spells, and blocks spell casts while active. The hit rectangles match the HUD and the preview logic does not save or modify the kingdom.
- The controller D-pad remains supported. This is **not a decision-confirmation or persistent council simulation**. Native Windows UE5 compilation, playtesting and mouse/controller accessibility validation are still required.

## Permanent Council choice: V13 save migration

- **F9 twice** after choosing a Council preview records one permanent Council policy. First press only arms confirmation; the second attempts a save. The Council must be unlocked, the hero alive and in the starting sanctuary, and other gameplay modals closed.
- Save data now uses V13, adding bounded `CouncilDecisionId` with `-1` for no choice. V1–V12 saves load as undecided; malformed V13 decisions and unknown future versions fail closed before overwriting progress. Failed saves restore the previous uncommitted value.
- The HUD displays the saved choice and a confirmation prompt. Existing previews remain safe: mouse and D-pad selection are never a commitment.
- **Immediate reaction:** NPCs whose contracts are completed acknowledge the saved policy in their subsequent dialogue. This does not give rewards or retroactively change quests.
- **Limit:** The Council policy is recorded and changes contextual NPC dialogue, but not faction reputation, settlement geometry, enemy threat, or constructed buildings. Those require explicit integration, native UE5 compilation and gameplay tests.

## Crossroads policy appearance integration

- The existing Crossroads now has three small, non-colliding instanced geometric decoration sets and a nonblocking in-world policy sign. After a confirmed V13 Council choice, the active set and sign change to **Protection**, **Reconstruction**, or **Fortification**.
- The arena checks the player's already-saved policy every two seconds, so choosing a policy or loading a saved policy refreshes visuals without per-frame actor ticking, NPC relocation, new map levels or modifying the player's save. All variant geometry has collision and overlap disabled.
- These are low-cost **visual indicators**, not actual repaired homes, working construction sites, staffed fortifications, altered quests or gameplay bonuses. The original hero art and gameplay world blockers are unchanged. Native Windows UE5 compilation and in-game collision, placement, controller and visuals testing remain outstanding.

## Council-responsive Crossroads professions

- Crossroads residents expose `GetCouncilAssignment(Hero)`, a no-tick, read-only description derived from the **saved Council policy** and each resident's existing `SocialRole`. Protection assigns guards to residents and scouts to escorts; Reconstruction assigns tinkers to repairs and merchants to supplies; Fortification assigns guards to borders and scouts to roads.
- The native NPC conversation panel displays the current assignment, but an active quest-feedback message takes priority. NPCs from other settlements and heroes without a confirmed policy retain their normal existing activity description.
- The jobs are **descriptive world reactivity**, not physical navigation, inventory trading, actual construction or simulated defense. No new saves, NPC actors, pathfinding, or automatic reputation awards. Native UE5 gameplay/visual testing remains outstanding.

## Visible NPC Council assignments

- Crossroads residents now have small, nonblocking, world-space text labels describing the policy-driven work already available in their conversation menu. Labels are hidden until a Council policy has been saved and take on a restrained color corresponding to Protection, Reconstruction or Fortification.
- The existing arena's two-second policy refresh changes the labels **only on policy transitions**, not per NPC tick or per frame. The resident's original dialogue, actor location, body, collisions, quest rewards, and original character art are untouched.
- These labels represent assignments, **not simulated walking, repairing, guarding or changing daily schedules**. Placement, visibility, legibility and native Unreal execution still require a Windows UE5 build/playtest.

## First bounded NPC workplace movement

- Up to four authored Crossroads **guards and scouts** can pace a short 100-unit route after a confirmed Council decision. The arena drives movement from a shared 0.2-second timer; NPC actors still have **no per-frame Tick** and no AI controller or additional spawn count.
- Each resident first requires an available NavMesh, two nearby projected endpoints, and a **direct two-point path** no longer than 180 units. Paths requiring a turn around architecture are rejected because the movement code follows a straight segment, not arbitrary NavMesh corners. A route-wide visibility obstacle check and a fresh obstacle check before each swept step prevent walking through the existing scenery. Invalid/missing navigation leaves that NPC stationary. Movement is capped at 55 units/second, aborts on a collision, and never exceeds a 145-unit radius from home. Guards use a short eastward watch lane; scouts use a westward survey lane.
- Motion pauses when the hero approaches within 300 units, when a conversation with that resident is open, or while hero menus are open. All quest/dialogue/saves, existing hero artwork, and other residents' placement remain unchanged.
- **Not a full NPC AI scheduler:** this is a small bidirectional patrol along validated short endpoints. No multi-waypoint path following, work animations, building repairs, or complex crowd avoidance have been authored. Native Windows UE5 compilation, NavMesh availability and in-game movement/collision need direct verification.

## Workplace stops and resident work-state UI

- Guard/scout patrols now pause **five seconds** at a validated watch/scout destination and **two seconds** at their authored home before the next trip. The existing shared 0.2-second arena timer advances the bounded pauses; no additional actor Tick, timers, animations, inventories, or save fields are introduced.
- The NPC conversation pane displays the resident's work phase: walking, working at post, returning, resting, or stationed when no NavMesh patrol exists. The phase is descriptive; no repairs or defensive combat effects are granted.
- The existing menu, player-proximity and conversation pauses take priority over workplace progression. This remains subject to native UE5 testing for placement, collision and visual legibility.

## Visible work activity indicators

- The existing Council guard/scout patrols now show a tiny non-colliding mesh glyph during their validated five-second workplace stop. It bobs and rotates at the existing 0.2-second arena update cadence, then hides while walking or resting. No added NPC actors, skeletal replacement, per-frame Tick, animations assets or save fields.
- The primitive work glyph is an early visual activity cue, **not a finished weapon/repair animation or simulated construction**. Native Unreal build, visual placement and controller/interaction tests are still required; original hero artwork remains untouched.

## Guard and scout workplace cue variants

- Guards now use a taller rotating primitive watch beacon; scouts use a flatter lateral-sweeping compass cue while at their validated post. Both reuse the original non-colliding work marker and the shared 0.2-second movement schedule. Neither replaces the resident's actual character/body artwork.
- Work cues are hidden immediately when the player approaches within 300 units, a conversation/other modal interrupts, or a route stops being valid. The existing five-second work hold pauses while the player is nearby, then resumes.
- These are **lightweight role-distinguishing animations**, not skeletal actor animations, repaired buildings, or combat-changing patrols. Native UE5 compilation and in-world visibility still need testing.

## Stationary merchant and tinker workplace equipment

- Existing Crossroads merchants display low, non-colliding primitive supply crates and the tinker displays a narrow, periodically repositioned equipment marker. The saved Council policy determines the tinker marker's proportions. These are simple **visual workplace props**, not finished tool animation or stock management.
- The arena maintains a bounded list of authored merchant and tinker NPCs, updating their marker states through the pre-existing two-second Council settlement timer. No new actor spawns, inventory grants, physics collisions, NPC Tick, save fields, or alterations to original character models.
- Markers hide when there is no confirmed Council policy. Native Unreal compilation and in-game visual placement remain to be verified.

## First transactional merchant and tinker interactions

- The native NPC conversation panel now has a **fourth Trade/Craft row**, selectable by existing mouse and controller D-pad + A navigation. Merchants sell a fixed starter weapon for 12 salvage; the Crossroads tinker crafts a fixed uncommon weapon for 20. A confirmed V13 Council policy is required. Other residents display the action as unavailable.
- Both operations require a second explicit selection within six seconds. They check inventory capacity, the resident's live interaction range, player state, and the existing persistence write block. Item addition and salvage deduction are saved synchronously in one world-save call; failure rolls back both.
- These are deliberately **small fixed-item transactions**, not an item catalog, buy-back, economy simulation, recipe discovery, crafting preview or finished merchant shop. They make no auto-equip or faction changes. Native UE5 mouse/controller and actual saving still require in-game testing.

## 111-entry merchant and tinker catalogue

- The fourth NPC menu row now browses **111 deterministic equipment entries**. Each one is a distinct valid encoded gear code across the existing **weapon, armor and accessory** slots, with 37 tier/rarity combinations per slot. No new SaveGame schema or fictional item codes.
- **D-pad left/right** while the Trade/Craft row is selected, or the clickable left/right edges of that row, moves through the entries with wraparound. The menu previews the item number (1/111), slot, tier, rarity, encoded item code, and the price. The center/A action still requires two explicit selections within six seconds.
- Browsing clears pending purchase confirmation, and buying/crafting persists item code and salvage together with rollback on save failure. A pure C++17 regression loop exhaustively checks 111 unique valid item codes, prices and navigation bounds.
- This is an **initial numerical equipment catalogue**, not 111 unique 3D models, icons, individual item names or distinct crafting animations. Merchant and tinker draw from the same 111 underlying designs, with separate prices. Native UE5 compilation, pointer layout and save/interaction playtesting remain unverified.

## Named equipment and crafting progression

- Each of the 111 catalog entries has a deterministic individual name combining one of 37 fantasy epithets with its equipment type: **Blade, Plate, or Charm**. Names reflect numeric item designs, not separate meshes or sprites. The native NPC menu now displays the name, tier, rarity, item code and salvage price.
- Tinker crafting requires level **1 through 5** according to the selected item's tier (`1 + (tier - 1) / 5`). The menu marks recipes locked if the player does not meet this requirement; attempting a locked recipe does not spend salvage, add items or change the save. Merchant buying remains available based on existing salvage/interaction checks.
- C++17 tests check every catalogue entry, progression thresholds and out-of-range cases. No new save schema or changes to Z/Nyra artwork. Full Unreal Editor compile and gameplay testing remain outstanding.

## Good / Evil morality foundation (Fable III-inspired, original Moonfall design)

- A persistent Good/Evil alignment score ranges from **-100 to +100**. The read-only native NPC conversation panel shows the score and a standing label (Dreaded, Cruel, Unaligned, Kind, Exalted). This does not overwrite independent faction reputation or personal resident bonds.
- The existing once-per-save Council decision contributes **+10 Good** for Protect Residents, **+8 Good** for Fund Reconstruction and **0** for Fortify Borders (a defensive policy is not automatically evil). Completing a real resident contract earns **+4 Good** and remains a single atomic save with its original salvage/reputation/bond consequences.
- Crossroads residents offer two new voluntary one-time actions through a **fifth NPC menu row** (D-pad up/down + A or mouse, left/right switches the action). **Aid:** donate 10 salvage, gain 10 Good, +6 personal trust and +2 faction reputation. **Extort:** take 5 salvage, gain 10 Evil, lose 6 personal trust and 4 faction reputation. Both require a second confirmation within six seconds. Each resident remembers one decision, stored by stable resident ID, preventing repeat farming.
- **Save V14** stores MoralAlignment and ResidentMoralChoices. V1-V13 saves remain readable; existing V13 Council decisions seed initial morality. Corrupt/unsupported values are rejected before mutating progress; all moral consequences and currency are rolled back when a save write fails.
- Residents add contextual lines when the player reaches very Good or Evil standing, preserving faction disposition and quest dialogue. This is **not** a copy of Fable's graphics, character morphing, wing effects, town rebuilding or royal treasury. Future choices, visual morphs and settlement outcomes require separate art/behaviour implementation and native UE5 gameplay validation.
- Updated pure C++17 and Python source-contract tests cover alignment bounds, decision outcomes, save versions, confirmation and rollback. Native Windows Unreal compilation, saved-game migration and mouse/controller playtests remain unverified.

## Read-only moral feedback in the world and character screen

- The existing character-sheet panel now shows a **color-and-text morality meter**, centered at zero, with negative alignment extending left and positive alignment right. The textual title and numeric score preserve accessibility without relying on color alone. The meter is read-only, creates no actors or materials, and does not touch original character artwork.
- A resident who has been aided displays **[HELPED]** in their existing Council assignment sign with a green tint, while a resident who has been extorted displays **[THREATENED]** in a red tint. The single affected label is refreshed only after the confirmed transaction saves successfully; a failed transaction retains the original sign. These markers are visible only in Crossroads when a Council policy is confirmed, using the pre-existing noncolliding sign.
- NPC Talk lines now individually acknowledge prior one-time aid/extortion decisions. Reputation, quests, inventories, and saved player appearance are not modified by the read-only UI feedback. **Angelic/demonic body morphs, world remodelling, crowd reactions, weapon appearance changes, and native UE5 gameplay validation are still outstanding.**

## Morality-driven Crossroads and economy (expanded)

- The saved Good/Evil alignment now has **five world tiers** at -60, -25, 0, +25, and +60. Crossroads swaps between collision-free **Hope** plinths and **Shadow** spires using two fixed four-instance meshes already available in Unreal. This does not mutate level terrain, hero meshes, authored Z/Nyra artwork, collision, faction scores or saved geometry. The Council sign includes an on-demand morality-world label.
- The arena reuses its existing two-second Council visual refresh. It compares both the saved Council policy and the derived morality tier before changing displays, with no per-frame scan. Global NPC sign scans remain limited to **policy transitions**, not every morality change. The individual changed resident is refreshed after its moral transaction saves.
- Merchants and tinkers now quote and charge the **same morality-adjusted salvage price**: at +25 Good a 5% rounded-up discount, at +60 Good a 10% rounded-up discount, at -25 Evil a 10% rounded-up surcharge, and at -60 Evil a 20% rounded-up surcharge; otherwise full price. No free/negative prices. The native shop preview labels a Good-standing or Feared adjustment. Existing confirmation and synchronous save rollback stay intact.
- This is a real **world visual and trading consequence**, not yet Fable-style physical player morphing, NPC crowds, town rebuilds, new architecture assets, or a dynamic society simulation. Native UE5 Windows compile and in-game readability/collision checks are still unverified.

## Hero morality silhouettes and gentle presence light

- At sustained **Exalted (+60)** or **Dreaded (-60)** alignment, the hero now displays two optional mirrored primitive ornaments and a small color-coded presence light attached to the capsule root. Good alignment uses outward-slanting, gold-lit motifs; Evil uses inward-slanting, crimson-lit motifs. Both are read-only cosmetic attachments and do not change the hero's original skeletal mesh, textures, animations, collision, damage or existing save.
- All ornament mesh parts are collision-free, overlap-free and shadowless. The light is shadowless and radius-limited to 190 Unreal units. It stays disabled at neutral/mild alignment, before save initialization and while defeated.
- The existing hero Tick accumulates a **0.5-second** bounded timer and only updates transforms and visibility if the alignment threshold or accessibility Reduce Motion setting changes. There is **no spinning/pulsing animation**, extra spawned NPCs or per-frame effect transform manipulation. Reduce Motion lowers light intensity to 90 rather than 210.
- Z and Nyra's original artwork is preserved. These are stylized **primitive placeholder silhouettes**, not authentic wing/horn assets, full-body morphs, new materials or finished UE5 effects. They still require native Unreal compilation and art/gameplay review.

## Moral equipment identity, without changing original art

- Valid equipped gear owned by the player receives a **read-only thematic identity** derived from saved alignment: Unmarked at neutral, Hallowed/Radiant for growing/strong Good alignment, and Shadowmarked/Dreadbound for growing/strong Evil alignment. Unowned, malformed, absent or wrong-slot gear always reports no equipped identity.
- The full-height native character sheet (820px or taller) displays the three equipment slots' moral theme and a text description of the associated visual motif. Compact character sheets omit these optional lines to avoid colliding with gameplay stats. The previous original stats, weapon resonance and equipment set bonuses remain unaffected.
- These are **text-only cosmetic identity previews**, not item mesh/material recolors or 111 new artwork assets. This is deliberate: original Z/Nyra art, inventory item codes, rarity/tier, combat calculations and V14 save data remain untouched. Pure C++17 tests cover the five alignment states and the no-gear case; source-contract tests prohibit mutation in preview code.

## Equipped gear gets original, non-destructive morality silhouettes

- The hero now has **three separate cosmetic primitive motifs** for equipped weapons, armor and accessories. Their shape and relative orientation vary between Good and Evil standing. They become visible at +25 or -25 morality and remain hidden for neutral characters, defeated heroes, missing gear, unowned items or invalid equipment codes.
- The same half-second cached visual refresh that drives the hero's existing +60/-60 morality ornament now also checks the three actually equipped slots. The cache includes an ownership/visibility mask, so equipping or unequipping gear updates the appearance without changing moral tier.
- All three meshes are attached to the capsule root, use the already-loaded Unreal cube, have no collision, overlaps or cast shadows, and are **completely separate** from original Z and Nyra character and equipment art. No saving, equipment stats, character meshes, item codes or inventory operations are changed by this purely cosmetic effect.
- These are simple geometric **working placeholders**, not finished mesh-specific weapons, materials, textures, particle systems or full Fable-style transformations. Native UE5 rendering and layout still need testing.

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
