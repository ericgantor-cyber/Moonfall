# Moonfall UE5: rigging across character categories

This guide describes **source-level integration points**, not completed animation assets or a verified Unreal Win64 build. Imported Z and Nyra artwork, skeletons, textures, and materials must remain unchanged.

## Existing playable heroes (Z, Nyra)
- The existing `AMoonfallHeroPawn` supports optional, independently assigned sword meshes, configurable hand socket names, fallbacks, grip offsets, and main/offhand montage hooks.
- To apply a new mesh or socket setup, call `RefreshDualWieldRig` in the Blueprint. A one-second bounded audit also detects skeletal mesh and animation-instance swaps.
- If the artist's skeleton has no compatible hand socket, the weapon uses the existing capsule-relative fallback. This **does not** retarget animation or add skin weights.

## Regular enemies, elite enemies, and bosses
- These categories currently share `AMoonfallEnemy` and therefore the new `CharacterRig` skeletal mesh component.
- Derive an enemy Blueprint and assign an existing skeletal mesh to `CharacterRig`. Use Unreal's animation blueprint normally and optionally assign `AttackRigMontage`, `HitRigMontage`, and `DeathRigMontage`.
- The original `Body` sphere remains the collision and navigation root.
- The skeletal visual uses absolute scale so the collision sphere's non-uniform X/Y/Z scale does not deform imported rigs; set the skeletal mesh component's own scale as needed in the Blueprint. With a valid skeletal asset, its sphere visual is hidden and the skeletal mesh appears. Without a skeletal asset, the sphere visual remains.
- Character rigs have no collision/overlaps or navigation contribution. Attack, hit, and defeat montage attempts emit `OnCharacterRigCue`. A missing montage or animation instance does not stop damage or rewards.
- Call `RefreshCharacterRig` after reassigning the mesh in a running level.

## NPCs and other Blueprint actors
- **No standalone native NPC character class is present in this UE5 source tree.** Existing or new Blueprint actors may add `MoonfallActorRigComponent`, which uses their own existing `USkeletalMeshComponent`.
- Add named entries to `MontageCues`, then call `PlayRigCue(CueId)`. The method returns whether playback started and emits `OnRigCue` for cosmetic fallback.
- If an NPC Blueprint has multiple skeletal mesh components (body, armor, props), set `PreferredRigMesh` to the **body's skeletal mesh**. The adapter validates that the selected component belongs to the same actor; when unset it selects the first mesh with a valid skeletal asset. `ActiveRigMesh` exposes what was selected for debugging.
- The adapter does not create, replace, re-skin or modify any character asset and does not tick. Call `RefreshRig` when swapping the actor's skeletal mesh.

## Native UE5 verification required before merge
1. Compile in Unreal 5.4 for Win64; fix any C++ or UHT errors.
2. Load original Z and Nyra skeletal Blueprints, test hand sockets, montages, fallback and both weapon meshes.
3. Test skeletal ordinary enemy, elite and boss variants; verify that sphere-only enemies still render, navigate, collide and reward normally.
4. Test Blueprint NPCs with and without skeletal meshes and montages; confirm null-safe behavior.
5. Test gameplay including blocking, dodge, death/finisher transitions, reduced-motion option, performance and save/load.
6. Audit stacked PR dependencies and checks before merging. Do not merge draft PR #910 solely because Python source contract tests pass.
