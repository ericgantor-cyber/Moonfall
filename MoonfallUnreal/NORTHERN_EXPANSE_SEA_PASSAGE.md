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
- Each gate is linked to its own `SailingVessel`. **Boarding now moves the
  hero onto the ship's actual collision-enabled walking deck**, then uses
  Unreal CharacterMovement's moving-platform base while the 0.1s timer
  carries the vessel between harbors. The player retains ordinary on-deck
  walking controls, but **cannot steer the ship**. Four invisible pawn-blocking
  guard rails prevent walking off the narrow prototype boat. After eleven
  seconds, the hero detaches from the walking base and is collision-teleported
  to safe land. Leaving the guarded deck, losing the pairing, dying, opening
  a menu or encountering blocked landing cancels the crossing without an
  arrival event. On cancellation it attempts to restore the original dry
  dock position, then a nearby fallback; the ship is reset to its origin.
- **Route integrity and refused-boarding safety:** the departure gate
  captures both the departure and destination ship identities at boarding.
  Every travel update checks the bidirectional gate pairing, both original
  ship references, distinct harbor sides and world ownership. If a Blueprint
  or map actor rewires the route mid-crossing, the boat cancels safely instead
  of arriving at a changed port. If boarding succeeds but the walking base
  cannot be assigned, the hero is returned to their original dry spot; a
  second, separately ground-validated harbor position is attempted if that
  return teleport is blocked. Neither case awards arrival or alters saves.
- **Movement and callback failure watchdog:** each 0.1-second sailing
  update now checks whether Unreal accepted the ship's movement and whether
  the actor actually reached the expected route point (within 10 cm).
  A ship stuck at its dock must never grant an eleven-second "arrival".
  The voyage also rechecks its active state after Blueprint callbacks at
  voyage start and during presentation: a script that cancels the journey,
  disables a harbor, changes the paired dock, or opens a menu cannot
  accidentally trigger a later arrival from stale state. Cancellation uses
  the existing passenger recovery flow. This still requires native UE5
  Editor and PIE gameplay testing before merge.
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
  The `bUseVoyageCamera` gate option is now **off by default** for a regular
  third-person walkable passenger deck; designers can enable the cinematic
  ship-wide camera if desired. If the hero has **Reduce Motion** enabled,
  camera switching is
  immediate instead of blended and decorative wave bob is disabled.
  No manual steering or input bindings were introduced.
- **Crimson sail animation:** six existing sail panels receive a small,
  lightweight, timer-driven sway during the crossing, without cloth physics.
  Each local rest transform is cached before animating and restored on any
  completion/cancellation. Reduced Motion stops sail flutter immediately
  and the movement handler already disables ship bob under that setting.
  The existing visual meshes remain nonblocking and no damage or naval
  encounter actors are spawned. The **new passenger floor and four invisible
  rails block only pawn collision**. Hero boarding uses a safe elevated marker
  instead of jumping from the harbor over empty water.
- **Passenger experience overhaul:** the Gothic Raven itself now carries
  an illuminated, nonblocking travel sign, showing the destination, current
  departure / open sea / approach phase and an eleven-to-one-second countdown.
  The pier sign is still present, but is not the sole source of travel info
  once aboard. The ship root, walking deck and guard rails explicitly use
  **Movable** scene-component mobility so CharacterMovement can recognize the
  vessel as a moving floor. Its pawn-blocking walking deck allows character
  step-up. Boarding height is calculated from the actual deck collision top
  plus the player's scaled capsule half-height, with a small clearance.
  The canceled-voyage emergency deck fallback uses the same capsule-aware
  placement. This is still unverified until native UE5 Editor playtesting.
- **Dark-sea ambience:** the existing crimson lantern pair pulses subtly
  during a voyage, without shadows or new assets. Both lanterns reset on
  departure cancellation or safe disembark and remain steady under Reduce
  Motion. The automatic sailing duration remains **11 seconds**. There are
  no ocean enemies, steering controls or naval battles.
- **Ground-verified disembark:** both boarding and the moment of arrival
  now require a **real blocking floor** beneath the receiving harbor's
  arrival marker, using a short downward Unreal visibility trace. Invalid
  collision, steep slopes (normal Z below 0.7), missing ground or faraway
  platforms reject the crossing rather than treating an editor marker as
  walkable terrain. Valid landings use the actual floor hit plus the
  hero's scaled capsule half-height, with a small clearance. If a dock
  disappears during the 11-second trip, arrival is canceled and the
  earlier harbor-rescue logic runs; no first-landing credit is granted.
  The visibility-traced hit must **also block Pawn collision**: a decorative
  material that appears to be ground cannot count as safe ferry terrain.
  This validation is now shared with a portable C++17 policy and edge-case
  tests on Windows and Linux (including missing collision, steep surfaces,
  oversized capsules and non-finite coordinates).
  Even the cancellation fallback harbor marker now requires a blocking
  walkable floor; if neither harbor landing is usable, the code attempts
  to restore the hero to the reset vessel instead of placing them over sea.
  **This is a source implementation, not a native UE5 walkability proof.**
  The authored mainland dock and Shiverwake pad have collision enabled,
  but must be confirmed in Unreal Editor.
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
on safe dry land at its respective harbor; the gate safely transfers players
to a ship-mounted elevated boarding marker. **Only the deck and invisible
safety rails block pawn movement; all visible ship mesh instances remain
nonblocking.** The ship moves on a bounded ten-updates-per-second timer
only while its ferry crossing is active, not a permanent Tick. The optional
`bShowGothicFerry` flag can hide geometry but the voyage requires the
assigned visual ship actor to remain valid.

