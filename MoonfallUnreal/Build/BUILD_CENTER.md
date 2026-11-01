# Moonfall Build Center: first playable Windows build

**Verified:** GitHub-hosted Windows PowerShell parsing, Build Center control initialization, and `Start-Build-Center.cmd -SelfTest` have passed in CI as of PR #284. **Not verified:** Unreal Engine 5.4 C++ compilation, packaged-game launch, graphics, gameplay, or installation on your PC.

## Start Build Center

From an updated local Moonfall checkout, double-click `MoonfallUnreal\Build\Start-Build-Center.cmd`. Alternatively:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\MoonfallUnreal\Build\Build-Center.ps1
```

1. **CHOOSE ENGINE**: select the UE5.4 installation folder containing `Engine\Build\BatchFiles\Build.bat`.
2. **BUILD PREFLIGHT**: verify the Unreal project, Build.bat and Visual Studio 2022 C++ toolchain without compiling or deleting outputs.
3. **TEST COMPILE**: compile the UE5.4 editor target only, without running packaging or deleting prior output.
4. **EXPORT LOG**: save build output and errors to a text file if the compile fails. **OPEN LOGS** opens persistent logs under LocalAppData.
5. **BUILD MOONFALL**: compile, cook, package, build launcher and NSIS installer after explicit confirmation.

Other buttons: **CHECK TOOLS** (preview), **INSTALL TOOLS** (winget, with confirmation), **CANCEL** (terminate job process tree; verify child processes are gone), **OPEN OUTPUT** (local build files).

## Optional packaged game process launch smoke test

Only after full cook/package succeeds, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\MoonfallUnreal\Build\Test-PackagedGame.ps1 -GameExecutable "C:\path\to\MoonfallUnreal.exe"
```

Or opt into the same check while packaging:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\MoonfallUnreal\Build\Build-Windows.ps1 -UnrealRoot "C:\Program Files\Epic Games\UE_5.4" -LaunchSmokeTest
```

The smoke test runs the packaged executable briefly and terminates **only the process it starts**. Do not run it when another unsaved game session is open. A passing process check **does not prove** that the window rendered correctly, controls work, combat performs well, or saves are reliable.

## Playable release acceptance (not yet completed)

- UE5.4 Windows C++ compile succeeds with no build errors
- Game executable launches with valid assets and visible HUD
- Character selection, movement, spells, enemies and leveling work
- Boss encounters, loot, item systems and save/resume are exercised where implemented
- Controller, remappable controls and UI readability are tested
- Frame rate and memory are measured with large enemy groups
- NSIS setup installs, launches, and uninstalls cleanly on Windows 10/11
- Release checksum matches, and publication occurs only after approval

**Security:** Do not connect a personal-use PC as a self-hosted GitHub runner for untrusted public-repository code. Prefer isolated/private runners.

Preserve Z and Nyra's supplied character artwork exactly and retain the red HP orb, blue mana ring and purple XP ring.
