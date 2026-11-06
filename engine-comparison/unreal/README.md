# Moonfall Unreal Engine 5.6 isolated RPG experiment

This source prototype was merged into the repository for comparison, not promoted to the production Godot game. It includes native C++ combat, continuous regions and chambers, an experimental minimap, boss phases, progression, and preserved imported art references. **No Unreal editor compilation, Windows packaged build, or gameplay benchmark has been verified.** The original Z and Nyra files must not be modified.

## Verify Win64 compilation (on a Windows development PC)

Prerequisites: a compatible Unreal Engine 5.6 installation, Visual Studio C++ workload and Windows SDK. In PowerShell from the repository root:

```powershell
& ".\engine-comparison\unreal\verify-windows-build.ps1" -EngineRoot "C:\Program Files\Epic Games\UE_5.6"
```

The script invokes Unreal's `Build.bat` to compile the `MoonfallPrototypeEditor` and `MoonfallPrototype` Win64 Development targets, checks exit codes, and requires the generated game executable to exist. A missing engine or failed compile returns an error. **The script has not been run with Unreal in this environment.** A passing source-only CI job does not mean the C++ target compiled.

## Packaging blocker

The repository currently has **no committed `.umap` level**, so there is no verified playable starting map. Unreal Editor 5.6's Python API can create a blank startup level with the project's PythonScriptPlugin enabled. The experimental game mode constructs the runtime prototype world.

On a Windows PC with Unreal Engine 5.6 installed, run the optional bootstrap once:

```powershell
& ".\\engine-comparison\\unreal\\verify-windows-build.ps1" -EngineRoot "C:\\Program Files\\Epic Games\\UE_5.6" -CreateMap
```

This launches Unreal Editor to create `Content/Maps/MoonfallPrototype.umap` if absent, then checks it exists. It never overwrites an existing level. The startup map path is now configured in `Config/DefaultEngine.ini`. **The map-creation command has not yet been run or validated in an actual Unreal Editor.** Before release, manually verify `MoonfallGameMode` spawns the intended pawn and save the resulting `.umap` level. Only then request an optional package verification:

```powershell
& ".\engine-comparison\unreal\verify-windows-build.ps1" -EngineRoot "C:\Program Files\Epic Games\UE_5.6" -Package
```

Packaging uses Unreal's `RunUAT.bat BuildCookRun` for Win64 Development, cooking, staging, and archiving. Every invocation writes into its own uniquely named `run-*` archive subfolder to prevent older packages from being mistaken for fresh outputs, even when Unreal preserves file modification times. The verifier also requires both a valid Win64 PE executable and nonempty cooked Unreal content (`.pak` or IoStore `.ucas`) in that isolated archive; this still does not replace launch testing. If no `.umap` exists, the script **fails explicitly** instead of claiming a playable build. A successful package still needs a manual Windows 10/11 launch and gameplay check.

## Enemy combat overhaul (source implemented, gameplay unverified)

Ordinary enemies no longer deal untelegraphed proximity damage. An engaged enemy must first begin a visible warning (0.6 seconds for ordinary enemies, 0.85 seconds for guardians), then finish its windup while the hero remains in strike range. A pulse on the existing mesh or portrait indicates danger without replacing imported Z/Nyra artwork or adding VFX actors. Attacks have recovery (1.3 seconds for ordinary enemies, 1.7 seconds for guardians). A landed finisher or riposte interrupts the windup, and a correctly timed dodge can award the existing counterattack window. These behaviors are covered by source-contract tests, **not yet tested in a running Unreal build**.

## Floating combat feedback (source implemented, gameplay unverified)

Melee, arcane bolt and boss hits now emit temporary damage numbers anchored to the target's position. Heavy finisher/riposte hits are larger and gold; magic hits are blue; ordinary strikes are white. Numbers drift upward and fade over 1.25 seconds, with a strict cap of 24 active entries to avoid allocations growing during long battles. The existing orb HUD, minimap and original Z/Nyra art are untouched. Rendering and timing are covered by source tests, but **visual clarity, frame pacing and actual packaged gameplay still require Unreal Editor testing**.

