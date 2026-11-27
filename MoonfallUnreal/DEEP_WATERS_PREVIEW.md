# Deep Waters preview

This is an isolated UE5.4 greybox, not a campaign map or a finished art pass.
The six rooms have open roofs and separate air pockets. No campaign save changes,
loot grants, new character art, or ferry route changes are required.

## Install in Unreal Editor

1. Compile MoonfallUnreal (including the MoonfallAirPocket class).
2. Create a dedicated empty map at /Game/DeepWaters/Maps/DeepWatersPreview.
3. Set World Settings > GameMode Override to MoonfallNorthPreviewGameMode.
4. Author a rectangular, axis-aligned PhysicsVolume with Water Volume enabled.
   Minimum dimensions: 6000 x 4000 x 500 cm. Select this volume.
5. Remove any existing PlayerStart in this dedicated map.
6. Execute Tools/install_deep_waters_preview.py inside Unreal Editor.
7. Inspect generated geometry and save the preview map manually.

The installer refuses other maps, incompatible game modes, undersized volumes,
and duplicate installation. On placement failure it attempts to remove only
actors created in that invocation.

## Acceptance checks (native editor required; not yet executed)

- Enter the pool from the dry dock; verify native swimming mode and surface speed.
- Tap Space/A to ascend and Q/B to dive; tap again to stop vertical intent.
- Visit all six rooms without a collision trap; escape directly upward.
- Test each air pocket with the hero's eye inside and outside its bounds.
- Verify three-minute standard breath and six-minute Shadowborn breath.
- Verify drowning pulses and stop drowning when reaching air.
- Verify the dry dock can be reached from the water.
- Verify existing menus, land controls, and ferry behavior are unchanged.
- Verify campaign save slots remain untouched during preview play and respawn.

The preview includes six session-only treasure chests, one per grotto. Collect them using existing pickup controls or auto-loot while swimming within reach and line of sight. The contextual HUD counts unique discoveries out of six. Claims survive hero respawn but reset when the preview world ends. No gear, XP, or currency is granted.

Acceptance checks: collect each chest once; verify duplicate IDs cannot increase the count, blocked sight prevents collection, and campaign inventory remains unchanged.

Six preview aquatic guardians use three-dimensional swept pursuit within a bounded home territory. They have six distinct profiles, windups of 0.75–1.4 seconds, named attacks and a contextual countdown, check line of sight and range before attacking, cancel attacks when the swimmer enters an air pocket or leaves the territory, and return home. Existing damage and parry rules apply; defeating them grants no campaign XP or equipment.

Native acceptance checks: verify each guardian can be hit while swimming, a wall blocks its bite, air pockets cancel its warning, and land enemies remain unchanged. Verify enemy placement and warning readability in the editor.

Final water materials and campaign placement are pending. Existing campaign equipment transactions remain unchanged.

## Encounter overhaul

Saltveil Hunter, Pearlback, Lantern Eel, Kelp Stalker, Glassfin Rare, and Brineheart Leviathan use different health, damage, reach, movement speed, recovery and silhouettes. The Leviathan has faster recovery at 65% and 30% health. The HUD reads the same live warning as the attack resolver. Underwater melee can aim vertically with camera pitch; diving uses the authored pool geometry rather than a fixed 3.3 m depth limit. These are preview encounters, not production bosses. Each ordinary grotto has a 70% chance of its original creature, with 30% distributed across the other three ordinary profiles. Glassfin has a 25% chance in its own grotto; otherwise a common creature appears. The final Leviathan is guaranteed. Rolls happen once per creature at world start, not each tick or hero respawn, with no additional actors. Preview creatures fail closed outside the preview game mode. Native checks must confirm that low breath remains visible over menus and the small-window HUD does not clip.
