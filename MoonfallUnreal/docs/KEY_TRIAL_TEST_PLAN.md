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
10. Disable Key Trial and verify regular arenas remain unchanged.

## Known limitations

- This is **not verified by Unreal compilation** or an interactive packaged
  Windows test. GitHub source checks are static contracts, not UE build tests.
- SaveGame progress is in a single local slot; multi-character profiles and
  full world/door persistence are not yet integrated.
- Gate unlock state is per spawned gate actor; a freshly spawned gate must be
  interacted with again even if its key remains collected.
- Fixed trial actor positions and placeholder meshes are for a temporary
  mechanics test, not handcrafted world placement.
- Z and Nyra's original artwork is not edited.
