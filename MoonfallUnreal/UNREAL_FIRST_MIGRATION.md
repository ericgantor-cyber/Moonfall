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
