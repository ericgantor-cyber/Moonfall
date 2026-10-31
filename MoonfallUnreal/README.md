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
