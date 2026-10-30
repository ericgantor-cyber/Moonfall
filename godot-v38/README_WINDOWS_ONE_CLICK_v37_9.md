# Moonfall v37.9: One-click Windows prerequisites and release build

**This ZIP contains source and setup automation, not copies of commercial or external installers.**

On Windows 10/11, extract the ZIP to a writable folder and double-click **`BUILD_MOONFALL_ONE_CLICK.bat`**. The script will:

1. Detect a working **Python 3** executable; if missing, install **Python 3.13** through Microsoft's Windows Package Manager (WinGet package `Python.Python.3.13`). It rejects the Windows Store `python.exe` placeholder and refreshes this process's PATH so the older batch tests use the right interpreter.
2. Detect the **Inno Setup 6** compiler (including per-user and Program Files paths). If missing, install **Inno Setup 6.7.3** through WinGet package `JRSoftware.InnoSetup`, pinned to major version 6. Both packages are obtained through the WinGet official catalog, which verifies installer hashes; installer binaries are not redistributed in Moonfall.
3. Download and SHA-256 verify the official Godot 4.7.2 Windows editor and export templates (large first-run download, about 1.3 GB for templates).
4. Run **39 source verifiers**, **65 registered Godot integration suites** and the existing strict error-log gate. An automated pass is required before an export can proceed.
5. Produce the portable Windows executable, then compile **the Inno Setup installer with a desktop-shortcut option**. In normal mode, if Inno Setup installation or compilation fails, the build reports failure instead of claiming a release.
6. Write `build/BUILD_STATUS_v37_9.json` with build hashes and tool information **only on scripted success**.

**Requirements:** Windows Package Manager (`winget`) should be present for first-time Python/Inno installation, plus internet access, disk space for Godot export templates, and any Windows permissions required by the installers. On systems without WinGet, install Microsoft App Installer from https://aka.ms/getwinget before using the build script. Local user permissions are requested when needed; this does not bypass administrator prompts. Some corporate/managed PCs block package installations.

**Advanced:** To build only a portable executable without Inno Setup, run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BUILD_MOONFALL_ONE_CLICK.ps1 -PortableOnly` in an existing PowerShell window. This is an explicit exception; normal double-click builds require an installer.

**Not verified in this environment:** WinGet install behavior on a Windows PC, actual Godot parsing/gameplay, installer compilation/install/uninstall, controller tests, FPS or 4K. The source ZIP does not itself contain a working `.exe` or the third-party Python/Inno installers. Z and Nyra's original artwork is unchanged.
