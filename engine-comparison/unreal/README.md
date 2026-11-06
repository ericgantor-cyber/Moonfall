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

The same bounded, temporary floating-number system now reports hero damage taken in red and successful timed dodges as cyan `DODGE` labels. This covers both local enemy strikes and the three-phase boss's hits, without adding screen-filling HUD widgets or new game actors. It records only damage actually taken and actual successful dodge evasions; restored-checkpoint protection does not display a false hit. The previous gold critical / blue magic hit labels and the sixteen-second autosave are unchanged. Actual visual readability, projection and frame pacing still need Unreal 5.6 testing.

## Manual acceptance checklist

- Compile editor and game C++ targets on Windows with UE 5.6 and record exact errors, warnings, and versions.
- Confirm a saved startup map, the custom game mode, continuous navigation, and player spawn.
- Test left-click attack, three-hit combos, arcane bolt/heal, Shift dodge, boss telegraphs, chase/leash/stagger and mouse-wheel zoom.
- Inspect the red HP, blue mana, purple XP HUD and minimap for readability, accessibility, and artwork fidelity.
- Save, quit, relaunch, and verify XP, checkpoint, guardian rewards, and boss progression.
- Package a Win64 build, launch on Windows 10/11, and record three repeatable frame-time/FPS/memory/load-time runs. Do not count editor preview results as shipping benchmarks.

The Godot game and original artwork remain the source of truth. Unreal gameplay verification remains pending until all steps above pass.
