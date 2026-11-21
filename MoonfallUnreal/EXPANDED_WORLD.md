# Moonfall UE5: streamed outdoor world blockout

This branch proposes a **new, functional source-level landscape foundation**.
It does not claim to ship a finished AAA open world or final artwork.

## Physical dimensions

Unreal Engine units are centimeters.

| Area | Physical geometry | Gameplay identity |
| --- | --- | --- |
| Ashen Wilds | 1,000 x 1,000 m | volcanic spires; town blockout |
| Moonveil Forest | 1,000 x 1,000 m | ancient forest silhouettes; town blockout |
| Hollow Depths | 1,000 x 1,000 m | stone columns; town blockout |
| Iron Foundry | 1,000 x 1,000 m | chimney and forge silhouettes; town blockout |
| Frostbound Reach | 1,000 x 1,000 m | glacial peaks; town blockout |
| Eclipse Wastes | 1,000 x 1,000 m | suspended monuments; town blockout |
| Gothic Isle | 1,000 x 1,000 m | ruins blockout; two shores/docks |

All seven are built from 5 x 5 tiles of 200 x 200 m each: **7 km² of
terrain blockout** (6 km² on the mainland plus 1 km² offshore).
Outdoor region centers sit within approximately X = -1.2 to +1.2 km and
Y = 0.62 to 3.02 km of the original arena. Outer mainland bounds are
X = -1.7 to +1.7 km, Y = 0.12 to 2.32 km. The island occupies
X = -0.5 to +0.5 km, Y = 2.52 to 3.52 km.

Each main region retains its own legacy five dungeon entrances in the
original 30-dungeon layout. The preexisting sanctuary, boss arena, player
character assets, Z, Nyra, original hero artwork, shrine data and saves
are untouched. The new outdoor blockout is additive, not a replacement.

## Walking, landmarks and performance

The sanctuary's north gate connects to the Moonveil province. Two
east/west interprovince roads connect each row of three provinces,
and three north/south roads connect the two rows. All these roads are
real collision-enabled ground, without load doors or scripted teleportation.

The coast has two separate walkable docks. The new **Nightferry**
source implements a reversible, 11-second continuous crossing over the
80-meter open-water gap with safe landing and no teleport between coasts.
The ship is a primitive gothic-boat silhouette, not finished artwork.
Right-click the vessel to board it, or stand near either dock and press
controller D-pad Up when not interacting with an NPC. A separate controller
input or new keyboard button was deliberately not added. The voyage
is **source-implemented but not verified in a native Unreal build**.
Blueprints can still query `GetDestinationName()` and `GetSecondsUntilArrival()`
as optional data-only helpers. **No destination, boarding-instruction,
countdown, or blocked-dock status text appears on the ferry.** The existing
mouse/controller boarding actions, travel timing, and safe-landing retry
remain unchanged.

`AMoonfallExpandedWorld` has no Tick. A 0.35-second timer checks the
hero's location and generates up to three nearest 200m tiles per update.
At most 25 tile actors reside at once; tiles unload beyond 600m, load
within 460m, and use an explicit hysteresis band. Only their floor
collides. Scenery is deterministic, non-colliding, non-ticking and
instanced, protecting performance and allowing replacement with
real biome assets. Each province has a recognizable cluster of
placeholder buildings. There are no newly authored shopkeepers,
quests, combat encounters, climates or multiplayer systems in
these blockouts.

These tiles use a custom proximity streamer, **not Unreal World
Partition**. They may need transition and asset prefetch tuning once
native builds and representative Windows machines can be profiled.

## Native Unreal 5.4 setup (still required)

1. Compile `MoonfallUnrealEditor Win64 Development` using UE5.4 and
   Visual Studio 2022 before treating the new C++ code as runtime-ready.
2. For a brand-new, empty startup map, run
   `MoonfallUnreal/Build/Create-StartupLevel.py` in Unreal Editor.
   It adds dungeon and outdoor dynamic NavMesh bounds.
3. For an EXISTING `MoonfallStart.umap`, run
   `MoonfallUnreal/Build/Upgrade-ExpandedWorldNavigation.py`.
   It makes a byte-for-byte backup first and never overwrites the
   original map or existing authored actors. Check the actual nav
   brush bounds in Editor.
4. Press P for the navigation overlay. Check all six province floors,
   the road connections, collisions, tile loading/unloading, memory
   use, multi-floor steering, and return travel to the sanctuary.
5. At the mainland dock, target the **Nightferry** with right mouse
   click or use controller D-pad Up nearby; validate that the character
   visibly rides for 11 seconds, lands on the Gothic Isle, and can
   return on the same boat. Confirm spell casting and NPC interactions
   remain unchanged elsewhere. Test interruption/death during transit,
   blocked shore disembarkation and recovery after removing an obstacle.
   Confirm death releases the moving deck as the character's movement base
   without overriding respawn. Verify walking around the moving deck with
   keyboard, mouse and controller and test obstruction and edge falls.
6. Run the new Python source regression suites:
   `python -m unittest -v tools.test_unreal_expanded_world tools.test_unreal_gothic_ferry`.
   Source checks do **not** substitute for real Win64 compilation.

## WoW-style boat movement (source implementation; native tests pending)

- Added a separate collision-enabled walking deck. The hero is based on the
  moving deck via `SetBase(Deck)` and retains normal CharacterMovement walking
  instead of being attached to the ferry root with movement disabled.
- Added a collision-enabled, dock-only **gangplank** connected to the deck.
  It faces the current shore, and is hidden with collision disabled while
  the boat sails. Characters can approach it physically while docked.
- **Departure still uses the existing right-click or controller interaction**.
  If the hero has physically walked onto the deck, the departure now keeps
  the hero's current position instead of repositioning them to the center.
  Characters interacting from the pier are still placed onto the deck.
  Automatic departure on walking aboard is not implemented; native
  moving-platform and gangplank tests remain necessary.
- Native UE5.4 tests must verify that the hero stays grounded while the boat
  moves, can steer in all directions, does not slip/fall through collision,
  and transfers off the deck correctly at both shore landings.
- The 11-second duration matches the Gothic Raven prototype in PR #1077.
  Reconcile both ferry implementations before merging.

## Next major upgrades

- Replace primitive silhouettes with original, properly licensed gothic
  towns, trees, geological cliffs, snowy mountains, foundries and landmarks.
  Do **not** alter the original Z/Nyra art.
- Finish the gothic Nightferry's 3D ship art, wake effects, sound, and
  accessible arrival prompts after verifying the reversible trip in UE5.
- Add roads/town services/region-specific quests, NPCs, foliage,
  encounters and a useful region-scale Atlas/minimap.
- Profile streaming transitions on Windows 10 and 11 and, when ready,
  move the outdoor world to Unreal's World Partition + level streaming
  without losing shrine, dungeon, or save compatibility.
