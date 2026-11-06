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

Packaging uses Unreal's `RunUAT.bat BuildCookRun` for Win64 Development, cooking, staging, and archiving. If no `.umap` exists, the script **fails explicitly** instead of claiming a playable build. A successful package still needs a manual Windows 10/11 launch and gameplay check.

## Manual acceptance checklist

- Compile editor and game C++ targets on Windows with UE 5.6 and record exact errors, warnings, and versions.
- Confirm a saved startup map, the custom game mode, continuous navigation, and player spawn.
- Test left-click attack, three-hit combos, arcane bolt/heal, Shift dodge, boss telegraphs, chase/leash/stagger and mouse-wheel zoom.
- Inspect the red HP, blue mana, purple XP HUD and minimap for readability, accessibility, and artwork fidelity.
- Save, quit, relaunch, and verify XP, checkpoint, guardian rewards, and boss progression.
- Package a Win64 build, launch on Windows 10/11, and record three repeatable frame-time/FPS/memory/load-time runs. Do not count editor preview results as shipping benchmarks.

The Godot game and original artwork remain the source of truth. Unreal gameplay verification remains pending until all steps above pass.
