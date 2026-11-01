# Moonfall Build Center (Windows)

**Status:** initial GUI source. It has not yet been launched or tested on a Windows machine. Full Unreal compilation requires Unreal Engine 5.4 and the development toolchain on your PC.

Launch from a local checkout of the Moonfall repository in **Windows PowerShell 5.1**:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\MoonfallUnreal\Build\Build-Center.ps1
```

Buttons:

- **CHECK TOOLS**: Detect .NET 8 SDK, NSIS, Visual Studio C++ build tools and required UE5.4 files. Does not install anything.
- **INSTALL TOOLS**: Asks for permission before running the winget installer script. Windows may request administrator approval.
- **BUILD MOONFALL**: Asks permission, then invokes the full local Unreal build and NSIS packaging script.
- **CANCEL**: Attempts to terminate the running Windows process tree. Check for orphaned compiler processes if termination fails.
- **OPEN OUTPUT**: Opens `MoonfallUnrealLocalBuild`. A successful installer should be named `MoonfallUnreal-Setup.exe` and accompanied by a SHA-256 checksum file.

Set the editable Unreal Engine root text field to your actual UE5.4 directory if different from `C:\Program Files\Epic Games\UE_5.4`.

This first stage does **not** register a GitHub runner, auto-publish a release, execute gameplay tests, or verify the graphical UI in a real Windows session. Never grant a public-repository Actions workflow direct access to your everyday PC. Use a private repo and dedicated isolated runner if enabling remote builds.

Next steps: Windows execution tests, robust cancellation integration tests, build diagnostics and versioned release publishing with confirmation.
