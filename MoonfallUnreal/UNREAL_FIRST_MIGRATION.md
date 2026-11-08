# Moonfall Unreal-first migration: verified boundaries

**Decision:** Unreal Engine 5 is the long-term target. Keep `godot-v38/` intact as the source-of-truth baseline until native Unreal Win64 playtests reach parity. Keep Z and Nyra's original source artwork untouched.

## Milestone 1: continuous test-world foundation (this PR)
- Six touching region floors, five dungeon corridors per region and fourteen traversable stage floors per corridor (30 x 14 = 420 stage locations).
- Encounter stages 3, 5, 9 and 12 give 120 resident slots; stage 14 gives 30 final guardians; three regional wildlife slots in each of the six regions give 18 additional enemies. These are individual native `AMoonfallEnemy` actor slots, not waves or fully authored species.
- Bounded proximity work: at most one new room mesh and one new enemy per 0.25-second world scan; no pre-spawning all 168 enemies. Only already approached rooms allocate their stage meshes.
- Uses existing `MoonfallHeroPawn`, `MoonfallEnemy`, combat and damage rules. Existing five-enemy arena remains available with `bContinuousWorld = false`.
- An isolated versioned Unreal SaveGame now stores validated defeated encounter IDs after **observed lethal hits**, and suppresses their respawn across restarts. Save writes retry on subsequent world ticks after a failure. The save excludes unrelated hero data and never reads Godot files. Persistence remains untested in the Unreal runtime.
- **Missing:** true Godot dungeon level design, obstacles, art, navigation volumes, physically distinct rooms, treasure and rewards, minimap/atlas, quests, Godot save migration, hero checkpoints, boss identity and animations, packaged Windows playtest.
- Source-contract unit tests are **not** Unreal Editor compilation or runtime performance evidence.

## Next migration gates
1. Compile and run UE 5.4 project on Windows against the exact branch/commit. Test world traversal, navigation, collision, streaming and attack reach. Correct issues before distributing.
2. Verify per-encounter defeated IDs in Unreal Editor across restart and simulated save failure; add hero checkpoints, room discoveries, migration handling and corruption recovery.
3. Port dungeon quest state and Raven Atlas discovery without adding combat HUD clutter; only visible minimap and red HP/blue mana/purple XP rings remain.
4. Reuse original Z/Nyra art **byte-identically**; verify imports and attribution. Do not turn original pictures into new designs.
5. Port Godot items, rarity/loot/crafting, professions, and quest rewards with golden reference fixtures. Never conflate `engine-comparison/` with the main `MoonfallUnreal/` module.
6. Add animation/VFX, real boss behaviors, controller accessibility and practical 1080p/1440p/4K GPU profiling.
7. Run repeatable Windows 10/11 editor build, cooked package, installer, clean-machine launch, suspend/save/load, input and gameplay tests. Only then reconsider Godot retirement.

## Engine/version caveat
The main `MoonfallUnreal.uproject` currently declares Unreal Engine 5.4. Do not claim it is compiled under 5.6 or rename its association until the configured toolchain is tested.

## Regional enemy upgrade (18 distinct native profiles)
Each of the six regions now has three named enemies: a faster skirmisher, a tougher frontline combatant, and an elite variant. All 18 use deterministic, bounded adjustments to existing native movement speed, hit reach, attack interval, health, damage and posture. This makes the existing fights play differently without introducing wave spawners, new combat systems or additional Tick actors. Names are exposed through `ArchetypeName` for later UI/VFX hookups. The profile roster includes Ashling Skirmisher, Cinderback Brute, Emberclaw Warden, Mirewing Stalker, Bogroot Crusher, Fenfang Alpha, Frostbite Runner, Glacial Bulwark, Rimehorn Ravager, Stormveil Prowler, Thundershell Sentinel, Tempest Fang, Ironwhisker Scout, Foundrybreaker, Sparkcoil Executioner, Moonshade Pouncer, Voidplate Colossus and Eclipse Reaver.

These remain the existing prototype enemy meshes and inherited AI moves, **not** eighteen independently modeled, fully animated creatures or eighteen new attack trees. Names and stat bounds are source-testable; Unreal gameplay and performance need verification in the engine. All regional encounter IDs, saved defeat state, dungeon residents and original Godot artwork stay unchanged.

