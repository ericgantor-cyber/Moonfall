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
