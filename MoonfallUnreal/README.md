# Moonfall Unreal Engine 5 migration starter

Experimental parallel Unreal Engine 5.4 C++ project. This does **not** replace the Godot game or convert existing gameplay. Open the .uproject in Unreal Engine 5.4 with Visual Studio C++ tools installed; allow source compilation and create a level with PlayerStart. This has not yet been compiled or run in Unreal. Only third-person pawn camera/movement and Space/controller jump input are scaffolded. Combat, controller Options/Start, spells, bosses, crafting, and installer are **not ported**.

No Z/Nyra sprites or meshes are included or altered. Import the user's approved original designs unchanged before adding hero visuals. No WoW action bars. Never delete Moonfall3D until a validated feature-parity migration exists.

## Migration step 2: combat input scaffold

Added keyboard and Xbox-style controller bindings for Space/A jump, Q/B dodge, Z/LB left cast, X/RB right cast, and C dual cast. Six spell indices expose base mana costs and cooldowns and Blueprint event `OnSpellCast` to hook custom VFX and gameplay. Healing is prototyped for Restoration; other spells currently spend mana and fire a Blueprint event but **do not damage enemies**. Dual cast checks cooldowns for both hands. No dungeon map, enemies, UI, Options menu, crafted items or art has been ported yet. This Unreal starter **has not been built or run**. The complete Godot version remains available under `Moonfall3D/`.

## Migration step 3: experimental arena and spell hits

An `AMoonfallArena` actor is spawned by the game mode on BeginPlay and builds a floor and four perimeter walls using built-in cube meshes, plus five damageable enemy actors in a ring. `AMoonfallEnemy` has health, a visible placeholder mesh, and a callable damage method. Non-healing spells raycast along the third-person camera direction, consume mana on a valid hit, and apply initial spell damage. These enemies are stationary placeholders; they have no attacks, AI, loot, or death animation. Unreal Editor/C++ build and playable runtime have **not** been verified, and this is not a converted Godot map. The original Godot game and all Z/Nyra artwork remain untouched.

## Migration step 5: navigation targeting and feedback hooks

Enemy pursuit now optionally projects its target onto the Unreal navigation system, with direct swept movement fallback if a NavMesh is absent. This **is not pathfinding**: a NavMeshBoundsVolume and AIController/path-following implementation are still needed for navigating around obstacles. Spell hits show short-lived debug damage text, and attacks call `ApplyMoonfallDamage` on the hero, which emits `OnHealthChanged` for a future Blueprint HUD. Actual HUD, attack animation, and packaged-game VFX remain unimplemented. This Unreal source has not been compiled in Unreal Editor. Godot game, Z/Nyra original artwork and no-WoW-bar preference remain unchanged.

## Unreal build verification procedure

The GitHub `Unreal source structure checks` workflow only checks repository layout and rudimentary brace balance; it does **not** compile the C++ project. To actually validate, install Unreal Engine 5.4 and the Visual Studio C++ game-development workload on Windows, right-click `MoonfallUnreal.uproject` and generate project files, then open the generated solution and build the `MoonfallUnrealEditor` Development Editor Win64 target. Open the project in Unreal Editor, create a blank level containing a PlayerStart and a NavMeshBoundsVolume covering the arena, press P to inspect navigation, and Play to verify the pawn, HP/mana HUD, enemies and attack windup. The warning currently uses Unreal debug drawing and is not a finished telegraph. Record build failures from the compiler before claiming runtime readiness.

## Migration step 9: provisional spell impact feedback

Successful offensive spell hits now draw a brief school-colored impact sphere and casting line using Unreal's debug renderer. These are temporary in-editor combat readability effects, not packaged production VFX, Niagara particles, or animated spell assets. Restoration does not create an offensive hit effect. Unreal compilation and Windows runtime are still unverified; source layout checks are not compiler checks. Original Godot game and original Z/Nyra art are untouched.

## 4K graphics starter preset

