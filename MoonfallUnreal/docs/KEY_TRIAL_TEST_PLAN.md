# UE5 key trial: test instructions

This trial is a small **opt-in** integration fixture. It does **not** represent
Moonfall's fully migrated Godot world or finished dungeon.

## Enable in UE 5.4 Editor

1. Compile the MoonfallUnreal C++ project on Windows with Unreal Engine 5.4.
2. Place/select the native `MoonfallArena` actor in a test level. Enable
   **Dungeon > Key Trial > Enable Key Trial**.
3. Place a player start with access to both sides of the arena.
4. Play as MoonfallHeroPawn; find the trial key near the negative-X/positive-Y
   side of the arena. It is a placeholder mesh, not a final asset.
5. Press **F** or the existing controller pickup action while nearby. Confirm
   it adds Wardens' Crest without taking an equipment slot.
6. Open the inventory. Click **KEYS** to see the collected key. No K shortcut.
7. Approach the gate near positive-X/positive-Y. It must remain physically
   closed until **F/controller pickup** is pressed. Unlocking must leave the
   key in the ring.
8. In a fresh save without the key, interact with the gate: it must remain
   closed and identify **WARDENS' CREST**. It must not open on proximity.
9. Save/restart, enable the trial again: the already collected key should not
   create another pickup. The gate should still require a deliberate unlock
   interaction.
10. Repeat with a saved open named gate: it should restore its open
    collision/visual state automatically when the level loads. This is
    restoration of a previously earned unlock, not unlocking a new door
    because the player approached. It should fire the separate
    OnSavedGateRestored event, not replay the first-unlock effect.
    A never-opened gate must still require deliberate interaction.
11. Test failed saves: a key pickup must remain in the world and a gate must
    stay closed if its newly unlocked state cannot be persisted.
12. Verify the HUD gives a brief, readable message for successful key
    collection, an already-owned key, missing-key door, successful unlock,
    and failed-save key/door attempts. The text must clear after 3.5 seconds
    without flashes and respect the combat text size preference.
13. Put a key behind the hero and another key or locked door in front,
    both within interaction range. F/controller pickup should prefer the
    front-facing object, regardless of spawn/actor iteration order.
14. Remove the front-facing object and retry: the reachable rear object
    must remain interactable as an accessibility fallback.
15. With a previously collected key actor still placed beside a locked
    door, press F/controller pickup. The stale key should be cleaned up
    without blocking the interaction with the gate. The same interaction
    must never automatically grant a new key or open a gate without its key.
16. Disable Key Trial and verify regular arenas remain unchanged.

## Known limitations

- This is **not verified by Unreal compilation** or an interactive packaged
  Windows test. GitHub source checks are static contracts, not UE build tests.
- SaveGame progress is in a single local slot; multi-character profiles are
  not yet supported. Named gates now save their IDs in version 2, and version-1
  saves can still be loaded.
- Named gates with a saved unlock restore their open state at BeginPlay
  without input or proximity; only never-unlocked gates require manual key
  interaction. Legacy gates with no unique GateId are session-only.
- If key, quest, or door progress cannot be saved, the action must fail
  without destroying the world key or opening the gate. Test read-only save
  directories and save/restart recovery before publishing.
- Fixed trial actor positions and placeholder meshes are for a temporary
  mechanics test, not handcrafted world placement.
- The fifth-kill world key is retried after subsequent confirmed kills if its
  actor failed to spawn; duplicates already owned are dismissed when a pickup
  is initialized or explicitly interacted with. This still requires native
  gameplay/save-restart verification.
- On a failed reward load, the gameplay HUD now displays
  `REWARD SAVE ERROR - ORIGINAL FILE PROTECTED` for 12 seconds.
  Milestone key respawn is disabled for that session until a valid save
  loads; never automatically delete or replace the original slot.
- Invalid or future-version reward saves are now protected against silent
  overwrite: `LoadRewardProgress` signals failure and subsequent reward
  save attempts fail closed. Back up the original slot before testing this
  scenario. Do not delete or migrate a player's slot automatically.
- Native validation should test a deliberately malformed/unsupported-version
  copy of the reward slot, confirm the original bytes are preserved, and
  restore the backup afterward.
- Z and Nyra's original artwork is not edited.