This ship visual was added only to the Northern Expanse **draft PR**. The
generator must be run in Unreal Editor 5.4 to create a real map asset.
If an older preview `.umap` already exists, the generator refuses to
overwrite it. An editor user may place the two visual actors manually
rather than deleting an existing preview map. Do not rerun creation against
real player world assets.

## What does NOT yet exist

This is an **opt-in walkable passenger ferry prototype**, not a complete
shipped sailing system. The decorated ship follows an animated eleven-second
route. The hero is now placed on a **moving pawn-blocking deck** supported
by CharacterMovement's movement base. Native UE5 testing is still necessary
to verify that the capsule tracks the ship correctly at all frame rates and
does not fall through the floor or clip into the rails. There are no
high-detail modeled interiors, real gangways, crew, animated cloth rigging,
dynamic wave physics, authored water assets, cross-level streaming, save-backed
crossing unlocks or final
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

## Native UE5 ferry test runner (configured, NOT yet executed)

The repository already contains a **manually dispatched** trusted Windows
compile workflow at `.github/workflows/unreal-compile-windows.yml`.
This branch adds `MoonfallUnreal/Build/Test-NorthernFerryNativeAutomation.ps1`
as an opt-in native post-compile smoke step. The workflow requires a
**trusted self-hosted Windows x64 runner labeled `unreal-5-4`** with
`UE54_ROOT` pointing to a licensed UE 5.4 installation. Ordinary GitHub
Windows runners do **not** include this engine.

Once the PR branch is reviewed and the trusted runner is available,
an authorized maintainer can select **Actions → Moonfall Unreal C++
compile (trusted Windows runner) → Run workflow** and select the
`feature/north-sea-passage-20261119` branch. This manually builds
the real UE Editor and Game targets, then invokes
`UnrealEditor-Cmd.exe` to execute
`Moonfall.NorthernExpanse.Ferry.ComponentContract`. The step
**fails** unless Unreal outputs a fresh `index.json` containing
exactly that test in the `Success` state, with zero failed/not-run
tests, and archives the evidence as
`MoonfallUnreal-GothicRaven-NativeTest-Report`.

**Do not interpret this as native verification until an actual successful
workflow execution and its report exist.** A successful component
contract is also distinct from *PIE gameplay testing*: characters walking
and jumping on the 11-second moving ship, collisions, cameras, controller
input and reverse voyages must still be verified interactively.
The workflow is manually dispatched only; never automatically
execute unreviewed PR code on a licensed self-hosted runner.

## Native Unreal ferry verification (NOT yet executed)

The new `MoonfallNorthFerryNativeTests.cpp` adds a real
`Moonfall.NorthernExpanse.Ferry.ComponentContract` Unreal automation test
under `WITH_DEV_AUTOMATION_TESTS`. It checks actor defaults, the
11-second route, a dedicated pawn-blocking deck and rails, nonblocking
decorative meshes, and the elevated boarding marker. This test requires a
**native UE5.4 Editor build** to compile and run. GitHub's portable
Windows/Linux jobs only confirm its source contract; they do **not**
run the Unreal automation framework.

**Native QA steps (in a throwaway preview world):**

1. Build the UE5.4 `MoonfallUnrealEditor` Win64 Development target.
   Resolve compiler/UHT issues before judging ferry functionality.
2. Run **Tools → Debug → Session Frontend → Automation** (or the Editor
   automation testing interface) and execute
   `Moonfall.NorthernExpanse.Ferry.ComponentContract`. Confirm a real green
   result from Unreal, not merely a Python source-test result.
3. Generate a new preview with `create_northern_expanse_preview.py`
   and inspect it with `validate_northern_expanse_preview.py`. These
   scripts will not overwrite an existing map.
4. In PIE, walk into the mainland boarding trigger, then move across the
   Gothic Raven's actual deck while the 11-second route moves. Test
   keyboard, mouse and controller, stationary riding, turning, jumping,
   rail collision, the normal hero camera, and reduced-motion settings.
   Verify the character's movement base remains the ship's
   `PassengerDeck` while grounded and never silently switches to the
   sea or another collider. Confirm the custom-scaled character capsule
   spawns above the deck instead of clipping through it. Look for the
   **onboard destination and countdown sign**, and verify crimson lantern
   pulses and reset under ordinary and Reduce Motion configurations.
   Check the same route in reverse.
5. Repeat with low frame rate and uncapped frame rate, including 30
   northbound and return trips. Check 11-second timing, the countdown,
   no unwanted instant-return boarding, loss-of-base cancellation,
   disabling or re-pairing a gate mid-voyage, replacing either ship,
   opening a menu, a defeated hero, forced invalid arrival, failed walking
   base assignment, and off-deck recovery. Remove/disable the
   destination dock collision **before boarding** and **mid-voyage**:
   boarding should refuse a missing/steep landing, and mid-voyage removal
   should cancel without granting arrival progress. Also verify that
   grounded arrival uses the *real floor height*, not the marker's height.
6. Confirm a bad landing never earns first-arrival credit, no player
   becomes stranded at sea, no controller loses camera ownership and
   no existing campaign save is read or written. The editor preview
   is isolated and progress intentionally disposable.

**Unverified:** this entire native QA matrix, true moving-base
behavior on every frame rate, and a packaged Windows build. Keep both
PR #1074 and PR #1077 draft until those gates are satisfied.

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
5. After native passenger-deck verification, add genuine gangways,
   high-quality sail animation, ambient storm visuals and sounds, safe docking,
   return-voyage skip, save-backed discovery and Windows 10/11 packaging.
   Keep the voyage automatic and **do not add ocean battles or encounters**.
   Preserve original Z and Nyra visuals exactly; do not merge without
   native verification and review.
