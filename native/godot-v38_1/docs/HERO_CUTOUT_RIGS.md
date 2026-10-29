# Moonfall original-art-safe articulated rig

The source files `assets/heroes/Z_Original.webp` and `assets/heroes/Nyra_Original.png` are preserved exactly. Z's file includes a richly painted background; there are no premade transparent limb cutouts checked in.

**Implemented:** Both rigs have 17 core joints and six extra hair, cape, and cloth joints, working independent bone motions, original whole-sprite movement, sockets, and attack/cast reactions.

**Safe cutout import:** `hero_art_rig.gd::install_cutouts(original, layers, approved)` takes 16 full-size transparent `Texture2D` layers. Each must be the same pixel size as the original image and must be manually reviewed before `approved=true`.

Required dictionary keys: `Backplate, Pelvis, Torso, Head, UpperArm_L, LowerArm_L, Hand_L, UpperArm_R, LowerArm_R, Hand_R, UpperLeg_L, LowerLeg_L, Foot_L, UpperLeg_R, LowerLeg_R, Foot_R`.

Each transparent part canvas must retain exact original image coordinates and paint detail. **Backplate must be a properly repaired background behind the removed character.** The artist must confirm all cutouts reassemble into the exact original picture in a neutral pose; size validation alone cannot prove pixel-perfect restoration.

Incomplete, unapproved, or mismatched layers are rejected, keeping the intact original Sprite3D visible. `restore_original()` undoes the swap without overwriting any source file.

**Not yet available:** Genuine independently moving visible limbs/hair/cloth on the Z/Nyra originals. Creating accurate transparent cutout source assets requires careful manual segmentation and clean-plate restoration, especially because Z's original is a full composition, not an isolated character.