## Additional overworld roster: 36 instead of 18 (source upgrade)
Three **additional** named wildlife profiles per region now bring the regional overworld to **36 distinct encounter slots** (six per region), while retaining 120 dungeon residents and 30 final dungeon guardians for **186 total encounter slots**. The new names are Ashen Viper, Magma Tusk, Coalwing Harrier, Sporefang Hunter, Mossbound Titan, Rotclaw Ambusher, Snowveil Lynx, Frostmaw Behemoth, Iceglass Stalker, Lightning Jackal, Thunderhoof Juggernaut, Cloudpiercer, Rivet Hound, Slagforge Goliath, Voltspine Pursuer, Nightglass Wraith, Abyssal Hornbeast, and Dreadmoon Ravager. They receive individually bounded movement, vitality, attack reach/cadence and posture profiles through the existing native combat actor.

**Save compatibility:** original overworld IDs 0–17 and original dungeon IDs 18–437 are unchanged. New overworld IDs are 438–455; schema version 1 validates both ranges, retaining prior defeats across restarts. The streaming scan still spawns no more than one enemy per tick. These are **configurable combat profiles on placeholder meshes**, not 36 independently animated models or new artificial-intelligence classes. Unreal Editor compilation and gameplay are still unverified.

## Six-style regional combat overhaul
Each of the 36 wildlife archetypes now selects one of six native attack rhythms using a deterministic profile index: pouncer, bulwark, reaver, harrier, juggernaut, or hunter. These adjust *telegraphed* strike windup, damage and effective reach within hard bounds while keeping the inherited 3-pattern combat rotation, dodge/parry interrupt logic, damage validation, and posture reaction. Regional windups draw a temporary debug-only circle without spawning new actors; the final animated attack assets/VFX are **not implemented**. Boss specials are left unaffected. The original Godot gameplay, saves and Z/Nyra artwork remain untouched.

Only source-level regression contracts have been added so far. Real Unreal Editor compilation, timing balance, controller accessibility and 3D animation validation remain required.

## Packaged-renderer combat visual foundation
Every native enemy now owns one reusable non-shadow-casting point light, hidden outside active attack windups and on defeat/menu pauses. It brightens toward the attack release with bounded intensity and alternates warm/cool style colors; boss warnings use a separate hue. The existing placeholder mesh leans and twists in anticipation. These are procedural in-game component effects, **not** Unreal debug drawing, new spawned VFX actors, skeletal animation assets, Niagara particle systems, or a finished animation library.

The combat hit logic, timing, damage, parry/dodge handling, navigation and original art are left intact. Native Unreal 5.4 compilation, actual packaged rendering, lighting cost and accessibility/contrast still require testing. On slower GPUs, benchmark the total cost of simultaneous dynamic warning lights before enabling many live enemies.

## Attack-sequence correctness and animation integration
Corrected a regional AI regression: the original six rhythm modifiers had been applied *before* the baseline attack-pattern assignments, which overwrote them. Their bounded modifications now apply **after** selecting the base pattern and boss special, before committing the strike. This preserves readable windups and means the six styles actually influence timing, reach and damage.

Added once-per-attack `OnAttackWindup` and `OnAttackReleased` Blueprint events (including follow-up boss strikes), so authored montages and Niagara cues can attach without affecting collision, dodge/parry damage, attack recovery or spawning extra actors. These hooks do **not** constitute authored skeletal animations or finished Niagara effects; a UE 5.4 Windows build and playtest are still required.

## Exploration map and saved dungeon discovery
Entering within 450 Unreal units of any of the 420 chamber locations permanently reveals that chamber for the Unreal prototype's compact north-up minimap. Unknown chamber locations stay blank, and only explored chambers within the local 4,800-unit radius are drawn. The center marker follows the actual hero location, using one existing canvas HUD and no new radar actor. No quest or enemy-tracker markers are invented.

