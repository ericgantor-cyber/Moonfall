# Northern Expanse: ferry from mainland to Shiverwake Coast (prototype)

This stacks on the Northern Expanse branch / PR #1074, **not** on current `main`.
The player reaches the nine-zone northern continent by boarding an original
Moonfall ferry at an authored mainland harbor. The paired return ferry is at
Shiverwake Coast, close to Tideglass Haven. It is an alternative crossing edge,
**not** a tenth province, a shrine shortcut, or a replacement of existing maps.

## What exists in code

- The portable `MoonfallNorthSeaPassage.h` policy only accepts mainland to
  Shiverwake or Shiverwake to mainland; no harbor can route to itself.
- `AMoonfallNorthFerryGate` is an editor-placeable, disabled-by-default,
  collision-triggered ferry deck and text sign. A healthy local player can walk
  onto the deck to board. A Blueprint boarding prompt can also call `BoardFerry`.
  There is no need for a separate keyboard-only control.
- Designers place **two** actors in the same loaded UE5 world, set
  `bIslandHarbor=false` at the mainland and `true` on the island, set
  each `PairedGate` to the other and explicitly enable both. A failed,
  missing, disabled or non-reciprocal pairing refuses travel.
- The arrival marker is outside its gate's overlap sphere. Place the markers
  on collision-tested safe ground, not above water or overlapping a boarding
  trigger. Successful Unreal `TeleportTo` returns control at the other harbor.
- `OnFerryArrival(Hero, bFirstIslandLanding)` permits future sound, camera,
  cinematic and journal hooks. First island landing is tracked **only for the
  current session** and is deliberately not written to the legacy save slot.
- No actors tick, no first-voyage achievements or rewards are written, and the
  original Godot world, UE5 six-zone gameplay, Z/Nyra artwork, Morvella actor,
  dungeon state and saved IDs are unchanged.

## Gothic Raven ferry visual (editor greybox)

`AMoonfallNorthGothicFerryActor` constructs an original dark-gothic
vessel silhouette in the editor using lightweight instanced engine cubes:
layered black hull and ribs, cathedral-style deck pavilion and spires,
ironwork/rails, five torn crimson sail panels, raven-head prow and beak,
crimson eyes, four hanging lanterns, and two modest non-shadowing red lights.
Optional built-in material tints are provisional. The full detailed concept
painting is **visual direction**, not a shipped or imported high-resolution
ship mesh, textured asset or animated ship.

The isolated Northern Expanse preview generator now places **two separate
decorative ships offshore**: a mainland vessel near (0, -7650, -40) and a
Shiverwake vessel near (0, -2750, -40). The actual boarding trigger stays
on safe dry land at its respective harbor. No ship parts generate collision
or overlaps; there is no ship Tick or ocean movement. The optional
`bShowGothicFerry` flag can hide geometry without changing the ferry gates.

This ship visual was added only to the Northern Expanse **draft PR**. The
generator must be run in Unreal Editor 5.4 to create a real map asset.
If an older preview `.umap` already exists, the generator refuses to
overwrite it. An editor user may place the two visual actors manually
rather than deleting an existing preview map. Do not rerun creation against
real player world assets.

## What does NOT yet exist

This is an **opt-in interactive gate prototype**, not a placed, traversable
ship journey. There is no real boat movement, sea physics, storms, sea monsters,
cutscene, ship or water artwork, loading/streaming between separate levels,
permanent crossing unlock, or generated final playable island landscapes.
It has not been compiled or playtested in Unreal 5.4 on Windows. The
Northern Expanse **editor preview generator now authors a dry mainland test
dock, a PlayerStart there, and two reciprocal, enabled ferry gates** beside
the existing connected nine-zone greybox. This creates a ferry path in the
generated test map only if an Unreal Editor user runs the tool and saves the
new level. The default main game startup remains unchanged.

### Preview-only campaign save isolation (source implementation)

The `AMoonfallHeroPawn` now identifies the dedicated
`AMoonfallNorthPreviewGameMode` at `BeginPlay`. In that mode it sets
`bPersistentSaveWritesBlocked` **before** attempting to load progress,
and `LoadWorldProgress()` refuses to read the campaign slot. Normal
two-second autosave calls are rejected through the existing write guard,
as are progress pickups that require successful persistent writes.
The save-existence check is skipped before the preview tests' new-player
sanctuary relocation. Death uses in-memory PlayerStart fallback, not the
campaign slot. The normal game mode retains its previous behavior.

This deliberately makes **preview progress disposable**, not permanently
saved. Test save isolation in actual UE5 before assuming it works. No
northern save schema, port unlocks or versioned cross-map persistence
exists yet. Do not attach experimental gates to a production main map.

## Native acceptance checklist

1. Integrate PR #1074 and this stacked PR in a throwaway checkout, run
   the portable Linux/Windows C++17 and source tests.
2. Compile the complete UE5.4 Editor Win64 target, then inspect both actors,
   component settings, UHT reflections and landing marker positions.
3. Run `MoonfallUnreal/Tools/create_northern_expanse_preview.py` **inside
   UE5 Editor 5.4**. It refuses to overwrite an existing preview map, places
   the nine-province greybox and an isolated mainland dock at
   (0, -10000, -20) with separate ferry gates at (0, -10000, 65)
   and (0, -1200, 65), and starts the hero at the mainland.
   Run `validate_northern_expanse_preview.py` in Editor and check the
   2 ferry gates, reciprocal references, dry arrival areas, and safe spacing.
   Then playtest northbound/return crossings, blocked landing, disabled
   gate, missing pair, menus open and a defeated hero.
   **No real player save must be loaded or modified.**
4. Check controls with mouse, keyboard and controller. Check that entering
   the arrival dock does not immediately bounce the player back to origin.
5. Only after tests, add actual ship VFX, fade transition, ocean hazards,
   return-voyage skip, save-backed discovery and packaging to Windows 10/11.
   Preserve original Z and Nyra visuals exactly; do not merge without
   native verification and review.