`Config/DefaultGameUserSettings.ini` supplies a **3840×2160 borderless fullscreen preference** with 75% internal render scale, high textures/antialiasing, and medium shadow/reflection quality. These are starting preferences, not a locked resolution or proof that the Unreal project renders at 4K. Unreal may override them with saved per-user settings, desktop display restrictions, and hardware capabilities. `DefaultEngine.ini` enables temporal AA/upscaling defaults; debug-only spell impact shapes are **not** Niagara VFX yet. For 4K sharpness on a powerful GPU, increase resolution quality toward 100%; for performance reduce it. Actual Unreal build, 4K GPU testing, packaging and display settings UI remain outstanding. No original Z/Nyra art modified.

## Initial graphics controls

Press F6 to cycle Unreal's four overall scalability levels (low to epic), or F7 to toggle windowed/borderless fullscreen. Changes use `UGameUserSettings` and are saved. The HUD shows the shortcut legend. This is an initial keyboard-operated graphics control, **not** the promised full navigable graphics Options menu, controller-friendly settings, resolution picker or render-scale slider. The existing 4K default remains a preference, not a verified 4K build. Unreal Editor compilation and packaged Windows testing are outstanding.

## Real Unreal Windows compile workflow

`.github/workflows/unreal-windows-build.yml` is a **manual**, opt-in GitHub Actions workflow requiring a trusted, self-hosted Windows x64 runner labeled `unreal-5-4`. Install Unreal Engine 5.4 and the Visual Studio C++ development tools on that runner, set runner environment variable `UE54_ROOT` to the Unreal installation directory, and trigger **Actions > Moonfall Unreal Windows build (self-hosted) > Run workflow**. It compiles `MoonfallUnrealEditor Win64 Development` and fails on compiler errors. Ordinary GitHub-hosted runners do not include Unreal, so this workflow will remain queued unless you have the appropriately labeled self-hosted runner. Do not enable self-hosted execution for untrusted pull-request code. This is an editor compile check, not a packaged Windows installer, playtest, or hardware frame-generation validation. Vendor plugins are not downloaded or distributed.

## Windows packaged build workflow

The optional self-hosted Windows workflow now compiles the Unreal editor target, runs Unreal AutomationTool `BuildCookRun` for a Win64 Development game package, checks that `MoonfallUnreal.exe` exists in the archived output, and uploads the whole packaged directory as a short-lived GitHub Actions artifact. A successful run is **not** a tested standalone installer and does **not** prove correct gameplay or frame generation. The workflow needs a configured trusted Windows x64 runner labeled `unreal-5-4` with `UE54_ROOT`, Unreal 5.4 and Visual Studio build tools. The packaging job has not yet been run. Ensure Windows Defender and licensing allow the runner to build/package before triggering manually.

## Windows installer (NSIS)

The manual self-hosted Windows workflow now attempts an **NSIS 3 per-user installer** only after Unreal packaging succeeds and exactly one packaged `MoonfallUnreal.exe` is found. Install NSIS 3 on the trusted Windows build machine and add `makensis.exe` to `PATH`. The installer script lives at `Installer/MoonfallUnreal.nsi`; it copies the game package, creates desktop and Start Menu shortcuts, and supplies an uninstaller. The successful workflow uploads `MoonfallUnreal-Setup.exe` separately. **No successful run, installed game, or functional executable has been verified yet.** Test shortcut launching, dependencies, uninstall, permissions and real gameplay on Windows before distributing. The workflow requires Unreal 5.4, Visual Studio C++ build tools, NSIS 3, and a self-hosted runner.

## Major quality pass (performance and combat correctness)

Enemy AI now caches navigation waypoints and refreshes paths at most approximately twice per second per enemy, with small randomized staggering to reduce coincident path solves. Defeated enemies stop ticking and are destroyed after a short period. Opening Options prevents camera orbit as well as movement and spellcasting. School colors now use explicit color values rather than engine predefined constants. Added source regression assertions; they are not UE engine compilation or playtesting.

### Remaining roadmap

