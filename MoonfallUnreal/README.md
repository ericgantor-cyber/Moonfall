# Moonfall Unreal Engine 5 migration starter

Experimental parallel Unreal Engine 5.4 C++ project. This does **not** replace the Godot game or convert existing gameplay. Open the .uproject in Unreal Engine 5.4 with Visual Studio C++ tools installed; allow source compilation and create a level with PlayerStart. This has not yet been compiled or run in Unreal. Only third-person pawn camera/movement and Space/controller jump input are scaffolded. Combat, controller Options/Start, spells, bosses, crafting, and installer are **not ported**.

No Z/Nyra sprites or meshes are included or altered. Import the user's approved original designs unchanged before adding hero visuals. No WoW action bars. Never delete Moonfall3D until a validated feature-parity migration exists.

## Migration step 2: combat input scaffold

Added keyboard and Xbox-style controller bindings for Space/A jump, Q/B dodge, Z/LB left cast, X/RB right cast, and C dual cast. Six spell indices expose base mana costs and cooldowns and Blueprint event `OnSpellCast` to hook custom VFX and gameplay. Healing is prototyped for Restoration; other spells currently spend mana and fire a Blueprint event but **do not damage enemies**. Dual cast checks cooldowns for both hands. No dungeon map, enemies, UI, Options menu, crafted items or art has been ported yet. This Unreal starter **has not been built or run**. The complete Godot version remains available under `Moonfall3D/`.
