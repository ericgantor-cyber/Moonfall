# Moonfall v37.3: Deep Waters (source milestone)

Adds bounded 3-axis underwater travel and one below-ground connected grotto per lake.

- In a lake: hold **C** (or gamepad right-stick click) to descend; press **Space** (or gamepad A) to rise quickly; release both to float upward slowly.
- **100% oxygen** drains whenever the player is physically submerged, even when C is released. Surface to refill.
- Water depth is limited to **3.3 world units**. Side grottos extend east from each lake beneath the world terrain; six daily replenishing caches give existing Moon Sigils and Ore Shards. No new currency.
- Terrain surface has an opening under each lake so swimming areas can be seen from below. Cave décor is non-colliding. While submerged, the hero's regular collision mask is temporarily cleared to pass through land-only colliders; it is restored on surfacing. This is an intentional prototype constraint: underwater physics, collision boundaries and camera must be validated in Godot.
- Z and Nyra original images are unchanged, though their 2D billboards remain unchanged rather than providing new animation rigs.
- No survival stopwatch, no building, no new game mode, no save-format reset. Saved cache cooldowns use reserved site slots 30..35, increasing accepted slot limit from 30 to 36.

## Test status
Offline source checks run with `python tests/verify_v37_3_deep_water.py`. Real engine tests run with `python tools/run_tests.py v37_3_deep_water` (requires Godot 4.7.2); Windows `.exe` and gameplay have not been validated here.
