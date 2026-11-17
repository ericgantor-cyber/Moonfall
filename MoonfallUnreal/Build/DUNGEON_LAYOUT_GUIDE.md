# Moonfall Unreal: continuous fourteen-chamber dungeons

The startup game mode now **spawns 30 native dungeon-layout actors** (six zone IDs × five dungeon IDs) plus one instanced ground road connecting all entrances to the arena's southern gateway. This does **not** create 30 authored `.umap` assets or finished biome art and bosses.

## Editor setup

1. Build the Unreal C++ project from the PR chain through this world integration change. Run the existing `MoonfallStart` map.
2. With `AMoonfallUnrealGameMode` running, the world road and thirty dungeons are created automatically at runtime. The original arena remains the starting area, with an opening in its south wall. Walk south, then east along the new road to find entrances. The dungeon passages extend further south.
3. Every spawned instance has unique **ZoneIndex** 0–5 and **DungeonIndex** 0–4 assigned before `FinishSpawning`; a deterministic seed selects its trap/room variation. Level designers may still place custom `MoonfallDungeonLayout` actors but must avoid duplicate identity pairs.
4. The generated architecture is 14 contiguous 1,200 × 900 Unreal-unit rooms, with a physical floor, side walls, four pillars per room, central openings through the internal dividers, and a solid end wall. The array has **instanced** mesh geometry and no per-frame actor tick.
5. The entrance trigger records one-time discovery (including XP and saved zone progress). The final-chamber trigger invokes the Blueprint event `OnFinalChamberEntered` **once per actor lifetime** for a player-controlled living hero. Level authors can attach an authored boss, gate, or quest sequence to that event.
6. Entrances and rooms are navigated by regular movement in one world; the actor intentionally does not call `OpenLevel`, teleport, or spawn waves.

## Limits and follow-up tasks

- The geometry uses Unreal Engine's basic cube asset. Replace its materials/meshes or place world art without changing Z and Nyra's original artwork.
- Each layout is a **straight connected dungeon blockout**, not a finished procedural cave network, random chamber graph, or unique biome. The runtime road is a simple continuous ground strip, not finished open-world terrain. All 30 generated layouts remain loaded at once; assess collision, pathfinding and frame time on target Windows hardware before shipping.
- Treasure chambers and mana shrines have native rewards. Boss behavior, enemy encounters, puzzles, clear-state persistence, zone-specific materials, streamed level placement, adaptive navmesh generation, and Windows gameplay must still be authored and verified in UE5.
- Regression tests only inspect source invariants; a passing check does not certify native UE compilation or runtime traversal.