The existing isolated schema-v1 Unreal SaveGame now also stores `DiscoveredChamberIds` as an optional bounded array. Old saves without the field still load defeated encounter progress; exploration is empty until discovered. Both arrays are saved together so an exploration write does not erase the previous defeated-enemy mask. It does not read or change Godot saves. This is an unexplored-area visibility **prototype**, not the full Godot Raven Atlas with authored terrain, atlas icons or world map textures. Unreal 5.4 compilation and in-editor save/reload remain unverified.

## Explored terrain and dungeon entrance map (next increment)
The in-game north-up Raven Atlas prototype now displays simple biome-colored terrain silhouettes for **visited regions only**, plus separately earned entrance icons for the 30 dungeon paths. Regional discovery requires physically approaching a region center (2,200 UU); entrance discovery requires approaching the actual first-chamber point (650 UU). Map data is stored as optional bounded `DiscoveredRegionIds` and `DiscoveredDungeonEntranceIds` alongside discovered chambers and defeated enemy IDs in the isolated, backward-compatible Unreal SaveGame v1. No unexplored entrances, terrain regions or chamber IDs are drawn. No quest icons are shown because main Unreal has not yet ported trustworthy quest state. These are basic canvas markers and colors, not Godot's fully authored terrain atlas or complete quest tracker.

Source-only checks cover ID bounds, map index math and fog gating. Native Unreal 5.4 compilation, save/reload, map visual positioning and Windows packaging remain unverified.

## Grounded guardian quest tracking (next parity increment)
The main Unreal project now derives **30 dungeon guardian quests from real encounter IDs**, not fabricated map destinations. The guardian objective for entrance `E` is complete only when the final chamber's native enemy ID `18 + E * 14 + 13` is in the persistent defeated-enemy set. Discovering a dungeon automatically tracks the first known entrance unless another quest is selected. The Blueprint-callable `TrackDungeonQuest(E)` refuses undiscovered entrances. Tracked quest ID is stored as an optional field in the existing isolated schema-v1 Unreal SaveGame; older saves without it select the lowest previously discovered entrance without losing victories.

The Raven Atlas renders a compact guardian quest label and the number of defeated guardians (out of 30) next to the minimap. Known entrance icons distinguish active and completed quests. Unknown entrances are never marked or targeted, and quests do not reveal unexplored map areas. This is objective tracking for actual placed guardian encounters, **not** a port of Godot's full authored quest narrative, NPC dialogue, quest item chains, rewards or wayfinding. Gameplay and save/reload remain unverified in native Unreal.

## Dungeon quest chains and permanent power rewards
Every discovered dungeon quest now shows the **four resident takedowns** at chambers 3, 5, 9 and 12, followed by its existing chamber-14 guardian. The stages are derived from the actual, persistent defeated-encounter IDs, not a wave counter or arbitrary quest timer. Guardian progress is independent from resident completion, so previously defeated guardians remain correctly completed in old saves.

Every uniquely defeated guardian adds **+3 maximum mana and +3 maximum stamina** (up to +90 of each from all 30 dungeons) to the active hero. The bonus is re-derived from the persistent defeated guardian set whenever a pawn is created, and an in-memory applied-count guard prevents repeatedly applying it during world ticks. It **never heals health, restores current mana/stamina or awards duplicate XP/loot on kill**. Existing Godot and Unreal save formats remain unchanged; no extra reward claim file is required. No NPC dialog or authored story quests are implied.

Native UE 5.4 compilation and live save/restart and character lifecycle tests remain required. The bonus is sourced from the same dungeon guardian-cleared IDs already persisted by the prototype.

## Regional quest guides and dialogue interactions
Six stationary, non-hostile placeholder quest guides now spawn near region centers without ticking AI or combat collisions. When close, the existing HUD shows an interaction prompt. **R** (keyboard) or **D-pad Right** (controller) selects the next discovered quest entrance in that guide's region, prioritizing unfinished objectives; guides cannot reveal or select undiscovered destinations. Interaction is blocked during menus and player defeat. A short four-second dialogue line identifies the region and confirms the known quest or asks the player to explore an entrance first. The Blueprint event `OnQuestGuideSpoken` allows authored voice, text and animations later.

These are six visible **cylinder placeholder landmarks**, not finished 3D NPC models, voiced conversations, branching dialogue trees, or authored story quests. The existing guardian quests and permanent resource-capacity rewards remain unchanged. Source tests now cover guide count/behavior, map fog gates, key/controller input and dialogue. Actual UE 5.4 compilation, controller testing, and quest selection in play remain pending.