## Click-to-resume after defeat (source implemented, gameplay unverified)

When health reaches zero, the existing left-click attack input reloads the current Unreal level. The existing isolated save slot restores the most recent **living** checkpoint, including location, health, mana, stamina, XP and previously saved exploration progress. A guard prevents repeated reload requests. No new keyboard shortcut, on-screen attack button, game mode, passive kill healing, or changes to Godot are introduced. This needs actual Unreal Editor testing to confirm a map reload and checkpoint restoration.

## Upgraded checkpoint recovery (source implemented, gameplay unverified)

In addition to the regular sixteen-second living checkpoint cycle, moving into a new connected world region now triggers an immediate checkpoint save. Reloading a checkpoint starts tracking from the restored region rather than saving repeatedly on the first frame. On defeat, the HUD temporarily shows a "Left-click to return to your last checkpoint" prompt and the existing click input reloads the level; it adds no permanent HUD controls. Saves preserve the hero's living checkpoint even when level transitions follow defeat. Actual disk write success, level reload and save restoration still need UE 5.6 gameplay testing.

## Checkpoint restore protection (source implemented, gameplay unverified)

Loading a valid living checkpoint now grants a short two-second protection window against both enemy and boss damage. The timer expires normally, so no permanent invulnerability is possible. The hero keeps the checkpoint's actual health and resources, with no healing or free perfect-dodge counterattack. Fresh runs without a restored checkpoint receive no protection. This prevents immediate repeat deaths when an old checkpoint is near an active guardian; its timing and behavior still require an Unreal 5.6 gameplay test.

## Enemy telegraph stability (source implemented, gameplay unverified)

Enemy strike warnings now hold the enemy at its current location rather than letting the return-home logic slide it away during a windup. Escaping the enemy's aggro/leash range cancels the warning and adds a short half-second recovery, preventing instant warning restarts on reentry. This also applies to guardian encounters, with no additional visual assets, wave spawns, or game modes. Real Unreal gameplay testing remains outstanding.

## Incoming damage and dodge feedback (source implemented, gameplay unverified)

The same bounded, temporary floating-number system now reports hero damage taken in red and successful timed dodges as a brief moonlit crescent with drifting blood-red streaks (an original, lightweight cinematic effect informed by Wukong's evasive afterimages and Dawnwalker's grounded duels). This covers both local enemy strikes and the three-phase boss's hits, without adding screen-filling HUD widgets or new game actors. It records only damage actually taken and actual successful dodge evasions; restored-checkpoint protection does not display a false hit. The previous gold critical / blue magic hit labels and the sixteen-second autosave are unchanged. Actual visual readability, projection and frame pacing still need Unreal 5.6 testing.

## Unified enemy and boss attack warnings (source implemented, runtime unverified)

Combat warnings now combine recognizable windup timing with a directional chevron, drawing on the readability of *Black Myth: Wukong* and the directional attack indicators of *The Blood of Dawnwalker*. An amber diamond and arrow near the attacking enemy indicate its incoming strike direction; the marker becomes crimson during the last 28% of the warning. The boss uses a larger indicator pointing toward its chosen impact location while the original world-space hazard decal remains active. Markers disappear immediately when a windup is interrupted, the enemy disengages, or the boss stops casting. This adds no new controls, imported artwork, or permanent HUD panel. The HUD implementation is source-test verified only and must be checked visually in an Unreal 5.6 gameplay build.

## Directional blocking, timed parries and perfect dodges (source implemented, gameplay unverified)

