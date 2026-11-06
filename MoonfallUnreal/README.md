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

## Unreal mouse-wheel camera zoom

Mouse wheel up/down now moves the third-person boom in 50-unit steps, clamped between 250 and 850 Unreal units. It stays disabled while Options is open. Camera impact offset remains additive and is clamped within the same safe bounds; Reduce Motion still suppresses the impact impulse. The existing mouse-click spell controls, controller inputs and hero artwork are unchanged. Source-regression checks are wired into Unreal CI; **this has not yet been compiled or tested inside Unreal Engine**.

Camera distance transitions now interpolate smoothly instead of jumping instantly between zoom steps. Controller D-pad Up/Down also zooms during gameplay; when Options is open, those same buttons retain menu navigation because zoom handlers ignore input while the menu is open. Actual Unreal engine compilation and playtesting remain pending.

## Unreal local minimap scaffold

The existing combat HUD now draws a small north-up chart in its upper-right area on screens at least 700×460 pixels. It shows the player at the centre and up to 32 nearby living enemy dots within 22 metres, using different colors/sizes for regular enemies, elites, and bosses. The map is **local combat awareness only**, not a discovered-world map, quest atlas, cave map, or fog-of-war implementation. It adds no actors, new modes, assets, or periodic pathfinding. **Unreal compilation and live gameplay testing remain unverified.**

## Visited-area minimap trail (Unreal prototype)

The local minimap now remembers a bounded set of **384 visited 2-metre world cells** and draws their centers when near the player. Unvisited positions are not filled in as terrain, so unexplored areas remain visually empty. This is temporary HUD-session memory and intentionally does **not** pretend to be a saved exploration atlas or actual cave geometry; exploration persistence, room discovery and quest icons still need to be ported from the existing Godot game. Only the existing Unreal combat HUD changes; hero art and the Godot project remain untouched. UE 5.4 compilation and packaged Windows playtesting are still unverified.

## Independent minimap range controls

Press **+ / =** to zoom the combat minimap in, or **-** to zoom out. This changes the visible local-world radius in 4-metre steps, bounded between 12 and 48 metres; it does not alter the third-person camera's mouse-wheel zoom. The map displays its current radius below the chart. The controls are disabled while Options is open. Existing visited-cell trail and enemy markers retain their clipping to the selected range. Source tests are included, but the Unreal Editor compile and Windows runtime remain unverified.

## Unreal exploration persistence and quest target hooks

The local minimap now loads up to **384 visited two-metre cells** from `GGameUserSettingsIni` on HUD BeginPlay and saves changes using Unreal's config system at most once every 15 seconds, plus a final save on EndPlay. Corrupt or out-of-range saved coordinates are skipped. The trail is recorded even if the screen is too small to display the minimap. This is bounded local exploration data, not a complete terrain atlas or an unlimited permanent map.

Unreal gameplay/Blueprint code can call `SetQuestTarget(WorldLocation)` and `ClearQuestTarget()` on the hero. An explicitly assigned objective appears in gold on the minimap, clamped to its edge with a direction line when distant. No quest is fabricated and no undiscovered cave geometry is rendered. Actual quest assignment/completion logic and full-world atlas still need migration. **UE 5.4 C++ compilation and Windows runtime testing remain unverified.**

## First connected Unreal gameplay quest: arena boss

The existing Unreal arena boss now starts a **Defeat the Arena Boss** objective when spawned and when the player pawn is available. It points the existing minimap quest target toward that boss. Defeating the boss through the existing damage/death code completes the objective once, clears its waypoint, and displays a completion banner in the existing combat HUD. Quest progress is scoped to this prototype game session; it is **not** a general quest journal, a saved quest, a full world atlas, or a migrated dungeon quest system. A preexisting manually assigned quest target takes priority rather than being overwritten when the arena boss spawns. Source checks are present; **Unreal C++ compilation and runtime gameplay are not verified.**

## Boss quest progress in the Unreal HUD

The arena-boss quest tracker now displays the player's **distance to the assigned boss waypoint (metres)** and a **boss-health progress bar** while the objective is active. The readout uses the living boss already found by the combat HUD, avoiding another enemy scan. When the tracked boss is defeated, the quest completes and its green completion message disappears after 5 seconds rather than remaining over gameplay forever. Quest rewards, a general-purpose journal, and an Unreal world atlas are not implemented by this update. The latest changes require Unreal Editor compilation and actual Windows gameplay verification.