## Regional quest guide presentation upgrade
The six native guide placeholders are now multi-part humanoid silhouettes (torso, sphere head, two arms) rather than plain pillars. They retain collision-free bodies and no AI tick. Their names are Ash Warden, Mire Keeper, Frost Herald, Storm Oracle, Foundry Chronicler, and Eclipse Seer. When interacted with, each rotates toward the player and the HUD takes its title from the actual guide instance rather than duplicating the name roster.

Guide dialogue now references grounded quest progress: undiscovered entrance, resident count (0–4), ready guardian encounter, or already-liberated dungeon. It still does not teleport, reveal hidden map locations, award unsupported loot or claim NPC voice/skeletal animation is implemented. Existing interaction remains R / gamepad D-pad Right; the Blueprint dialogue hook remains available. Native UE compile and playtest remain required.

## Three-way NPC AI prototype: social memory, civilian routines and consequence
This is an **original Moonfall implementation** combining broad design inspirations from Fable III (reputation, reactions), Assassin's Creed (ambient local routines) and The Blood of Dawnwalker (choice consequences). It does **not** copy any game's models, scripts, dialogue or proprietary AI.

- **Social memory**: Each of the six region guides tracks persisted trust (−100 to +100) and fear (0 to 100). Support (G / controller D-pad Up) increases trust by 7 and lowers fear by 4; intimidate (H / D-pad Down) reduces trust by 10 and raises fear by 12. Each guide has a 30-second local cooldown, interaction distance 380 UU and menu/death guards. Dialogue responds to trust/fear and the latest action. These extra save arrays are optional, clamped and backward compatible with older Unreal v1 saves.
- **Actual deeds**: A guide's displayed trust also increases +8 for each of the five real defeated guardians in their region. Defeat IDs are the same persisted combat victories already used by the Raven Atlas and quest system, never a fabricated NPC reputation quest.
- **Local routines**: Six guide actors each run a half-second rate-limited scheduler only near the hero, patrolling between three small watch/market/camp waypoints with swept world-static collision. Regional phase offsets avoid all guides moving in unison. This is **not** a full dynamic day/night simulation or populated-city navigation AI. World exploration does not advance a timed story deadline.
- **Original gameplay**: No waves, new combat AI controllers, artificial companion relationships, or hidden map discoveries. Existing R / D-pad Right quest-guide dialogue and selection remains. Z and Nyra's source artwork and Godot sources stay unchanged.

**Needed before release:** UE 5.4 native compile and in-editor playtest, test controller D-pad input vs menu navigation, NPC path validity around authored terrain, confirm saves across restarts and profile dynamic light cost. Dialogue lines are rule-based templates, not LLM-generated conversations or authored voiced performances.

## NPC population expansion and role-aware behavior
The six-region prototype now spawns **18 individually named civilian NPCs** (three per region: quest guide, artisan and scout) with original role-specific temperaments. They each receive a stable NPC ID `RegionIndex * 3 + RoleIndex`. Lightweight ambient routines retain half-second proximity gating and use role-offset schedules. Quest guides alone select discovered dungeon objectives; artisans and scouts share regional observations without rewriting the active quest.

Each person has distinct trust/fear memory in optional, bounded 18-slot Unreal SaveGame arrays. Existing regional NPC memories are migrated to the original guide identities when older saves lack per-person arrays, and older fields stay populated. Genuine local guardian victories contribute role-sensitive reputation (+8 for guides, +5 for artisans, +10 for scouts per guardian). Social choices have personal cooldowns and role-dependent reactions; bullying scouts builds less fear but hurts trust more, while artisans particularly value respectful conversation. Dialogue displays a speaker's role-specific reaction, personality and current schedule.

This is a local rule-based **prototype**, not a fully simulated town, emergent language model, authored relationships between all NPCs, or new artwork. It preserves Godot, its saves, and original Z/Nyra images. Native UE 5.4 compilation, reload and navigation playtests are outstanding.

