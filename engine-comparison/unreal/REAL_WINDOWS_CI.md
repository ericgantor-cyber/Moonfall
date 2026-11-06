# Real Unreal Engine 5.6 Windows CI

Moonfall's ordinary source and PowerShell checks run on hosted GitHub runners, **without** Unreal Engine. They cannot establish that Unreal C++ compiles or that a packaged game runs.

An optional `unreal-win64-real-build` job is available in `.github/workflows/engine-comparison-static.yml`. It is disabled by default and does not affect the existing Godot release.

## Set up a trusted Windows build machine

1. Install licensed Unreal Engine **5.6**, Visual Studio C++ build tools and a Windows SDK on a Windows x64 machine with enough disk space for a full engine build.
2. Register that machine as a self-hosted GitHub Actions runner for this repository, label it `unreal-5.6` in addition to the normal `self-hosted`, `Windows`, and `X64` labels. Do not accept untrusted pull-request code on this runner. Restrict who can enable or dispatch builds.
3. Set repository Actions variables:
   - `MOONFALL_UE56_ROOT`: absolute install directory, such as `C:\Program Files\Epic Games\UE_5.6`.
   - `MOONFALL_UNREAL_RUNNER_ENABLED`: `true` when the trusted runner is ready, or unset/false to keep the costly job skipped.
4. After the workflow is merged into the repository's default branch, manually start **Moonfall dual-engine prototype static checks** from GitHub Actions using **Run workflow** on a reviewed branch. The real-engine job is deliberately restricted to `workflow_dispatch`; ordinary pull-request updates never send PR code to your self-hosted runner. Review the selected branch and any changes to the workflow before dispatching it.

The job calls the real `verify-windows-build.ps1` with `-CreateMap -Package`. It requires actual Unreal editor and game compilation, a saved startup map, a newly created package archive, a valid Win64 PE executable and nonempty cooked content. Only if all checks succeed does it upload a 7-day downloadable GitHub Actions artifact named `Moonfall-Unreal56-Win64-Development`.

## Compiler failure diagnostics

If the real-engine job fails, it attempts to upload `Moonfall-Unreal56-Failure-Logs` with project Unreal logs, available crash records, and intermediate Win64 build logs for seven days. Open the failed Actions run, inspect the first compiler error in the job console, then download the diagnostic artifact if it exists. Some failures occur before Unreal creates any project log, in which case the Actions console is the primary diagnostic source. A diagnostic upload is never evidence that the game compiled.

## What this does not prove

A green real-engine job proves compiler and packaging checks passed on that build machine. It **does not** by itself validate runtime combat, controller inputs, imported Z and Nyra artwork, frame time, checkpoint persistence, or installation on Windows 10 and 11. Test those separately before publishing a release. Do not merge the experimental Unreal project into production Godot solely on the basis of CI.
