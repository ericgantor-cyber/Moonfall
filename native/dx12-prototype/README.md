# Moonfall DirectX 12 experiment

This is a **separate native Windows graphics prototype**, not the Moonfall game renderer, launcher, or a DLSS integration. Its window now runs a **small, playable native DX12 combat arena** with simple colored rectangles, real-time inputs, deterministic enemy pursuit, projectiles and waves. Direct3D 12 draws this experiment with back-buffer clears and scoped render-target rectangles. It proves a small amount of gameplay can run natively without porting the existing Moonfall JavaScript engine.

The project uses the installed Windows SDK and CMake/MSVC; no copied NVIDIA binaries or artwork. Z and Nyra's original art remains entirely in the game.

The **Moonfall Windows installer also bundles this experimental executable** and offers **Alt → Graphics → Play DX12 Combat Arena (experimental)**. This starts a separate *playable test window* and does not switch Moonfall's main renderer or enable DLSS. Z and Nyra are not represented by the abstract test square, and their original artwork remains unchanged. A full Windows build now requires CMake, Visual Studio 2022 C++ build tools and a Windows SDK.

## Build (Windows 10/11, Visual Studio 2022 C++ workload, Windows SDK)

```powershell
cmake -S native/dx12-prototype -B native/dx12-prototype/build -G "Visual Studio 17 2022" -A x64
cmake --build native/dx12-prototype/build --config Release
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe --probe
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe
```

The `--probe` command initializes a Direct3D 12 device without opening a window. It reports hardware or WARP fallback. **WARP is a software renderer, not an indication of playable performance.**

## Prototype controls and rules

- WASD or arrow keys: move the anonymous test marker
- Space or left mouse button: auto-target shot at the nearest enemy
- Shift + movement: dash (cooldown)
- R: restart arena after victory/defeat, or start a new run anytime
- Three increasingly difficult waves; 36 defeats wins the prototype arena; health at zero loses
- The window title displays health, progress and wave; game elements are simple rectangles, not Moonfall's final combat art or a representation of Z or Nyra.

Native simulation is deliberately separated in `arena.hpp` and checked by `MoonfallArenaTests` under CTest. This is **not** a port of Moonfall's original abilities, bosses, saves, effects or art.

## Next milestones

1. Verify prototype runs on a real gaming PC, collects vendor/driver information, and handles device loss.
2. Replace prototype rectangles with a carefully verified art/sprite loading pipeline while preserving original art, then port one real Moonfall encounter and its mechanics.
3. Add correct depth, motion vectors, temporal jitter, texture residency and measured frame times.
4. Only then evaluate NVIDIA DLSS Super Resolution integration/licensing on supported RTX GPUs.

Do **not** delete the browser/Electron game. The DX12 prototype intentionally builds independently; it cannot consume the existing JavaScript renderer automatically.