## Faction and NPC-to-NPC relationship increment
Each region's three civilian NPCs now have deterministic **trusting guide–artisan** (+60), **guide–scout** (+35), and **artisan–scout rivalry** (−25) relationships, with bonds symmetric and confined to their own region. A social choice changes the target's persistent trust/fear and sends a modest one-hop gossip adjustment to the two other local NPCs: allies respond positively to kindness and negatively to threats, while rivals can respond in the opposite direction. Gossip is bounded, applies only once per actual social action (not recursively), and uses the existing 30-second personal interaction cooldown. The old six-guide SaveGame trust field is kept synchronized for compatibility.

Each region exposes a bounded faction standing in [−100, +100] based on all three residents' individual trust, real guardian-victory reputation and half-weighted fear. The existing NPC interaction HUD now displays local faction standing and guide bond, with distinct reactions for strongly supportive or hostile communities. These changes do not fabricate regional quest completion or reveal undiscovered map coordinates.

**Limitations:** Rule-based local relationships, not a comprehensive family tree, simulated economic system, or autonomous NPC conversations; native UE 5.4 compile, multiplayer authority, restart save verification and playtesting remain necessary. Godot and original Z/Nyra art are untouched.

## Fable-inspired living population: occupational shifts and interaction reactions
The 18 civilian prototypes now share a compact local **eight-minute day cycle** split into four 120-second shifts. Guide, artisan and scout roles map those shifts into deterministic work, market, watch, patrol, rest and night-watch activities. Each NPC moves between a small set of nearby waypoints using a half-second, hero-proximity-gated update and collision sweeps; no AI controller, citywide navigation graph or new actor spawning is required. This is a simulation clock, not real-time day/night sky and lighting. NPCs do not advance or time out quests.

During conversations, the NPC pauses movement for five seconds and uses existing primitive arm meshes for a simple trust/fear-dependent greeting or wary stance. Disposition is derived from existing persistent individual relationships and regional deeds. These are placeholder poses, **not skeletal animation or authored speech**. The existing social-choice, quest, fog and save contracts remain unchanged.

Verification required before playable Unreal release: native Unreal 5.4 C++ Editor compile, in-game shifts and wall collision checks, 18-NPC perf profile, animated asset authoring, controller input, save/restart and Windows packaging.

## Virtual population streaming: 1,111 persistent civilian identities
The Unreal prototype now represents **1,111 stable civilian IDs**, not 1,111 simultaneously spawned actors. The 18 existing named guides, artisans and scouts retain IDs 0–17 and remain authored in the original six regions. The additional **1,093** numbered residents have deterministic regional positions, roles, names, personality labels and per-person trust/fear records. Their locations are derived from IDs rather than stored as full actor transforms.

A quarter-second arena scheduler examines up to **80 records** per pass, spawning at most **two** residents per pass within 1,200 Unreal units of the player. At most **24 additional resident actors** are active at a time; moving beyond 1,650 units destroys their actor representations, but keeps their stable identity and save-backed social relationship. The original 18 authored actors remain separate. No actor is streamed when there is no valid player pawn. New residents participate in existing local conversations and support/intimidate actions with 30-second interaction cooldowns; their save arrays are optional and validated on load, so older Unreal v1 saves retain historical exploration, guardian progress, and original named NPC relationships.

**Important limitations:** This is a scalable population and actor-streaming foundation, not 1,111 fully authored character models, independent careers, dynamic real-estate ownership, permanent actor-world-position storage or an in-game verified population count. Residents use generated identities and primitive NPC meshes, and their short local waypoints reset to deterministic home locations after streaming. The system still needs native Unreal compilation, performance profiling, NPC collision/navigation testing, relationship save/restart testing, controller validation and packaged Windows testing. Godot and the original Z/Nyra artwork remain untouched.

## NPC economy: role-based Salvage services and region supply
Artisans (role 1) offer **35 mana recovery** and scouts (role 2) **35 stamina recovery** at their current local price. Their services use the existing earned `Salvage` inventory resource from dismantled loot; currency is deducted only after a live nearby NPC is found, the player is alive and out of menus, sufficient salvage is held, and the requested resource is actually missing. No health-on-kill, automatic loot, XP, permanent stat upgrades or infinite free refill are introduced. Keyboard **T** or gamepad **D-pad Left** purchases a nearby artisan/scout service, and the existing on-screen NPC dialogue displays quotes and success. Guides cannot be traded with.

