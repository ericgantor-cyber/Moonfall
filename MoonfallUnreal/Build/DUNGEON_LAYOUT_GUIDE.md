# Moonfall Unreal: continuous fourteen-chamber dungeons

## Startup-world randomizer integration

After compiling the stacked Unreal C++ PR changes, open `MoonfallUnreal.uproject` in Unreal Editor and run `Build/Create-StartupLevel.py` in the editor Python console. The script now adds a **single actual `MoonfallDungeonLayout` actor** to `/Game/Maps/MoonfallStart`, including when the startup map already exists. It uses zone 0, dungeon 0, and fixed seed **90111**. Its world position is (3500, 4200, 0) and its 14 rooms extend along local +X.

The install is label-guarded and idempotent. It will not replace an existing dungeon actor or overwrite previously authored map actors. To regenerate an existing layout, set its `LayoutSeed` in the Unreal details panel. Level and collision must be inspected before packaging. The first dungeon is added only when this Editor script is actually executed; the source-only PR does not contain a prebuilt `.umap` or Windows executable.


This source change adds a **placeable native** `AMoonfallDungeonLayout`. It does **not** place 30 dungeons in the shipped map or supply authored boss/enemy behaviors.

## Editor setup

1. Build the Unreal C++ project from the PR chain (including #863–#865).
2. In the Unreal Editor, place **MoonfallDungeonLayout** into a persistent or streamed world-level area with enough clear ground. The actor's origin marks the **center of chamber 1**. The long axis runs along its local **+X**, allowing rotation by the level designer.
3. Set the instance's **ZoneIndex** to 0–5 and **DungeonIndex** to 0–4. Use each pair only once per six-zone world. An invalid pair disables the overlap events. A separately authored `MoonfallDungeonEntrance` actor is optional because the layout already has its own entrance trigger.
4. The generated architecture is 14 contiguous 1,200 × 900 Unreal-unit rooms, with a physical floor, side walls, four pillars per room, central openings through the internal dividers, and a solid end wall. The array has **instanced** mesh geometry and no per-frame actor tick.
5. The entrance trigger records one-time discovery (including XP and saved zone progress). The final-chamber trigger invokes the Blueprint event `OnFinalChamberEntered` **once per actor lifetime** for a player-controlled living hero. Level authors can attach an authored boss, gate, or quest sequence to that event.
6. Entrances and rooms are navigated by regular movement in one world; the actor intentionally does not call `OpenLevel`, teleport, or spawn waves.

## Limits and follow-up tasks

- The geometry uses Unreal Engine's basic cube asset. Replace its materials/meshes or place world art without changing Z and Nyra's original artwork.
- Each layout is a **straight connected dungeon blockout**, not a finished procedural cave network, random chamber graph, or unique biome. Place it in sufficiently separated/streamed locations so the geometry doesn't overlap another part of the persistent world.
- Boss behaviors, enemy placements, chamber loot, quest clear-state persistence, adaptive navmesh generation, collision/gameplay accessibility, and the Windows build must still be authored and tested in UE5.
- Regression tests only inspect source invariants; a passing check does not certify native UE compilation or runtime traversal.