Combat now supports three complementary defensive choices in the isolated Unreal prototype: hold **right mouse button** to guard, steer the protected 120-degree frontal cone using **WASD**, and tap the same right mouse button within 0.22 seconds of a strike for a perfectly timed parry. A maintained guard reduces frontal damage by 75% but spends stamina (at least 12 per blocked strike); exhausted stamina causes a short 1.1-second guard break. An accurate parry prevents the hit, earns a timed riposte, and staggers a normal enemy or guardian, while still requiring frontal alignment. **Left Shift + movement** retains the original perfect dodge and cancels guarding instead of stacking invulnerability. Boss radial area attacks intentionally bypass guards and parries; dodge or leave the hazard. Guard feedback uses a blue shield outline, silver parry flash, and fractured red break cue, separate from the moonlit dodge crescent. No original character art, permanent HUD modes, Godot files, or 16-second autosave behavior were changed. Actual controller feel, facing, timing, physics and engine compilation still need Unreal Engine 5.6 tests.

## Defensive combat fixes and visual clarity (source implemented, runtime unverified)

Guard now faces the nearest available enemy when starting from a standstill; moving with WASD continues to steer the facing direction. A new 0.45-second parry recovery prevents repeated right-click taps from refreshing the 0.22-second perfect-parry window, while held right mouse still blocks normally. Parry timing is sampled before frame decay to protect the final valid input frame during frame spikes. The HUD now shows a small direction arrow only while guarding, with a brief silver emphasis during the live parry window. Boss radial attacks that cannot be blocked have a clearly crossed warning marker so players can choose to dodge instead. The 16-second autosave and both original character artworks remain unchanged. Source tests do not substitute for actual Unreal 5.6 compilation and gameplay validation.

## Guard-break combat lockout and stagger pulse fix (source implemented, gameplay unverified)

A broken guard now really imposes its short recovery: the hero cannot immediately attack, cast, heal or dodge until the guard-break timer expires. Guarding and offense no longer overlap: committing to a valid melee hit or spell lowers the guard. A successful parry still earns a separate riposte window, which can be used after lowering the guard. Staggering a warning enemy immediately resets its warning pulse instead of leaving an enlarged portrait on screen. Timings, controls, sixteen-second autosaves and original artwork are unchanged. Unreal gameplay/compile tests remain outstanding.

## Phantom Blade-inspired defensive pressure (source implemented, gameplay unverified)

Combining the general combat ideas discussed from *Phantom Blade Zero* (the supplied Medal clip itself was not playable here), ordinary enemies and chamber guardians now accumulate temporary posture pressure from landed strikes. Successful ripostes build pressure fastest, followed by combo finishers, ordinary melee, then magic. A full posture bar interrupts the current warning and creates a brief stagger (1.0 seconds ordinary enemies, 1.2 seconds guardians), with no health restoration or additional wave modes. Pressure begins decaying after 2.4 seconds without a hit, and the small meter only appears above nearby enemies who have accumulated pressure. A perfect parry speeds the next attack recovery to at most 0.12 seconds while maintaining the combo opportunity. Windup indicators are now blue/silver for attacks that can be parried, and consistently crimson with a crossed marker for dodge-only boss radial attacks. These are original Moonfall mechanics, not copied assets or footage. Actual Unreal 5.6 compilation and visual/gameplay verification remain pending.

## Manual acceptance checklist

- Compile editor and game C++ targets on Windows with UE 5.6 and record exact errors, warnings, and versions.
- Confirm a saved startup map, the custom game mode, continuous navigation, and player spawn.
- Test left-click attack, three-hit combos, arcane bolt/heal, Shift dodge, boss telegraphs, chase/leash/stagger and mouse-wheel zoom.
- Inspect the red HP, blue mana, purple XP HUD and minimap for readability, accessibility, and artwork fidelity.
- Save, quit, relaunch, and verify XP, checkpoint, guardian rewards, and boss progression.
- Package a Win64 build, launch on Windows 10/11, and record three repeatable frame-time/FPS/memory/load-time runs. Do not count editor preview results as shipping benchmarks.

The Godot game and original artwork remain the source of truth. Unreal gameplay verification remains pending until all steps above pass.