Each of the six regions has a bounded supply rating (0–100) derived from *real* defeated guardians and positive faction standing. Service prices are deterministic 4–25 salvage, increasing with negative regional standing and decreasing as supplies improve. This is a small first step toward role-based towns, not a full NPC production/consumption simulation, merchant stock exchange, persistent player currency system or independently simulated economy. Player Salvage itself is currently held by the existing hero pawn and is **not added to the Unreal world SaveGame** here, so persistence across process restart needs its own integrated player-save treatment.

Source regressions check actual currency spending, minimum/maximum price bounds, no health/XP/loot abuse, input availability and legacy world-save compatibility. Native UE 5.4 Editor compilation, player money/restart and controller interactions remain to verify. Original Godot sources, saves and the Z/Nyra artwork are unmodified.

## Crafting and workshop production increment
Artisan NPCs now offer a **Salvage-funded uncommon equipment recipe** through the existing item-code inventory system. The crafted item slot rotates deterministically by region (weapon, armor or accessory) and its level tier follows the hero's actual level, bounded to the existing valid 1–25 tier range. Crafted gear is placed in inventory and **never auto-equipped**. No enemy loot table, health-on-kill, XP or new external inventory system is introduced.

Each discovered region has **finite workshop inventory**, initialized to 3 units and saved as an optional six-element array in Unreal's existing SaveGame. A capped, at-most-one-unit production update occurs every 120 seconds, conditioned on regional discovery and with production capacity based on actual local supply rating. Workshop stock never goes negative and loads with a strict 0–8 clamp; legacy slots without this field retain the initial stock. Crafting prices remain positive and bounded (7–20 Salvage), influenced by local supply and faction reputation. The transaction checks proximity, actual artisan role, current stock, available Salvage, living hero and closed menus, then decrements stock, spends Salvage and adds one valid item. This is an initial supply chain, **not** a complete simulated merchant market.

Controls: **V** near an artisan (controller face-top outside inventory). The prior C DualCast, gamepad face-left loot and inventory bindings are preserved. Source contract tests verify item code validation ranges, stock limits and the absence of free healing, XP or auto-equipping. Native UE5 compile, transaction playtests, hero Salvage persistence across restart and Windows packaging are still required.

## Valhalla-inspired four-tab Moonfall journal
The Unreal Canvas inventory has a new **Gear / Character / Quests / World** page hierarchy using Moonfall's existing dark fantasy palette, no copied Assassin's Creed assets. **I** opens Gear; **K** opens Character, **J** opens Quests and **M** opens World directly. While the menu is open, **A/D**, left/right arrows or controller D-pad left/right switch pages. **W/S**, up/down arrows or D-pad up/down select gear entries; **Enter/controller face-top** equips and **Delete/controller face-left** salvages only while Gear is selected. **Esc/Start** closes the menu. Existing graphics/accessibility options remain separate, and combat/movement is blocked while the journal is open.

Character displays live level, XP, Health, Mana, Stamina, spell power, three equipped item tiers, and Salvage. Quests lists only **discovered** dungeon objectives and real resident/guardian progress, highlighting the tracked quest. The expanded Raven Atlas draws only actually discovered regions, dungeon entrances and chambers, plus the player marker, leaving hidden areas blank. No extra HUD actor or proprietary Assassin's Creed graphic was added.

**Limitations:** This is a four-page source-level Canvas journal rather than a polished UMG animated production UI. Native Unreal compilation, controller interaction/playtests, small-screen readability and visual asset production are outstanding. Original Godot content and Z/Nyra artwork are unchanged.

