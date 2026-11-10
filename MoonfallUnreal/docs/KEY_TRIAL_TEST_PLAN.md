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
- The fifth-kill world key is tracked with a weak actor reference after its
  first discovery/spawn. Subsequent kills avoid repeatedly scanning all world
  keys while it is still alive; if destroyed without collection, the next
  confirmed kill can rediscover or respawn it. Native gameplay should verify
  recovery, destruction and save/restart behavior. Duplicates already owned
  are dismissed when initialized or explicitly interacted with.
- On a failed reward load, the gameplay HUD now displays
  `REWARD SAVE ERROR - ORIGINAL FILE PROTECTED` for 12 seconds.
  Milestone key respawn is disabled for that session until a valid save
  loads; never automatically delete or replace the original slot.
- Invalid or future-version reward saves are now protected against silent
  overwrite: `LoadRewardProgress` signals failure and subsequent reward
  save attempts fail closed. Back up the original slot before testing this
  scenario. Do not delete or migrate a player's slot automatically.
- When a kill cannot persist its reward, the existing HUD feedback now
  warns for 6 seconds that gold and kill progress were not saved, or that an
  invalid save is protected. In a read-only test save, verify the in-memory
  kill count and gold roll back and no milestone key is awarded for that
  failed transaction.
- Native validation should test a deliberately malformed/unsupported-version
  copy of the reward slot, confirm the original bytes are preserved, and
  restore the backup afterward.
- Both the hero and named gates now read the shared save-slot identifier
  from `UMoonfallRewardSaveGame::GetRewardSlotName()`; the existing on-disk
  `Moonfall_RewardProgress_v1` filename is deliberately unchanged to
  preserve v1/v2 save compatibility.
- Named gates reuse the hero's already-validated reward state when the hero
  initializes first, avoiding one disk read per door. If a gate initializes
  first, it loads the existing reward slot directly. Check both initialization
  orders in a native UE5 level, and verify no gate restores an unlock when the
  loaded reward save is corrupt or has an unsupported schema.
- Z and Nyra's original artwork is not edited.

## Empty-slot-only auto-equip verification

- Start with an empty weapon, armor and accessory loadout. Collect one item
  for each slot using F/controller pickup. Each first item should equip,
  and the inventory and loadout should survive a save/restart.
- Collect a stronger item for an occupied slot. It must enter the bag
  without replacing the currently worn piece; equipping an upgrade remains
  an explicit inventory action.
- With writes denied, collect an item for an empty slot. The bag entry,
  equipped code, tier and affix, and any armor-derived health bonus must
  roll back; the world pickup remains available for retry.
- Test scripted equipment grants with both empty and occupied slots.
  They share the same pickup transaction and do not replace occupied gear.

## Diverse loot categories within an encounter

- For the first three equipment drops from one enemy, choose random
  weapon/armor/accessory categories **without replacement**. Bosses with
  three or four drops therefore cover all three categories, and elites
  with two drops get two different equipment categories.
- A fourth bonus boss drop has a fully randomized category. Rarity, traits,
  tier range, atomic world-loot saving, and empty-slot-only auto-equip remain
  unchanged.
- Native UE5 test: defeat several elites and bosses, confirm category
  coverage and save/reload of each exact item code. Repeat with failed
  saves to verify the same diverse items reappear as temporary drops.

## Randomized enemy loot

- Regular enemies always roll one item plus a 20% chance for a second.
  Elites always roll two plus a 35% chance for a third. Bosses always
  roll three plus a 50% chance for a fourth. Rolls are independent for
  each defeated enemy; the batch remains bounded to four records.
- Each item independently rolls its rarity, trait and equipment category,
  with tier varied by -1/0/+1 and clamped to 1..25. Existing minimum
  rarity protections for elite/boss loot remain intact.
- In native UE5 gameplay, test large samples from each enemy class,
  verify the base reward is guaranteed and bonus rates are plausible,
  then check save/reload, duplicate IDs, the 512-world-drop cap and
  the no-loss temporary pickup fallback under failed writes.

## Batched elite and boss loot saves

