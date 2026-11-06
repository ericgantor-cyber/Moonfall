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

## Manual acceptance checklist

- Compile editor and game C++ targets on Windows with UE 5.6 and record exact errors, warnings, and versions.
- Confirm a saved startup map, the custom game mode, continuous navigation, and player spawn.
- Test left-click attack, three-hit combos, arcane bolt/heal, Shift dodge, boss telegraphs, chase/leash/stagger and mouse-wheel zoom.
- Inspect the red HP, blue mana, purple XP HUD and minimap for readability, accessibility, and artwork fidelity.
- Save, quit, relaunch, and verify XP, checkpoint, guardian rewards, and boss progression.
- Package a Win64 build, launch on Windows 10/11, and record three repeatable frame-time/FPS/memory/load-time runs. Do not count editor preview results as shipping benchmarks.

The Godot game and original artwork remain the source of truth. Unreal gameplay verification remains pending until all steps above pass.
