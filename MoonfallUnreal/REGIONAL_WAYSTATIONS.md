# Moonfall: six connected regional waystations

**Implementation:** Unreal Engine 5.4 C++ source. This is a draft
feature, **not** a verified Windows 10/11 executable or cooked content.

Six new visitable camps branch north from the existing continuous
30-dungeon east/west public road. Each region has a distinct landmark name,
a permanently discoverable shrine, two named NPCs and a small group of
nonblocking ruins. They use existing NPC dialogue, faction reputation,
one-time salvage contracts, bonds, shrine discovery and saved checkpoint
state. **No extra game mode, teleporter, zone loading screen, save version,
invented gear slot, loot table change, or god character artwork changes.**

| Region | Roadside waystation | Discoverable shrine | Scout | Second resident |
| --- | --- | --- | --- | --- |
| Ashen Wilds | Emberwatch Camp | Emberwatch Shrine | Cinder Runner | Ashforge Keeper, merchant |
| Moonveil Forest | Silverleaf Refuge | Silverleaf Shrine | Mira Fernstep | Selune Whisper, scholar |
| Hollow Depths | Deepstone Haven | Deepstone Shrine | Nox Tunnelwatch | Dara Flint, tinker |
| Iron Foundry | Sparkfall Relay | Sparkfall Shrine | Kora Spark | Mek Ironhand, tinker |
| Frostbound Reach | Winterglass Lodge | Winterglass Shrine | Eira Froststep | Orin Snowbind, scholar |
| Eclipse Wastes | Duskwatch Refuge | Duskwatch Shrine | Sable Nightroad | Vesper Rune, elder |

## How to reach each camp

Each site is next to the **third of the five dungeon entrances** in its
region. Travel along the existing thirty-lane east/west road from the
Crossroads. Follow the warm-colored, nonblocking **NORTH: [camp name]** road sign to the north side of the road. A second high-contrast sign identifies the camp, its shrine and resident contracts. A
flat 960-unit-long station platform overlaps the main public road by 230
units, without a gap, loading trigger or jump. The camp floor and public
road have the same Z=0 walking surface. A shrine is near the far edge;
two residents are around the center.

The six locations are symmetric with respect to the original center
Crossroads, with relative X coordinates **-16875, -10125, -3375, +3375,
+10125 and +16875**. Roadside platforms sit at local Y=-1800;
original dungeon entrances remain south at Y=-3100. The existing
Shattered Sanctuary and Z/Nyra showcase remain untouched.

## Persistence and interactions

- Each shrine has its own **stable FName ID** and human-readable Atlas
  display name. Approach within its existing discovery radius to activate
  it. Discovered shrines remain active when another checkpoint is set.
  Their IDs are stored in the existing discovered-shrine array.
- All **12 residents** have stable, distinct resident IDs, settlement
  IDs, factions, races and roles. Use Moonfall's normal mouse/controller
  NPC interaction to talk, accept a salvage contract and confirm a turn-in.
  Existing reputation, quest completion and resident bond records apply.
  Merchants and tinkers are **dialogue/contract NPCs**, not new open
  item shops or fully implemented crafting services.
- Players can choose another checkpoint by discovering another shrine;
  an outpost does not automatically become the respawn point just because
  it exists. Approaching it is required.
- New site construction is optional via the arena actor's
  `bEnableRegionalWaystations` property, enabled by default. Existing
  saved world progress and original shrine IDs remain compatible.
- The six new shrines and twelve resident IDs stay below the current
  64-entry shrine and quest-state save caps, even with prior Crossroads
  residents. No change to the serialized save schema.

## Performance and accessibility

- Only **two instanced static-mesh components** render the six camp
  platforms and the 24 non-colliding ruin pillars, rather than a new
  ticking actor per prop. Ruins have no collision or dynamic shadows.
  Platforms have physical collision and join the road.
- Twelve world-space high-contrast labels (one road direction and one camp sign per region), six shrine actors and twelve
  static NPC actors are spawned once. The arena, all NPCs and shrines
  have **no per-frame Tick**.
- Readable region and shrine names provide non-color navigation cues.
  The existing NPC conversation, quest prompts and Atlas continue using
  the same inputs; no new hotkey or mandatory interaction mode is added.
- Geometry spawns in the existing arena world setup. On lower-end PCs
  review the cost of additional residents, text and shrines in a native
  UE profiler; source inspections cannot establish a real frame rate.

## Native validation before merging

1. On a real Windows UE5.4 editor, compile the project and open both the
   generated default world and an authored arena with the toggle off.
2. Walk the road to all six camps. Check no floor gap, obstructing pillar,
   incorrect world transform, or collision blocker; inspect at 720p,
   1080p, and 4K.
3. Test both mouse and controller NPC interactions, one-time contracts,
   turn-in confirmations and faction/bond records. Verify no repeated
   salvage rewards.
4. Approach each shrine, confirm Atlas discovery and correct respawn
   checkpoint; save, restart and verify all 6 IDs persist.
5. Confirm all 30 dungeon lanes, the original sanctuary, existing
   residents, Z and Nyra character artwork, bosses, and prior saves still
   work. Monitor hitching/physics/render costs on Windows 10 and 11.
6. Do not merge until native compile and regression playtests are
   independently verified.

Automated checks in `tools/test_unreal_regional_waystations.py`
cover the counts, ID uniqueness, relative placement, road overlap,
actor collision policy, persisted integration, and the original boss
source contracts. They do **not** replace native playtesting.