## Explored-world atlas overlay (Unreal migration)

Press **M** or the controller **View/Back** button to open/close a larger north-up world chart in the existing HUD. It draws only previously visited two-metre cells within a 150-metre player-centred view, a player marker and a deliberately assigned gold quest destination. Unknown space stays empty, **not** fabricated terrain. Explored cells remain bounded to the existing 384-cell config save; this is **not yet** Skyrim-style continuous terrain, caves, geometry scanning, or a complete WoW quest journal.

Exploration tracking now runs before the Options early return, so briefly viewing Options no longer interrupts visited-cell registration. The atlas is a HUD overlay rather than another gameplay mode. Existing camera zoom, combat HUD and original Z/Nyra art are unchanged. **UE 5.4 compilation, packaged Windows builds and runtime input behavior remain unverified.**

## World atlas input safety fix

While the M/View world atlas is open, player movement, camera-look inputs, spellcasting, dodge and nearby enemy AI are blocked to prevent combat underneath the map overlay. Closing the atlas resumes those actions. Opening Options closes the atlas first to prevent overlapping overlays; the atlas itself does not create a separate game mode. This is source-level behavior, not a verified pause of every game timer or physics simulation. Unreal Editor build and Windows gameplay validation remain outstanding.

## Large world-atlas zoom and quest-marker clipping

With the large world atlas open, **+ / =** zooms in and **-** zooms out in 25-metre steps, with a 50–500-metre player-centred viewing radius. With the atlas closed, the same keys retain the separate **12–48-metre minimap** zoom. Distant quest markers are constrained to the visible chart area rather than overlapping the title or escaping the panel. This is still a discovered-cell chart with bounded memory, **not** a rendered terrain map or fully ported quest journal. Unreal compilation and actual gameplay input tests remain unverified.

## Atlas panning and recenter

The M/View world atlas now pans in 20-metre steps with **WASD**, **arrow keys**, or **controller D-pad**, bounded to ±500 metres from the player's current position. Press **R** or the controller's **Y/Triangle** button to recenter on the player. The player marker, visited cells, and quest target now use the panned viewport correctly; player markers disappear when fully outside the visible chart. Map mode blocks game camera zoom so the D-pad cannot both pan the atlas and zoom the third-person camera. The small minimap and the separate world-atlas zoom still retain their own settings. These controls are committed as C++ source; **Unreal compile and Windows runtime input tests have not been performed.**

## Personal exploration waypoint pins

In the open world atlas, pan the map until the desired location is at its centre. Press **T** (controller left shoulder/LB) to place one green personal waypoint at that world position, or **Backspace** (controller right shoulder/RB) to clear it. The pin remains visible on the atlas and local minimap, independently of the gold quest objective, and is clipped to the chart's view range. These bindings reuse shoulder buttons that cast spells during normal gameplay; map mode already blocks casting, so the two actions do not conflict. Personal waypoints are now saved to the local Unreal game-user settings file when placed or cleared, and reloaded when the hero starts. Invalid saved coordinates are ignored. Only one personal pin is stored and it remains independent of quest destinations. Clicking to place markers, route pathfinding, and real terrain/cave geometry are not implemented yet. Unreal compilation and Windows playtests remain unverified.

## Personal waypoint persistence

The T/LB personal pin survives a normal restart via `Moonfall.Map` in `GGameUserSettingsIni`. The Backspace/RB clear action also writes the inactive state, preventing a deleted marker from returning on next start. Stored X/Y/Z are validated as finite and within bounded world coordinates before activation. This is local per-configuration persistence, not cloud saves or character-slot-specific progress. Source CI covers the persistence code; a UE 5.4 compilation and a live Windows restart test remain unverified.

## Mouse click placement on the exploration atlas

Open the large atlas with **M** or controller View, then **left-click inside the map** to place or move the saved green personal waypoint at that world location. The pointer is shown while the map is open and hidden again when it closes or Options opens. Header, footer and outside-map clicks are ignored; the screen-to-world conversion matches the existing panned, zoomed, north-up map projection. Left-click while the map is closed retains its existing attack mapping. **T/LB** still places a pin at the panned map centre and **Backspace/RB** clears it. The quest marker remains independent. Unreal Editor compilation, mouse capture behavior and Windows gameplay testing are unverified.