- One enemy death generates a randomized group: regular 1–2, elite 2–3, boss 3–4.
  All valid records are appended and committed with a **single** reward-save
  write, then their pickup actors are materialized. On write failure the
  entire new group is rolled back, with temporary manual pickups attempted
  for every rolled item and an explicit unsaved-loot warning.
- In a test arena kill multiple elites and bosses, restart and verify each
  dropped code appears exactly once with the expected count. Verify there
  are no phantom saved IDs from rejected writes or malformed duplicate IDs.
- Run read-only-save tests and fill the 512-record cap. Verify no partial
  saved boss reward group appears: either the group is committed, or every
  attempted item becomes temporary where actor spawning succeeds.
- This reduces save writes for one multi-drop encounter, but many different
  enemy deaths still write independently. Verify gameplay on native UE5
  Windows builds before concluding that combat stutter has improved.

## Pickup visibility optimization and on-demand recovery

- When six nearer reachable pickups are already candidates, distant actors
  should not incur expensive line-of-sight traces. Test a crowded loot area
  with walls and verify an occluded near pickup never excludes an accessible
  farther pickup. Verify only six closest visible items are collected.
- Simulate a successful durable save followed by failure to spawn its loot
  actor. Stand within 250 cm of that saved record and press F/controller.
  The action must make one recovery attempt, rescan nearby loot, and permit
  pickup without requiring a process restart.
- With no saved loot nearby, verify pressing F does not trigger a full
  persisted-world restoration. Retry happens only on explicit input, not
  per-frame. Profile dense arenas and save/reload behavior in native UE5.

## Manual loot interaction with many persistent drops

- Scatter more than 128 persistent loot actors across the map, then spawn
  a new pickup next to the hero. Press F/controller pickup and confirm the
  nearby item is collectable even if it was spawned after those distant drops.
- Each press still chooses at most six nearest valid candidates within 250 cm
  and performs height/visibility checks. Verify multiple adjacent pickups
  are gathered in proximity order, not actor iteration order.
- The actor scan is now unbounded per explicit button press; profile worst
  case worlds with hundreds of transient actors. It does not run every frame.

## Orphan and duplicate persistent pickup cleanup

- Restore world loot with two actors carrying the same persisted GUID.
  The first scan must remove extra copies, then use the saved record as
  the authoritative item code; only one valid pickup remains collectable.
- Restore with a persisted actor whose GUID is missing from the current-map
  ledger, including a GUID belonging to a different map. The stale actor
  must be removed, while saved drops from the other map stay on disk.
- Normal temporary pickups deliberately have no persistent GUID. They
  must survive this reconciliation pass and remain available for manual
  collection; never delete transient fallback loot by mistake.
- Re-run restoration and save/reload to verify idempotence, duplicate
  prevention and correct inventory/equipped rollback when disk writes fail.

## Cross-map pickup hardening and saved-actor collision checks

- Persisted pickups now require an exact match of GUID, item code **and**
  current map before removing a saved record. Test that a stale world actor
  from map A cannot claim its saved loot while in map B.
- During restoration, the existing actor index now tracks GUIDs to actual
  pickup actors in one world scan. If a live actor has the saved GUID but the
  wrong item code, restoration destroys the conflicting actor, logs a warning
  and recreates the authoritative saved item instead of blocking recovery.
  Verify the correct item is collectable and no phantom pickup remains.
  If the replacement spawn fails, the saved record remains for another load.
- Verify that failed pickups still leave saved records and the physical
  pickup intact, and that successful pickups persist normally after restart.
  Native UE5 compilation and cross-map gameplay validation remain pending.

## Map-aware world-loot persistence (reward save v5)

- New persistent drops store the originating level name alongside their GUID,
  item code and position. On map A, leave loot on the ground, travel to a
  different map B and reload. A's saved drops must not appear at B's coordinates.
  Return to map A and verify those exact drops are restored there.
- Keep the same cross-map ledger so picking up a drop in A only removes its
  matching GUID while leaving saved drops from B untouched. The global ledger
  cap is still 512 records.
