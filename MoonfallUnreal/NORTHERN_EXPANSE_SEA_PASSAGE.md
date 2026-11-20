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
  onto the deck to start an **eleven-second automatic sailing preview**.
  A Blueprint boarding prompt can also call `BoardFerry`, which now returns
  whether the crossing STARTED, not whether it has already arrived.
  There is no manual steering or separate keyboard-only helm control.
- Designers place **two** actors in the same loaded UE5 world, set
  `bIslandHarbor=false` at the mainland and `true` on the island, set
  each `PairedGate` to the other and explicitly enable both. A failed,
  missing, disabled or non-reciprocal pairing refuses travel.
- Each gate is linked to its own decorative `SailingVessel` actor. A 0.1s
  timer moves the outbound vessel toward the receiving harbor using a smooth
  bounded path and gentle cosmetic wave bob (no Actor Tick, no boat physics).
  After eleven seconds, a healthy nearby player is collision-teleported to
  the receiving harbor. If the traveler leaves the dock, dies, opens a menu,
  the pairing becomes invalid, or landing is blocked, the crossing cancels;
  the ship returns to its initial position and no arrival event fires.
- **Accessible voyage countdown:** the harbor sign displays "ARRIVAL IN
  11S" and counts down to "1S" during the timed animation. The optional
  `OnAutomaticVoyageProgress(Hero, SecondsRemaining, PhaseIndex)` event
  supports future subtitles, narration, sound and camera transitions:
  1 = departing harbor (first quarter), 2 = crossing the dark sea (middle
  half), 3 = approaching harbor (final quarter). Countdown and cues are
  published only when the displayed second or phase changes, not every
  0.1-second boat movement update. A successful landing fires the separate
  `OnFerryArrival` event; a failed landing must **never** announce arrival.
  With no Blueprint implementation, only the dock sign visibly changes.
- **Gothic Raven cinematic camera:** the ship now has a rear-facing chase
  viewpoint looking along the black hull toward its raven prow. On starting
  the automatic crossing the *local* player controller can blend into the
  ship camera; the normal view is restored when the voyage succeeds, fails
  or is canceled. The restoration only runs if the ship is still the active
  view target, so a separate cutscene/camera system is not forcibly reset.
  The `bUseVoyageCamera` gate option can disable this and keep the existing
  view. If the hero has **Reduce Motion** enabled, camera switching is
  immediate instead of blended and decorative wave bob is disabled.
  No manual steering or input bindings were introduced.
  **The character remains at the departure dock during the camera ride**;
  the ship's deck is currently non-colliding geometry, and this is
  *not* yet a walkable moving-deck feature.
- The arrival marker is outside its gate's overlap sphere. Place markers on
  collision-tested safe ground, not over water or other boarding triggers.
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
or overlaps; the ship moves on a **bounded ten-updates-per-second timer
only while its ferry crossing is active**, not a permanent Tick. The optional
`bShowGothicFerry` flag can hide geometry but the voyage requires the
assigned visual ship actor to remain valid.

This ship visual was added only to the Northern Expanse **draft PR**. The
generator must be run in Unreal Editor 5.4 to create a real map asset.
If an older preview `.umap` already exists, the generator refuses to
overwrite it. An editor user may place the two visual actors manually
rather than deleting an existing preview map. Do not rerun creation against
real player world assets.

## What does NOT yet exist

This is an **opt-in automatic crossing prototype**, not a complete
walkable sailing experience. The decorated ship **does** follow an animated
eleven-second route, but the hero **currently remains at the departure dock
during the ship animation** before being moved to safe land at its destination.
The new camera follows the animated boat, but the hero is not physically
attached to it. There is no moving walkable deck, animated cloth/sail rigging, interactive
captain or crew, dynamic wave physics, camera cutscene, authored ship/water
assets, streaming between levels, permanent crossing unlock, or final
playable island landscapes. Per the design direction there will be **no
ghost-ship fights, sea monsters, naval combat or ocean encounters**.
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
5. Only after tests, add a walkable sailing deck, genuine sail animations,
   ambient storm visuals and sounds, camera transitions, safe docking,
   return-voyage skip, save-backed discovery and Windows 10/11 packaging.
   Keep the voyage automatic and **do not add ocean battles or encounters**.
   Preserve original Z and Nyra visuals exactly; do not merge without
   native verification and review.
