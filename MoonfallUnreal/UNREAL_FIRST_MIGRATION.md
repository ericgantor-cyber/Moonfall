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
