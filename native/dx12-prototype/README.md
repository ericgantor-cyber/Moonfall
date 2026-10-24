# Moonfall DirectX 12 experiment

This is a **separate native Windows graphics prototype**, not the Moonfall game renderer, launcher, or a DLSS integration. Its window clears a Direct3D 12 swapchain to an animated purple color, proving device/queue/swapchain/command-list/present operations can work.

The project uses the installed Windows SDK and CMake/MSVC; no copied NVIDIA binaries or artwork. Z and Nyra's original art remains entirely in the game.

## Build (Windows 10/11, Visual Studio 2022 C++ workload, Windows SDK)

```powershell
cmake -S native/dx12-prototype -B native/dx12-prototype/build -G "Visual Studio 17 2022" -A x64
cmake --build native/dx12-prototype/build --config Release
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe --probe
.\native\dx12-prototype\build\Release\MoonfallDX12Prototype.exe
```

The `--probe` command initializes a Direct3D 12 device without opening a window. It reports hardware or WARP fallback. **WARP is a software renderer, not an indication of playable performance.**

## Next milestones

1. Verify prototype runs on a real gaming PC, collects vendor/driver information, and handles device loss.
2. Move one isolated scene onto the DX12 renderer, preserving original art and game logic.
3. Add correct depth, motion vectors, temporal jitter, texture residency and measured frame times.
4. Only then evaluate NVIDIA DLSS Super Resolution integration/licensing on supported RTX GPUs.

Do **not** delete the browser/Electron game. The DX12 prototype intentionally builds independently; it cannot consume the existing JavaScript renderer automatically.
