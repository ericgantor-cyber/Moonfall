# Moonfall UE5.6 Windows verification

The actual experimental UE5.6 project is `engine-comparison/unreal/MoonfallPrototype/MoonfallPrototype.uproject`. This process does **not** use the older `MoonfallUnreal` UE5.4 starter.

## Requirements

A trusted Windows 10/11 x64 computer with Unreal Engine **5.6**, Visual Studio **2022** with MSVC C++ x64 tools, and a Windows 10/11 SDK. GitHub-hosted Windows runners do not come with a licensed full Unreal installation. The manual GitHub workflows require a trusted self-hosted runner labeled `unreal-5-6` and `UE56_ROOT` pointing to the UE5.6 installation.

## Run locally from PowerShell

From the repository root:

```powershell
& .\engine-comparison\unreal\MoonfallPrototype\Build\Verify-Windows.ps1 -UnrealRoot 'C:\Program Files\Epic Games\UE_5.6'
```

The script runs, in order: engine/compiler/SDK preflight, the `MoonfallPrototypeEditor` and `MoonfallPrototype` Win64 Development C++ builds, map validation/generation and a cooked Windows package, then a 25-second headless boot check. It stops on failure. The archive goes into a fresh temporary directory; its location is printed after a successful run. Use `-OutputDirectory 'C:\SomeEmptyDirectory'` to choose an empty output folder.

If the build fails, look at its compiler output and Unreal `Saved/Logs`. If the cook fails, look for `AutomationTool` errors and missing imported assets. If the boot check fails, inspect `SmokeDiagnostics/MoonfallBoot.log` in the output folder.

This is **not** a visual gameplay test, a signed installer, or proof that quests, controls, save/reload, and performance work. Original Z/Nyra artwork and the Godot production build remain untouched.
