# Windows build: automatic dependency preparation

Use PowerShell on a trusted Windows build computer with Unreal Engine 5.4 installed.

1. Preview missing tools (no installations):
   `powershell -ExecutionPolicy Bypass -File MoonfallUnreal/Build/Setup-WindowsPrerequisites.ps1`
2. Install missing .NET 8 SDK, NSIS, and Visual Studio 2022 C++ Build Tools using Windows Package Manager (may request administrator approval):
   `powershell -ExecutionPolicy Bypass -File MoonfallUnreal/Build/Setup-WindowsPrerequisites.ps1 -InstallMissing`
3. Reopen PowerShell after installation. Set `UE54_ROOT` to the actual engine directory (e.g. `$env:UE54_ROOT = 'C:\Program Files\Epic Games\UE_5.4'`).
4. To connect the GitHub runner, visit repository **Settings > Actions > Runners > New self-hosted runner**, use GitHub's current Windows x64 download and registration instructions, and assign the additional label `unreal-5-4`. Do not share or hardcode its short-lived registration token.
5. Start `run.cmd` in a session with `UE54_ROOT` set, then invoke the **Moonfall Unreal Windows build (self-hosted)** workflow manually.

The script **does not** automatically download Unreal Engine or a GitHub runner, register a runner, change repository visibility, accept trust decisions, or trigger code execution on a personal PC.

**Security:** GitHub strongly advises using self-hosted runners for private repositories only. Do not connect a daily-use Windows PC directly to a public repo's PR workflow; prefer a dedicated isolated build machine.

**Status:** This automates prerequisite installation only; UE compilation, Windows installer, gameplay tests and final release are still pending.
