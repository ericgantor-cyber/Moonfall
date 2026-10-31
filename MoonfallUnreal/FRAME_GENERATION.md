# Frame generation integration for Moonfall Unreal

**Status: preparation only. Frame generation is NOT installed or active.** The Unreal C++ starter targets Unreal Engine 5.4; it is not compiled or packaged yet. Its default Windows RHI now requests DirectX 12 to prepare for supported plugins. Render resolution scaling and temporal anti-aliasing are **not** frame generation.

## Choose a supported plugin

- **AMD FSR Upscaling / Frame Generation**: AMD's current Unreal plugin supports UE 5.3-5.8, including 5.4. Download from https://gpuopen.com/learn/ue-fsr/ and follow its engine-specific setup. Its analytical frame generation fallback can cover more supported GPUs; ML variants have specific AMD GPU and OS requirements.
- **NVIDIA DLSS Frame Generation**: NVIDIA's latest plugin releases are version-specific. For this UE 5.4 project obtain an **explicitly UE 5.4-compatible archived version** from https://forums.developer.nvidia.com/t/nvidia-dlss-unreal-engine-plugin-archive/377564 or migrate the project to an engine version supported by the latest plugin. The latest NVIDIA DLSS 4.5 downloads on its main page target newer UE releases: https://developer.nvidia.com/rtx/dlss. Do not install a plugin intended for a different Unreal version.

## Practical integration checklist

1. Build and launch the baseline Unreal project in UE 5.4 on Windows before adding any proprietary plugin.
2. Download the chosen vendor's official compatible Unreal plugin and review its license and engine/platform/GPU requirements.
3. Install the vendor plugin locally under the engine or project Plugins folder as instructed by its official documentation. Do **not** commit vendor binaries or redistribute them without permission.
4. Enable and configure the plugin in Unreal Editor, including its required upscaler and frame generation options. Keep frame generation **disabled by default** until validated.
5. Test in a **packaged Windows DX12 build** on supported hardware, with frame generation both on and off; verify startup, UI, Alt+Tab, display-mode changes, latency and frame pacing. Monitor real simulation FPS separately from generated/display FPS.
6. Expose a user-facing Frame Generation choice only after plugin availability and GPU/driver compatibility can be detected; otherwise hide/disable the setting with an explanation. Keep the 4K resolution-scale fallback working when FG is unavailable.

**No claims of working DLSS/FSR frame generation, doubled FPS, NVIDIA Reflex, or a 4K Windows build should be made until hardware testing succeeds.** Godot Moonfall3D and Z/Nyra artwork are unchanged.