- v4 legacy saves did not record a map. On first v4 load, records are
  provisionally assigned to the current map for migration; the original
  location cannot be inferred reliably. Back up v4 saves before migration.
- Invalid v5 records with a missing level identity are skipped. Test full
  restart after a successful save and verify named gate progress still opens.

## World-loot persistence (reward save v4)

- Back up an existing v1, v2 or v3 reward save. Loading upgrades it in memory
  without discarding previously saved gold, gates, keys, equipped gear or bag.
  Schema v4 adds up to 512 durable world drops with unique GUIDs and positions.
- Kill regular, elite and boss enemies; leave several drops on the floor. Exit,
  reload, and verify the same item codes and positions reappear. Reload twice
  to verify each saved GUID appears only once.
- Pick up a restored drop with F/controller. Its GUID and item code must be
  removed from saved ground loot in the same save transaction that adds the
  item to the bag; test empty-slot auto-equip and occupied-slot preservation.
  After restart, the collected drop must not reappear.
- Deny reward-save writes and try collecting a restored drop. The bag, equipped
  stats and GUID ledger must roll back, and the pickup must remain available.
  Deny writes during enemy death: new uncommitted drops must not be portrayed
  as durable; the HUD reports the write failure.
- Invalid saved GUIDs, duplicate GUIDs, malformed item codes and non-finite
  positions are rejected on load; inspect a backup save, never production data.
- On reload with hundreds of saved drops, restoration now indexes existing
  persistent GUIDs once, rather than traversing all live pickups for every
  saved record. Verify there are no duplicates after repeated restoration.
- If the persistent world-loot ledger is full or its disk write fails, the
  game attempts a clearly labeled temporary pickup with the exact same item
  code. Verify it can be manually collected after saving recovers, but that
  the warning accurately states it will not survive exiting before pickup.
  If even transient spawning fails, a distinct failure warning is shown.
- Native UE5 compilation, Windows playtest, and stress testing with many
  pickups are still required. Scripted *recovery* drops created during an
  ongoing disk-write failure are best effort and cannot survive a restart
  until a successful save occurs.

## UE5 loot and save v3 verification

- Upgrade a copy of a valid schema-v1 or v2 reward save. Gold, keys, quests
  and named open gates should remain available; schema v3 adds collected
  inventory and salvage without changing the existing slot filename.
- Manually pick up several dropped equipment items with F/controller input.
  Restart and verify the saved bag entries persist, including duplicates.
  Ground drops that were not collected are still transient in this prototype.
- With save writes denied, try to pick up ground equipment. The item must
  remain visible and loot inventory must be rolled back, with a HUD warning.
  Check that a cluster pickup does not destroy a failed-to-save item.
- Salvage a non-equipped item with writes allowed, then reload: the item
  stays removed and the salvage currency persists. Deny writes and repeat:
  the original item and previous salvage value must both be restored.
- Test invalid item codes in a *backup* v3 save. Malformed entries must not
  materialize in the inventory, and the loaded bag is capped at 4,096 items.
- Schema v3 now also saves the explicitly equipped weapon, armor and
  accessory item codes, and upgrade/ascend/reforge crafting transactions.
  Restore only equipped items still present in the validated saved bag.
  Verify worn armor correctly adjusts health exactly once after load/reload.
- With save writes denied, attempt direct equipping and each crafting action
  on an unworn and worn item. The original inventory code, salvage currency,
  equipped code/tier/affix and armor-derived health must be restored.
- Scripted legacy equipment grants now use the same transactional inventory
  save as manual pickups. With a writable save, verify the exact granted code
  remains in inventory after restart without automatically equipping.
  With save writes denied, the exact rolled item should spawn as a manually
  collectable world pickup (not be rerolled). If the recovery spawn fails,
  the HUD must explicitly warn that the item could not be recovered.
- Normal enemy drops now use the v4 persistent world-loot ledger. Emergency
  recovery drops spawned when scripted reward saving fails remain unsaved,
  since the disk is already rejecting writes. Native Windows UE5 compilation,
  gameplay and stress checks are still necessary.