## Corrupted crow enemy mobs (Phantom Blade Zero combat reference)

The starter arena now spawns **two hostile airborne corrupted crow enemies**, inspired by the oversized crow enemies in Phantom Blade Zero but using original Moonfall behaviors and placeholder geometry, not copied game assets. These enemies circle over the player and now alternate **three clearly telegraphed moves**: a diving peck, a heavier talon sweep, and a delayed feint dive. Each uses a brief warning before a collision-checked single hit, then retreats into cooldown. The crow's visible placeholder silhouette now has separate flapping wings and a beak. Hitting a crow during its warning interrupts its attack. They use the normal Moonfall enemy health, damage, XP and loot systems; idle birds update less often at a distance and cannot attack while Options or the world atlas is open. The earlier **ambient bird flock has been removed** to avoid unnecessary duplicate code. These are prototype mobs, not polished crow art or animation. Unreal compilation, hit-testing, visual design and Windows playtesting are still unverified.

### Corrupted crow combat tuning

The two starter crows use a deterministic three-move sequence, a maximum 20 Hz nearby AI update, reduced distant update rate, and no new spawned combat actors. Telegraphs draw a short-lived warning ring at the targeted ground area. The swoop resolves at most one hit and explicitly permits contact with the player actor while rejecting occluded hits through walls. Standard combat XP and equipment rewards remain intact. Crow wings and beak are composed from built-in primitive meshes as prototype geometry; their final mesh, feathers, animation, dodge tuning, gameplay balance, engine compilation and Windows runtime validation remain outstanding.

## Crow and arena art foundation upgrade

The hostile corrupted crow prototype now has an articulated primitive silhouette with wings, tail, claws, beak and eyes, plus a short-range, non-shadow-casting red warning glow only during its attack. Original mesh and material assets can be plugged into `CrowBodyArt`, `CrowWingArt`, `CrowFeatherMaterial` and `CrowAccentMaterial` through an enemy Blueprint. A configurable `CrowEnemyClass` in the arena and `ArenaClass` in the GameMode let Unreal spawn those art-ready Blueprint subclasses without discarding the native fallback. The starter arena also has six non-colliding ruined stone pillars and two inexpensive colored lights.

See [Art integration guide](Art/ART_PIPELINE.md) for the detailed original hero-art preservation requirements, import workflow, missing art inventory, performance budgets and UE compilation test plan. **This source upgrade does not mean all Moonfall art is finished or imported; hero 3D models, polished crow rigging, world terrain and most other production assets are still pending.** Unreal Engine compilation and Windows runtime playtests have not been performed.

## Further environment art overhaul: instanced stone and ground rubble

The native prototype arena now uses one `UInstancedStaticMeshComponent` for non-colliding, non-shadow-casting ruined stone architecture, broken supports, and deterministic scatter rubble. `RubbleCount` defaults to 24 and is clamped to 0–48 for performance. Environment artists can replace the default cubes using `RuinMeshArt` and `RuinStoneMaterial` on the arena Blueprint without adding an actor per prop. The crow warning glow is cleared when gameplay is suppressed by the Options or atlas overlay. This is a source-level visual foundation, **not imported finished 3D scenery or creature textures**. Unreal compilation, GPU performance, materials, and Windows packaging remain unverified.

## Additional art upgrade: floor, walls, ordinary mobs and bosses

The native Unreal prototype now offers **separate authored mesh/material slots** for the arena ground (`GroundMeshArt` / `GroundMaterial`), arena boundary walls (`BoundaryMeshArt` / `BoundaryMaterial`), regular enemies (`RegularEnemyArt` / `RegularEnemyMaterial`), and bosses (`BossEnemyArt` / `BossEnemyMaterial`). These extend the existing corrupted crow and instanced ruin art paths without modifying combat hit handling or changing Z and Nyra's original images. When no original assets are assigned, default primitive meshes remain as fallback. See `Art/ART_PIPELINE.md` for asset preparation and Unreal import checks. **This does not mean finished game-wide art is imported or that UE 5.4 has compiled.**
