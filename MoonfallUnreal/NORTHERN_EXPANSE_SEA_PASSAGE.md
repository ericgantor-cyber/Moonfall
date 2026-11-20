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

## What does NOT yet exist

This is an **opt-in interactive gate prototype**, not a placed, traversable
ship journey. There is no real boat movement, sea physics, storms, sea monsters,
cutscene, ship or water artwork, loading/streaming between separate levels,
permanent crossing unlock, or generated playable island map. It has not
been compiled or playtested in Unreal 5.4 on Windows. The existing northern
preview generator and default game startup are unchanged; merely adding this
actor to source does not make it appear in-game.

### Critical save isolation before an island preview playtest

The current Moonfall hero pawn loads/saves the existing world slot and
autosaves every two seconds. Do **not** attach the experimental ferry to the
legacy world or run an experimental continent save against a real player's
existing save. A dedicated northern preview pawn / separate test save slot
and versioned world travel persistence must be implemented and verified
first. This branch does not change any autosave behavior.

## Native acceptance checklist

1. Integrate PR #1074 and this stacked PR in a throwaway checkout, run
   the portable Linux/Windows C++17 and source tests.
2. Compile the complete UE5.4 Editor Win64 target, then inspect both actors,
   component settings, UHT reflections and landing marker positions.
3. In an **isolated test level and isolated save**, place the connected
   continent and two mutually paired gates, both enabled, at safe dock floors.
   Make northbound/return crossings, try blocked landing, disabled gate,
   missing pair, menus open and a defeated hero.
4. Check controls with mouse, keyboard and controller. Check that entering
   the arrival dock does not immediately bounce the player back to origin.
5. Only after tests, add actual ship VFX, fade transition, ocean hazards,
   return-voyage skip, save-backed discovery and packaging to Windows 10/11.
   Preserve original Z and Nyra visuals exactly; do not merge without
   native verification and review.
