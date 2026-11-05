# Moonfall compact transparent orb art: integration handoff

User-approved visual direction: **slightly smaller ornate red/blue/purple orbs with no surrounding background**, alongside the upper-right minimap. No extra permanent on-screen counters or panels.

The user-created visual file is `moonfall_orbs_small_transparent.png` (RGBA, 680 × 424, transparent corners). It was verified in the conversation workspace, but **the binary asset is not committed to this GitHub branch** because the available repository integration only writes UTF-8 text. This branch contains the import/load hooks, not the bitmap.

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
- The alpha PNG is an **art/skin overlay**, not a working HUD with dynamic numeric labels; the colored rings in the fallback are live, but dynamic image masking and mana/stamina integration are **not** implemented in the imported-art branch.
- The minimap remains separate in the upper-right. No extra panels.
- Godot/Z/Nyra assets and save data are not modified. Unity/Unreal visual testing remains unverified.
- Only merge into production after checking the actual display at multiple Windows resolutions and ensuring HUD values are fully dynamic.