## Five-system integrated overhaul: population, relationships, economy, crafting, menus
1. **Player economy persistence**: A separate Unreal-only `UMoonfallPlayerEconomySave` SaveGame at slot `MoonfallUnrealPlayerEconomyV1` holds Salvage, validated inventory item codes (up to 2,048) and worn gear. Load runs on hero BeginPlay. Successful pickup, scripted award, salvage, equip, merchant services and crafting save these values. Equipped codes must match a genuinely owned valid bag item before being restored. Equipped armor restores its maximum-health capacity without an extra health refill. This saves **economy and equipped gear**, not full character levels, position or complete combat state. The existing world save and Godot saves are unchanged.
2. **Generated NPC social participation**: Every one of the 1,093 virtual residents can affect two immediate same-region neighbors through capped, single-hop word of mouth. Their actual non-neutral social histories contribute 30% of region faction standing, while the original named NPCs hold 70%. No recursive gossip, full 1,111 actor updates or cross-region diffusion.
3. **Real regional crafting variation**: Six region-driven equipment recipes vary by slot and affix. A verified higher regional supply rating (65+) unlocks rare crafting quality; otherwise the result is uncommon. All outputs use legal existing inventory item codes, require stock and Salvage, and are never automatically equipped.
4. **Journal quest selection**: Quest page now highlights a selectable discovered quest, supports up/down and Enter/Y to track it, and pages through up to all 30 known dungeon objectives in groups of nine. The atlas and journal still conceal undiscovered entrances and chambers.
5. **Regression and integration hardening**: New tests cover SaveGame item validation, equipment capacity restoration, gossip and faction weighting, recipes and journal pagination. No duplicated UI actor, wave system or new external art.

**Release boundary**: This is a committed source-level integration in the draft Unreal PR. Native UE 5.4 C++ compilation, in-game SaveGame restart behavior, UI/controller interaction, collision, frame-time profiling and Windows installer are not verified. World stock and player inventory use independent SaveGame slots and are not yet an atomic transaction across a crash.

## Native Win64 build entry point and inventory overflow guard
A native build verification script is available as `MoonfallUnreal/Verify-Win64.ps1`. From Windows PowerShell, with Unreal Engine 5.4 and MSVC/Windows SDK already installed, run:

```powershell
.\Verify-Win64.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.4" -BuildGame
```

The script checks `Build.version` for UE **5.4**, invokes the actual Unreal Build Tool via `Build.bat` for the Editor target and optionally the Win64 Development game target, and fails explicitly on native compiler errors. It **does not** package an installer or perform gameplay testing. GitHub's existing ordinary Actions are still **source-contract checks only** because they don't have a local Unreal installation.

The inventory cap is now enforced **before** loot pickups, scripted direct awards, or crafting consumes workshop stock or Salvage. This closes a save-data loss path: the economy slot can serialize at most 2,048 validated item codes, so the player cannot accumulate unsavable additional items. Ground loot can remain uncollected when the bag is full. Existing equipped-item salvage protections remain active.

**Remaining release blockers:** Native build has not yet run, save/stock updates use separate slots rather than one crash-atomic transaction, all real Windows gameplay controls/UI still need hands-on tests, and the current Unreal art is primitive placeholders. Godot and original Z/Nyra artwork remain untouched.

## Windows readiness follow-up: single Arena ownership
The game mode now detects an Arena actor already placed in the level before spawning its own default `AMoonfallArena`. This avoids accidentally creating two world simulation coordinators, duplicate enemy/NPC actors and competing saves under the same progress slot when a designer-authored level includes its own Arena. The continuous-world Arena also explicitly includes `EngineUtils.h` and `GameFramework/Pawn.h`, ensuring actor iteration and pawn types have direct compile dependencies rather than relying on other headers to pull them in. A source regression test checks the startup guard.

The native `Verify-Win64.ps1` verification command remains available on a Windows machine with a real Unreal 5.4 installation. GitHub CI here validates source contracts, **not** Unreal Engine compilation or Windows packaging.

## Crafting save-order fix
The artisan purchase path now writes **decremented workshop stock to the Unreal world save slot before giving the hero an item or spending Salvage**. If the world save fails, it reverses the in-memory decrement and rejects the transaction. This prevents a stale workshop stock save from allowing duplicate purchases across a restart when the hero's item was already saved. Source regressions enforce stock save, save-failure rollback and craft-step ordering.

**Caution:** The world save and the independent player economy save remain separate files. The change does not guarantee true two-slot atomic transactions: an interruption or player save failure after the workshop save can still leave mismatched inventory/stock. Full crash consistency needs a transactional journal or a unified save workflow and native runtime/restart tests.

