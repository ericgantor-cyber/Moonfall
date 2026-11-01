# Moonfall Unreal 5.4: real Windows compilation

The repository's Linux and hosted Windows checks do **not** compile Unreal Engine.
The compile job must run on a Windows machine with Unreal Engine 5.4 installed.

## Option A: Local Windows build (recommended first)

1. Install Unreal Engine **5.4** using Epic Games Launcher.
2. Install Visual Studio 2022 with **Desktop development with C++**, MSVC and Windows SDK.
3. Clone or download the branch containing the latest changes.
4. In PowerShell, from the repository root:

```powershell
$env:UE54_ROOT = 'C:\Program Files\Epic Games\UE_5.4'
./MoonfallUnreal/Build/Build-Windows.ps1 -UnrealRoot $env:UE54_ROOT -PreflightOnly
./MoonfallUnreal/Build/Build-Windows.ps1 -UnrealRoot $env:UE54_ROOT -CompileOnly
```

The preflight checks installation paths and C++ tooling, but **does not compile**.
The second command compiles the Unreal Editor target. Preserve the complete error
output from any unsuccessful build and attach the first compiler error and surrounding lines.

## Option B: GitHub Actions self-hosted build

1. Register a trusted **self-hosted Windows x64** GitHub Actions runner for this repository.
   Follow GitHub's repository Settings > Actions > Runners instructions; do not publish
   access tokens in issues, code, logs, or chat.
2. Add runner label `unreal-5-4`, and install the same prerequisites as above.
3. Set the `UE54_ROOT` environment variable on the runner machine to the engine directory.
4. The workflow `Windows Unreal build prerequisites` includes a manual input
   `compile_on_self_hosted`. Run the workflow from a branch where the workflow exists
   with that input checked. Check the `unreal-editor-compile` job, not the hosted
   `windows-preflight` job.
5. If compilation fails, inspect the job logs and the
   `moonfall-unreal-build-diagnostics` artifact, if generated.

**Security:** Only run trusted code on a persistent self-hosted runner. Restrict who
can trigger workflow runs, avoid adding repository secrets to the build process,
and keep the runner and developer toolchain updated.

## What counts as verified

- **Source checks passed:** Source layout, project metadata, Python balance checks and
  PowerShell script parsing work. This is NOT proof that Unreal compiles.
- **Editor compile passed:** The `-CompileOnly` step exits successfully with no compiler errors.
  This is NOT proof that cooking, packaging, launcher or installer work.
- **Installer verified:** Run the full `Build-Windows.ps1` pipeline with packaging tools
  available, verify its output checksum, install on a clean Windows 10/11 machine,
  launch the game, and test gameplay/controller support.

The Unreal prototype is experimental; earlier draft PRs must be reconciled before
a release candidate. Z and Nyra's original image assets must remain unchanged.
