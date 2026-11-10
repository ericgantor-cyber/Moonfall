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
10. Repeat with a saved open gate: it must stay physically closed after load
    until a deliberate interaction, then open using its saved ID even if the
    corresponding key's state has changed.
11. Test failed saves: a key pickup must remain in the world and a gate must
    stay closed if its newly unlocked state cannot be persisted.
12. Verify the HUD gives a brief, readable message for successful key
    collection, an already-owned key, missing-key door, successful unlock,
    and failed-save key/door attempts. The text must clear after 3.5 seconds
    without flashes and respect the combat text size preference.
13. Disable Key Trial and verify regular arenas remain unchanged.

## Known limitations

- This is **not verified by Unreal compilation** or an interactive packaged
  Windows test. GitHub source checks are static contracts, not UE build tests.
- SaveGame progress is in a single local slot; multi-character profiles are
  not yet supported. Named gates now save their IDs in version 2, and version-1
  saves can still be loaded.
- A newly spawned named gate remains visually locked until you explicitly
  interact. That interaction restores the saved unlock without consuming keys.
  Legacy gates with no unique GateId are session-only.
- If key, quest, or door progress cannot be saved, the action must fail
  without destroying the world key or opening the gate. Test read-only save
  directories and save/restart recovery before publishing.
- Fixed trial actor positions and placeholder meshes are for a temporary
  mechanics test, not handcrafted world placement.
- The fifth-kill world key is retried after subsequent confirmed kills if its
  actor failed to spawn; duplicates already owned are dismissed when a pickup
  is initialized or explicitly interacted with. This still requires native
  gameplay/save-restart verification.
- Z and Nyra's original artwork is not edited.
