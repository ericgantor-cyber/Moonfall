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

Final water materials, enemies, and campaign placement are pending. Existing campaign equipment transactions remain unchanged.