## Failed hero SaveGame compensation during crafting
Artisan purchases now check the synchronous result of writing the player-economy SaveGame. Successful workshop stock persistence is required first. If saving the hero's newly crafted item and Salvage balance fails, the crafted item is removed from the active inventory, the previous Salvage balance is restored, and a bounded workshop stock refund is persisted using the world SaveGame. The UI only reports crafting success once the player save succeeds. Existing other callers retain the `SavePlayerEconomy()` convenience wrapper, while the purchase path uses `TrySavePlayerEconomy()` to check the actual result.

This is *best-effort compensation*, **not** a crash-atomic two-slot transaction. A process crash between slot writes or failed stock-refund persistence may still cause mismatch, and normal equipment/Salvage changes outside crafting do not yet propagate save-failure feedback. Native UE 5.4 compilation, Windows runtime failure injection and restart tests remain required.

## Latest integration pass: save-result-aware NPC services and crafting
- Artisan gear purchases require a completed workshop stock save followed by a successful hero inventory/Salvage save; on hero-save failure the new item and Salvage deduction are rolled back in memory and a bounded workshop refund is attempted and saved.
- Scout/artisan **mana or stamina services** now also check hero-save success before restoring resources. On failure, the Salvage deduction is rolled back and no service is reported as successful.
- Regression tests verify conditional save outcomes and ensure loot tests inspect only loot code, rather than mistaking the unrelated craft rollback as automated item deletion.

This is still best-effort recovery, not a true atomic transaction across two SaveGame slots. The rare interruption between slot writes, and workshop refund write failures, remain unhandled without a transaction journal. Unreal 5.4 native compilation and Windows gameplay tests have not been run here.

## NPC interaction identity bug fix
Social actions (encourage/intimidate) now refresh the actually targeted character's role, personality and current routine **before** calculating service-price feedback. Previously, the last-spoken NPC role could leak into a different character's dialogue, causing incorrect mana/stamina quotes. The original guide identity now shows the guide-to-artisan bond rather than asking for a self-bond of zero; artisan and scout relations to the guide are unchanged. Regression assertions cover both talk and social action paths.

No native Unreal compilation or in-engine controller/dialogue playtest has been performed for this fix; only source checks are available in the repository.

## Quest journal tracked-selection repair
Opening **J: Quests** now selects the current tracked *discovered* dungeon quest using the same ascending dungeon ID ordering as quest confirmation, instead of retaining a stale selection from another objective. If the tracked quest is unavailable or no quest is discovered, selection safely resets to the first available entry or zero. Confirming a quest also clamps the selected index to the current discovered count. Existing fog-of-war access checks, Enter/Y tracking and nine-entry pagination stay unchanged. Source regression coverage checks both menu entry and confirmed-selection paths. Native UE and controller verification are still pending.

## Ground loot loss on failed inventory save
Picking up a ground equipment actor no longer destroys the pickup unless `TrySavePlayerEconomy()` successfully writes the newly added inventory item. On a save failure, the added item is removed from the in-memory bag and the pickup returns false, retaining the world actor for another attempt. Last-loot UI metadata is updated only after a successful save. The existing maximum-inventory guard, item-code validation and no-auto-equip rules still apply. Source regression tests check the save/rollback/destroy ordering.

This does not persist ground pickup actors through a process restart or guarantee atomic state across world and player SaveGame files. Native Unreal 5.4 compilation and live disk-failure tests remain unverified.

## Inventory equipment and salvage SaveGame failure recovery
- **Equipping** now checks the result of `TrySavePlayerEconomy()` and, on failed write, restores the previously equipped tier, affix and item code, aggregate equipment tier, maximum health and current health. Health UI events are fired only after a successful armor equip save, preventing a false equipment update.
- **Salvaging** now keeps a copy of the original Salvage balance and the removed item. On a failed write the exact item is inserted back at its original inventory index, Salvage returns to its previous balance, and the operation reports failure. Inventory selection changes only after a successful save.
- Added source regressions for both rollback paths. These avoid reporting successful operations while the persistent economy file still contains the previous inventory state.

Not a native UE5.4 compile or injected save-failure gameplay test. The independent world/player slots remain non-atomic across process crashes.
