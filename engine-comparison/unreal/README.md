# Moonfall Unreal Engine 5 isolated combat experiment

Tracks #685. Source project skeleton now exists in `MoonfallPrototype/` (UE 5.6 association). It contains a native C++ pawn and game mode with primitive floor, hero and boss; the boss has an area warning followed by damage. **Uncompiled and untested:** no Unreal editor, Windows executable or gameplay benchmark has been run.

## Open and test
1. Install a compatible Unreal Engine 5.6 editor with C++ build tools and Windows SDK.
2. Open `MoonfallPrototype/MoonfallPrototype.uproject` and generate/build its C++ modules.
3. Open an empty level and press Play; the experimental game mode should spawn `AMoonfallPawn`. If the world uses a different game mode, select MoonfallGameMode in World Settings.
4. Verify WASD (X/Y plane), left-click attacks within 400 cm, Shift dodge, mouse wheel zoom.
5. Boss locks a 250 cm radius ground warning to player position for 1.15 s, then deals 20 HP; dodging protects from damage. Boss HP starts at 100; each attack deals 15 HP on hit.
6. **Known limitations:** SaveGame-based victory progression is implemented in source but not validated in a packaged Unreal build; basic colored materials and a polished HUD are not implemented; game has placeholder geometry only; keyboard input setup uses legacy axis mappings; map packaging and camera/audio/UI polish require further work.

## Acceptance criteria before marking prototype playable
- Unreal C++ compiles cleanly in editor and Windows packaged build.
- All inputs register without focus loss; boss telegraph and hitbox alignment verified.
- SaveGame-based permanent bonus verified through game restart and red/blue/purple HUD implemented.
- One continuous traversable level, no wave mode.
- Reproducible Windows 10/11 FPS, frame-time, memory and load time measurements.
- Original Z and Nyra artwork never modified or included in comparison files.

Do not merge into the existing Godot game until independent validation and side-by-side comparison are complete.
