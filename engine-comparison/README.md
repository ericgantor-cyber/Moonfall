# Moonfall Dual Engine Experimental Comparison

Tracks #685 Unreal and #686 Unity. Isolated prototype work. Production Godot game and original Z/Nyra artwork MUST remain untouched.

## Equal benchmark contract
One continuous 3D arena segment; no waves or alternate game modes. WASD moves, left mouse attacks, Shift dodges, wheel zooms. Boss has 100 HP; hero has 100 HP; attack deals 15 damage within 4 m, cooldown .35 sec; boss marks a 2.5 m circle for 1.15 sec before dealing 20 damage; dodge immunity .32 sec. Permanent victory reward +5 max HP, capped at 100 victories. Red HP, blue mana, purple XP HUD colors.

## Unity start
Create a new Unity 6 3D project; add the included Unity C# script under Assets/Scripts; enable legacy or Both Input Handling; press Play in an empty scene. The script creates placeholder objects at runtime. It is unverified source code, NOT a shipped Unity build.

## Unreal start
See unreal/README.md. Unreal project implementation remains TODO.

## Benchmark checklist
Record version, CPU, GPU, drivers, renderer, build size, p50/p95 frame times, 1% low FPS, peak memory, load latency, input lag, combat correctness and warnings for 3 repeatable Windows shipping-build runs. Do not compare editor frame rates. Keep unverified claims labeled.
