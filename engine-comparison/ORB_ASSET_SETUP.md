# Moonfall compact transparent orb art: integration handoff

User-approved visual direction: **slightly smaller ornate red/blue/purple orbs with no surrounding background**, alongside the upper-right minimap. No extra permanent on-screen counters or panels.

The user-created visual file is `moonfall_orbs_small_transparent.png` (RGBA, 680 × 424, transparent corners; no baked-in numbers). It was verified in the conversation workspace, but **the binary asset is not committed to this GitHub branch** because the available repository integration only writes UTF-8 text. This branch contains the import/load hooks, not the bitmap.

## Import to Unity 6
1. Download the PNG from the chat artifact.
2. Place it at `engine-comparison/unity/Assets/Resources/MoonfallOrbs.png`, creating the `Resources` folder if needed.
3. Set Texture Type to Default; alpha transparency enabled.
4. The existing `MoonfallPrototype.cs` loads it once with `Resources.Load<Texture2D>("MoonfallOrbs")` and renders it at most 300px wide in the lower-left. Without the PNG, it retains code-drawn rings.

## Import to Unreal Engine 5
1. Download the same PNG.
2. Import into the prototype Unreal project Content Browser at `/Game/UI/MoonfallOrbs`. Ensure **alpha** is retained and the asset name is exactly `MoonfallOrbs`.
3. `AMoonfallHud` loads the imported UTexture2D at BeginPlay; if the asset is absent it draws the fallback rings. It renders at most 300px wide in the lower-left.

## Important status
- The transparent PNG is an **art/skin overlay**, with **live HP, mana, XP and stamina numbers rendered inside its orbs** by both engine prototypes. The engine controls text, not the imported art. HP updates from combat, XP from victories, and stamina loses 45 on a dodge and recovers 30 per second. Mana currently stays full (180/180) because spells have not been ported. The skin's ornament and glowing fills remain static; live orb fill masking is not yet implemented.
- The minimap remains separate in the upper-right. No extra panels.
- Godot/Z/Nyra assets and save data are not modified. Unity/Unreal visual testing remains unverified.
- The source is uncompiled in Unity and Unreal. Test text positioning and legibility at multiple Windows resolutions; verify that save values and input changes reflect correctly. Do not merge to production without real gameplay checks.
