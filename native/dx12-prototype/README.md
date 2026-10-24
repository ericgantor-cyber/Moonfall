# Moonfall DirectX 12 experiment

This is a **separate native Windows graphics prototype**, not the main Moonfall game renderer or a DLSS integration. Its arena loads source-identical Z and Nyra textures and the original game’s **bear, three creep, Fire Titan, and Nature Titan** artwork. Gameplay remains a partial native port. Its window now runs a **small, playable native DX12 combat arena** with simple colored rectangles, real-time inputs, deterministic enemy pursuit, projectiles and waves. Direct3D 12 draws this experiment with back-buffer clears and scoped render-target rectangles. It proves a small amount of gameplay can run natively without porting the existing Moonfall JavaScript engine.

The project uses Windows SDK, CMake, WIC and MSVC; no NVIDIA binaries. Original images are never edited in the browser game. Build-time script `prepare-dx12-heroes.js` extracts Z (`art-001.webp`), Nyra (`art-004.png`), Bear (`art-005.webp`), creeps (`art-006` to `art-008.webp`) and Titans (`art-009/010.png`) from the modular assets; it losslessly converts the WebP copies into PNG for native DX12 and checks that the RGBA pixels are identical. All eight original source hashes are recorded in `original-hero-art.json`. No restyling or newly generated character art is used.

The **Moonfall Windows installer also bundles this experimental executable** and offers **Alt → Graphics → Play DX12 Original Art Arena (experimental)**. This starts a separate *playable test window* and does not switch Moonfall's main renderer or enable DLSS. Z and Nyra use the extracted original character images as alpha-blended textured sprites in the installed arena. Their original artwork remains unchanged. The DX12 arena is still a separate prototype; the Hydra does not have a verified original image (it remains geometric), and full Moonfall mechanics are not yet ported. A full Windows build now requires CMake, Visual Studio 2022 C++ build tools and a Windows SDK.

## Build (Windows 10/11, Visual Studio 2022 C++ workload, Windows SDK)

```powershell
cmake -S native/dx12-prototype -B native/dx12-prototype/build -G "Visual Studio 17 2022" -A x64
cmake --build native/dx12-prototype/build --config Release
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe --probe
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe
```

The `--probe` command initializes a Direct3D 12 device without opening a window. It reports hardware or WARP fallback. **WARP is a software renderer, not an indication of playable performance.**

## Prototype controls and rules

- WASD or arrow keys: move the selected character using original Z or Nyra art
- Tab: switch between original Z and Nyra artwork (same run)
- E: early prototype hero skill. Z has a faster, stronger **Dragon Burst**; Nyra has a temporary **Shield**. These remain approximations, not full-game parity
- Q: switch between **Bolt** (auto-targeted projectile) and **Nova** (close-range pulse with cooldown)
- Space or left mouse button: auto-target shot at the nearest enemy
- Shift + movement: dash (cooldown)
- R: restart arena after victory/defeat, or start a new run anytime
- Three waves with Chaser, Runner and Brute prototype roles, now displayed using the actual original creep and bear images where installed
- After 36 defeats, the Hydra prototype appears, with its existing enrage/minions/area strike and **additional original-game-derived volley patterns**. Boss projectile counts (5 then 9), 190/145-frame cadence, 35-frame warning, and stage-dependent aimed spread are based on `boss-combat-stage.js` at 60 FPS
- When Hydra falls, **press 1** for the Moonsteel Edge prototype (+25% damage) or **press 2** for Crimson Vitality (+30 maximum HP and 50 healing), then fight a **Fire/Nature Titan** stage using original Titan image sprites. The boon effects are simplified, not identical to the browser game's full +100 HP rule
- Short-lived hit flashes, original boss-style bullet patterns, impact outlines, and weapon pulse effects are rendered natively; health at zero loses
- The window title displays hero, health, progress and wave. Creeps, bear and Titans use exact original game art. Hydra (no verified source sprite), projectiles and effects use simple test geometry. Z and Nyra retain their exact original art textures.

Native simulation is deliberately separated in `arena.hpp` and checked by `MoonfallArenaTests` under CTest. A separate `--art-probe` initializes all eight original textures on the Windows runner, verifying WIC decode and DX12 uploads. `test-dx12-source-port.js` verifies sprite file references and source-defined boss pattern constants. This **is not a complete port** of original boss/weapon behavior, saves, all enemy designs, multi-stage progression or artwork. The Hydra, enemy roles and weapons are **prototype interpretations**. Z and Nyra's *actual original* image files are preserved and used as-is.

## Next milestones

1. Verify prototype runs on a real gaming PC, collects vendor/driver information, and handles device loss.
2. Identify any Hydra-specific original art, verify original boss move sets beyond `mfBossExtra`, and expand original enemy/room move-set parity.
3. Replace experimental skill approximations with verified game-equivalent abilities and add depth, motion vectors, temporal jitter, texture residency and measured frame times.
4. Only then evaluate NVIDIA DLSS Super Resolution integration/licensing on supported RTX GPUs.

Do **not** delete the browser/Electron game. The DX12 prototype intentionally builds independently; it cannot consume the existing JavaScript renderer automatically.
