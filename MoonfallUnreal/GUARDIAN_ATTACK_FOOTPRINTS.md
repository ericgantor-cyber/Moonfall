# UE5 guardian attack footprint slice (draft)

Four distinct **opt-in** guardian move footprints reuse AMoonfallEnemy's existing windup, real cover trace, parry/dodge/guard, damage and recovery pipeline. Assign one of these native Actor Tags to a non-divine boss Blueprint: `MoonfallGuardianMoonbound`, `MoonfallGuardianEmber`, `MoonfallGuardianVenom`, `MoonfallGuardianEclipse`. They are **not automatically assigned** to existing dungeons yet.

- Moonbound Ring: donut shape; safe in the center or outside outer ring.
- Ember Charge: forward lane; sidestep.
- Venom Pool: target-locked circle; move out of the circle.
- Eclipse Cross: world-aligned crossed lanes at the locked target; leave both lanes.

Windup is >=1.35 seconds, damage multiplier <=1.6, and the existing weapon/collision/cover/defense outcomes are unchanged. Prototype geometry uses nonpersistent engine debug lines/circles with names, not production meshes/VFX. Existing Z/Nyra boss identity, art, spells and Godot files are not modified.

**QA before merge:** Compile native UE5.4 on Windows; tag one guardian of each kind; test warnings and actual hits against moving/dodging/parrying/blocking heroes, line-of-sight/cover, interrupt/death/menu cancellation, distance edge cases, and CPU/GPU load. Compile and run portable `tools/test_unreal_guardian_footprints.cpp` with a C++17 compiler. Source review and GitHub CI alone do not establish a native Unreal build.
