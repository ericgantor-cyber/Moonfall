# Moonfall Native v11: Living World Upgrade

## Implemented in source

- Day/night cycle (8 game-minutes per cycle) with lighting and fog changes in the same connected six-region map.
- Four world weather states: Clear, Mist, Rain, Eclipse. Rain slows unmounted travel slightly; Mist reduces creature awareness; night increases their awareness. Weather changes at intervals, not every frame.
- Six map markers for regional encounters: three supply caches and three 3-kill hunts. Rewards are gold/materials, with an Epic trophy for hunts. Completed events cannot be claimed twice.
- Hurt non-boss, non-rift creatures may retreat briefly with a cooldown; no new AI actors are spawned.
- Existing v10 rifts, boss phases, hero/tamer talents, taming, skinning, gear, and six continuous regions preserved.
- Checkpoint schema 2 remains intact. v11 adds optional day, weather, event completion and active hunt progress, with defaults for old saves.
- Fresh hero selection resets world events, weather, time, and transient progression; loading a save clears stale attack/dodge/skinning state.
- Removed duplicated HUD status fields, and exposed day/weather/event progress.
- Updated Windows startup/verify/export/installer scripts for v11 paths.
- Z and Nyra art files copied unchanged, hash checked in tests.

## Not implemented / testing limits

- No native Windows executable or compiled installer is included.
- No Godot engine was installed in this working environment, so this is source/structural verification, not proof the game launches or plays without errors.
- Randomized indoor dungeon architecture, full skill-tree rework, new authored enemy art, and cinematic boss cutscenes are outside this v11 pass.
- Do not distribute as a tested release until `VERIFY_GODOT_PROJECT.bat`, the v11 smoke test, and an in-engine playtest pass on Windows.
