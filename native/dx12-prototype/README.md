# Moonfall DirectX 12 experiment

This is a **separate native Windows graphics prototype**, not the main Moonfall game renderer or a DLSS integration. Its experimental arena loads the original Z and Nyra character art as GPU textures when installed, with early experimental hero skills, while the rest of the arena is still an approximation. Its window now runs a **small, playable native DX12 combat arena** with simple colored rectangles, real-time inputs, deterministic enemy pursuit, projectiles and waves. Direct3D 12 draws this experiment with back-buffer clears and scoped render-target rectangles. It proves a small amount of gameplay can run natively without porting the existing Moonfall JavaScript engine.

The project uses Windows SDK, CMake, WIC and MSVC; no NVIDIA binaries. Original Z and Nyra image assets are never edited in the browser game. Build-time script `prepare-dx12-heroes.js` extracts the game-provided Z artwork (`art-001.webp`) and Nyra artwork (`art-004.png`) from the modular assets; it losslessly converts Z's copy into PNG for native DX12 and checks that the RGBA pixels are identical. Original source hashes are recorded in `original-hero-art.json`. No restyling or newly generated character art is used.

The **Moonfall Windows installer also bundles this experimental executable** and offers **Alt → Graphics → Play DX12 Original Art Arena (experimental)**. This starts a separate *playable test window* and does not switch Moonfall's main renderer or enable DLSS. Z and Nyra use the extracted original character images as alpha-blended textured sprites in the installed arena. Their original artwork remains unchanged. The DX12 arena is still a separate prototype; it does not reproduce the complete original Moonfall combat. A full Windows build now requires CMake, Visual Studio 2022 C++ build tools and a Windows SDK.

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
- Three waves with **Chaser**, **Runner** (fast) and **Brute** (slow/tough) enemy roles
- After 36 regular defeats, a two-phase **Hydra prototype** appears. At half health it enrages, moves faster, and summons minions. A visible warning box precedes its area strike; defeat it to win
- Short-lived hit flashes, impact outlines, enemy health bars and weapon pulse effects are rendered natively using simple DX12 rectangles; health at zero loses
- The window title displays hero, health, progress and wave. Enemies, projectiles, Hydra and effects still use simple test geometry; Z and Nyra use their exact original art textures.

Native simulation is deliberately separated in `arena.hpp` and checked by `MoonfallArenaTests` under CTest. A separate `--art-probe` initializes original character textures on the Windows runner, verifying the WIC decode and DX12 uploads. This **is not a full port** of the main game's actual weapons, bosses, enemies, saves, level systems or art. The Hydra, enemy roles and weapons are **prototype interpretations**. Z and Nyra's *actual original* image files are preserved and used as-is.

## Next milestones

1. Verify prototype runs on a real gaming PC, collects vendor/driver information, and handles device loss.
2. Port verified main-game enemy/boss art and move-set data, including the Hydra's actual attack phases, rather than treating these simplified placeholders as full parity.
3. Replace experimental skill approximations with verified game-equivalent abilities and add depth, motion vectors, temporal jitter, texture residency and measured frame times.
4. Only then evaluate NVIDIA DLSS Super Resolution integration/licensing on supported RTX GPUs.

Do **not** delete the browser/Electron game. The DX12 prototype intentionally builds independently; it cannot consume the existing JavaScript renderer automatically.