- UE 5.4 Windows editor/game compilation on configured Unreal runner, then packaged game and NSIS installation tests
- Runtime Niagara or comparable real effects replacing debug-only traces; enemy animation assets and multi-phase boss design
- Character source asset import only with approval and without altering Z or Nyra's original designs
- GPU performance measurements at 1080p/1440p/4K, accessibility and controller UI verification
- Compatible DLSS/FSR frame generation plugin install, capability detection, and Windows GPU testing; currently inactive

## Windows runtime prerequisites

The Unreal Windows `BuildCookRun` workflow now requests `-prereqs` so the packaged distribution includes Unreal's supported prerequisite redistributable installer when available. This is distinct from bundling the Unreal Editor or integrating GPU-specific DLSS/FSR plugins. Test VC++/DirectX prerequisite installation and launch on a clean Windows 10/11 machine before distribution; some prerequisites may still require installation or elevation. The real Unreal Win64 packaging workflow has not yet been executed successfully.

## Build locally on Windows without a GitHub runner

Install Unreal Engine 5.4, Visual Studio game-development/C++ build tools, .NET 8 SDK and NSIS 3 (with `makensis.exe` on PATH). From the repository root, run PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\MoonfallUnreal\Build\Build-Windows.ps1 -UnrealRoot 'C:\Program Files\Epic Games\UE_5.4'
```

The script fails early if prerequisites are absent, compiles the editor target, runs Unreal Windows packaging with prerequisites, builds the self-contained launcher, runs the launcher file check, and creates `MoonfallUnrealLocalBuild\MoonfallUnreal-Setup.exe`. **No Windows Unreal compile has been run here.** This does not remove the need for UE 5.4 on the build machine, nor replace manual game/installer tests. The finished installer is intended for end users without Unreal Editor.


## Generate the startup level with Unreal Editor

The project includes Build/Create-StartupLevel.py to create a blank MoonfallStart map with PlayerStart and a navigation bounds volume. It **must run inside Unreal Editor 5.4**, not regular Python or GitHub Actions. It refuses to overwrite a preexisting level.

1. Open MoonfallUnreal.uproject in Unreal Editor 5.4 and enable Python Editor Script Plugin if prompted, then restart the editor.
2. Open the Unreal Editor Python console using Tools > Debug > Python Console (menu location may vary).
3. Enter the following command, replacing the path with the actual checkout location:

```python
exec(open(r"C:\PATH\TO\Moonfall\MoonfallUnreal\Build\Create-StartupLevel.py", encoding="utf-8").read())
```

4. Confirm Content/Maps/MoonfallStart.umap was saved. Press P in the editor to inspect whether the navigation volume covers the arena. Resize or rebuild navigation as necessary; the script's volume size is provisional.
5. The generator updates GameDefaultMap and EditorStartupMap to /Game/Maps/MoonfallStart in Config/DefaultEngine.ini after the level saves, preserving unrelated settings and creating a .ini.before-moonfall-startup backup. Inspect the result before packaging.
6. Test Play in Editor and compile the Unreal C++ code on Windows before attempting a packaged game or installer. The generated map is a prototype test level, not the finished 3D Moonfall world.

**Status:** Only the editor script and its source checks have been committed. A binary .umap asset, Unreal compilation, and a Windows playable build remain unverified.


## Unreal Windows packaging: required startup level

The prototype currently defaults to the generic engine Entry map. Before packaging a playable Windows build:

1. Open MoonfallUnreal.uproject using Unreal Engine 5.4 and the Visual Studio C++ tools.
2. Create a Basic level in the Unreal Editor. Verify the C++ MoonfallUnrealGameMode is active; it spawns the prototype arena and player pawn.
3. Add PlayerStart and a NavMeshBoundsVolume covering the arena. Verify controls, collisions, combat, guard/parry, boss attacks and HUD in Play in Editor.
4. Save as MoonfallUnreal/Content/Maps/MoonfallStart.umap.
5. If you used the generator, it updates and backs up the two startup-map settings automatically after a successful save. If you created the map manually, set GameDefaultMap=/Game/Maps/MoonfallStart and EditorStartupMap=/Game/Maps/MoonfallStart under GameMapsSettings.
6. On a trusted UE 5.4 Windows machine, run Build-Windows.ps1 -CompileOnly to compile C++; run -PreflightOnly for packaging prerequisites and level configuration, then run the full script to package and build the installer.
7. Launch the packaged game and test the installer, gameplay and accessibility on Windows 10 and 11. A process-launch smoke test does not prove gameplay correctness.

Packaging deliberately stops with an actionable error if the project-owned startup level is missing. The map and a verified Unreal Windows build have not yet been created by this change.


## WoW-inspired Moonfall character creation (experimental, PR #930)

The new native `UMoonfallCharacterCreationWidget` builds a dark fantasy UMG
race-selection screen at runtime. This is an **original Moonfall interface**
influenced by World of Warcraft's strong central character preview and
separated race/detail panels, not a copy of Blizzard's UI or art.

**Layout:** Scrollable playable-race list at left, a prominent center image
area with previous/next and Continue buttons, and a right-hand race name,
family and Vitality/Power/Speed preview with family navigation. The UI is
intended for mouse, keyboard and controller. **No single-switch gameplay
controls are included.**

**Unreal Editor integration (not automated or native-compiled yet):**
1. Compile the UE5.4 C++ project with UMG, Slate and SlateCore enabled.
2. Create a Widget Blueprint derived from `MoonfallCharacterCreationWidget`.
3. Create/display that widget from the character-creation menu and give it
   keyboard/gamepad focus. The widget builds its layout in `NativeConstruct`
   when its WidgetTree has no existing root; do not add a competing root.
4. Optionally call `InitializeSelection(RaceId, BaseVitality, BasePower,
   BaseSpeed)` to seed the preview. Base values must be finite/nonnegative.
5. Implement `OnRacePreviewRequested(RaceId)` in the Widget Blueprint to
   select the game's **existing approved original** Moonfall character
   asset. Call `SetPreviewTexture(ExistingMoonfallTexture)` for a portrait.
   This code does not import or modify Z/Nyra's art and does not yet render
   a live 3D character viewport.
6. Bind `OnRaceConfirmed(RaceId)` in the owning Blueprint to the existing
   validated character creation workflow. This C++ widget does **not**
   spawn heroes, create saves, or assume race selection applies gameplay stats.

**Inputs:** Mouse-click a race; left-drag inside the central preview requests character rotation, and mouse-wheel over the preview requests zoom. Mouse-wheel outside the preview keeps normal scroll behavior in the race list. The owning Widget Blueprint must implement `OnPreviewRotateRequested(DeltaYaw)` and `OnPreviewZoomRequested(ZoomSteps)` to control the existing 3D preview actor/camera; the C++ widget only emits events, and it does not rotate or redraw Z/Nyra art. Left/Right arrows or D-pad cycle races;
Up/Down arrows or D-pad cycle playable race families; Enter/controller
south-face button confirms. These input routes need focus/gameplay testing
in Unreal Editor. Button focus or an input-mode change in the owning menu
could intercept keys. No 3D camera orbit, class grid, appearance editor or
name entry has been implemented yet.

**Optional direct 3D preview control:** Call `BindPreviewRig(PreviewPivot, PreviewArm)`
on the widget, supplying a dedicated menu-only `USceneComponent` pivot and
`USpringArmComponent` camera arm. Left drag now directly rotates that pivot
and wheel zoom changes spring arm distance in the 125-650 range. The **RESET
VIEW** button restores the initial pivot rotation and arm length. If no
component is bound, the widget still calls the Blueprint rotate/zoom events.
Never bind the live player pawn or its camera: these inputs are for a separate
character-preview actor only. A 3D preview actor, SceneCapture and render
target still must be set up by the owning Blueprint; this does not generate
or replace Z/Nyra art. The image-only fallback cannot physically rotate a
static portrait.

The implementation has Python **source-contract** regression checks.
A green source check is **not** proof of UE5.4 UHT/C++ compilation, packaged
Windows gameplay, or accessibility focus behavior.
