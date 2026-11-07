# Moonfall Unreal on GitHub-hosted Windows

The repository provides a manually triggered workflow, [Moonfall Unreal GitHub-hosted Windows feasibility and compile](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-hosted-windows.yml). It does **not** require attaching a personal Windows PC.

## First: measure feasibility, no engine download

Open the workflow, select **Run workflow**, and leave **attempt_compile** unchecked. This launches a Windows 2022 hosted runner (or your configured larger hosted runner), verifies Visual Studio 2022 C++ tooling, and reports **free disk space**.

- The workflow measures free space and compares it with **minimum_free_gb** (default **80 GB**, configurable from **35 to 500 GB**). A preflight-only request reports insufficient space; a request with **attempt_compile=true** fails early if the runner is below the threshold. Some UE 5.4 builds and intermediates require more than 80 GB, so adjust this to the actual engine archive and build footprint.
- A successful preflight means that Windows prerequisites and preliminary storage were checked. It does **not** mean Unreal Engine is installed or that Moonfall compiles.

## Optional: configure a larger GitHub-hosted Windows runner

If your GitHub plan and billing permit a larger hosted runner, provision one through your organization or repository runner administration. In repository **Settings > Secrets and variables > Actions > Variables**, set `MOONFALL_HOSTED_RUNNER` to that runner's **exact configured label**. Both the preflight and the compilation jobs use that label; if omitted, they use `windows-2022`.

Larger hosted runners may cost more, and GitHub eligibility and available labels differ by account. Never invent a label; use the one GitHub shows after you provision a runner.

## Optional: real compilation with an authorized UE 5.4 installed build

Unreal Engine is **not** bundled with the standard GitHub-hosted Windows image. The optional compile job expects the owner to provide an authorized UE 5.4 installed build as a ZIP archive stored on a trusted access-controlled HTTPS endpoint. Add repository Actions **Secrets**:

- `MOONFALL_UE54_ARCHIVE_URL`: an authorized HTTPS download URL to the installed-build ZIP with `Engine/Build/BatchFiles/Build.bat` at its archive root.
- `MOONFALL_UE54_ARCHIVE_SHA256`: the verified SHA-256 hash of the **exact ZIP bytes**.

The workflow measures the compilation runner's own free disk space against the configured threshold before downloading, verifies the ZIP hash before extracting, then attempts a **real `MoonfallUnrealEditor Win64 Development` compile** through `Build-Windows.ps1 -CompileOnly`. An archived installed build must be compatible with the target Windows/compiler setup and comply with Epic licensing. Do not upload private engine archives to a public repository. An HTTPS link or matching hash alone does not guarantee source trust.

Select `attempt_compile=true` **only after** capacity, permissions, and these secrets have been prepared. The job will fail clearly if any prerequisite is missing. This workflow does not package an installer, does not automatically provision or purchase runner capacity, and has not yet been used for a verified UE compile.

The existing **self-hosted Unreal Windows build** workflow remains available for packaging, installer creation, and follow-up Windows gameplay testing.

## Overhaul: honest, fail-fast hosted runner gating

- The requested `minimum_free_gb` value defaults to **80**, not the older 35 GB threshold. It is an initial safety gate, not a guarantee that a full Unreal installation will fit.
- **Preflight mode** reports any shortage without attempting an engine download or compilation.
- **Compile mode** fails immediately when the preflight runner cannot meet the disk requirement, then independently rechecks the disk on the actual compile runner (a separate temporary Windows machine).
- Engine archives are hash-verified before extraction; downloads no longer follow redirects. The latter means the private archive URL must resolve directly, without redirecting to a separate download address.
- The workflow still requires manually configured secrets, appropriate licensed engine archives, hosted runner storage, and a real compile to pass. A successful preflight is **not** a successful Unreal build.

## Verified hosted Windows preflight (2026-11-07 UTC)

A real standard `windows-2022` GitHub Actions runner executed the dedicated [Windows hosted runner reality check](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42315908958) successfully. Its log reported **150 GB total disk**, **147.03 GB available**, **Visual Studio 2022 C++ toolchain detected**, and **80 GB preliminary disk threshold passed**.

These values describe **that specific runner instance at that time**, not a guaranteed size or available capacity for future GitHub-hosted machines. Unreal Engine 5.4 was not installed, downloaded, compiled, cooked, or packaged as part of this check. The full build still needs an authorized installed-engine archive supplied through the documented repository secrets (or an alternative supported UE provisioning method), and enough space on the *separate* compile job runner. Do not treat a passing preflight as a UE compilation result.

## Fix-and-progress checks (2026-11-07 UTC)

The hosted compile workflow now requires the preflight job's `has_capacity=true` output before scheduling the costly engine job. It also **rechecks free disk on the separate compile runner after downloading the ZIP**, reads the archive directory to estimate uncompressed size and requires at least **15 GB additional headroom** before extraction. Suspicious archive path entries are rejected. The [hosted Windows reality check](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-hosted-reality-check.yml) now reruns when the hosted compiler workflow changes and validates that these safety checks remain in its source.

These checks avoid known failure modes and are not yet proof the UE ZIP extraction and real Unreal compilation succeed. The managed Windows image remains an ephemeral machine; engine files are not persisted across jobs.

## Reusable engine-archive validation and real Windows security tests

The download job calls `MoonfallUnreal/Build/Test-UnrealInstalledArchive.ps1` before extraction. This verifies the exact ZIP SHA-256, checks for the required `Engine/Build/BatchFiles/Build.bat`, rejects suspicious/duplicate ZIP paths and symbolic links, accounts for the archive's declared expanded size, and enforces another 15 GB of free disk reserve after downloading.

`MoonfallUnreal/Build/Test-UnrealInstalledArchive.Tests.ps1` builds small synthetic ZIP archives and tests valid installs, path traversal (including nested traversal), missing Unreal build files, case-insensitive duplicate paths, and checksum mismatch. These tests run automatically on a **real GitHub-hosted Windows runner** when the hosted build workflow, archive validator or test script changes. [Verified passing test run](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42316458771): six cases passed, with Visual Studio 2022 C++ detected and 146.91 GB free on that particular runner. No actual Unreal 5.4 installation or Moonfall gameplay compilation has happened yet.

## Eleven Windows archive validation tests verified (2026-11-07 UTC)

The shared Unreal installed-build validator also rejects NTFS-reserved DOS device names (including device names with extensions), directory components ending in spaces/dots, empty ZIP path components, and files that collide with parent directory paths. A [real GitHub-hosted Windows runner](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42316628599) passed **11/11 synthetic archive tests**. It reported 146.91 GB free disk and Visual Studio 2022 C++ available. These tests exercise archive validation only; no UE 5.4 engine archive was provisioned and no real game compilation was performed.

## Verified real Windows launcher compilation (2026-11-07 UTC)

The [Windows launcher build workflow](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-launcher-windows.yml) runs on GitHub-hosted `windows-2022`, publishes the **real** `MoonfallLauncher.exe` with .NET 8 for `win-x64`, and checks the launcher process exit codes when game files are missing or when a **synthetic test-only** folder structure is supplied. It writes a SHA-256 digest and uploads a short-lived artifact explicitly named **`MoonfallLauncher-ONLY-NOT-THE-GAME`**. That artifact contains the compiled launcher, NOT any cooked Unreal game or installer. Synthetic game files used in CI are NOT included.

[Verified successful build run](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42316908003): Windows .NET compilation passed; both launcher's file-structure checks passed; binary and checksum artifact uploaded. No UE 5.4 C++ game compilation or Windows installer build occurred.

## NSIS installer template compiled on Windows (2026-11-07 UTC)

The [Moonfall Windows launcher workflow](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-launcher-windows.yml) now additionally compiles the real NSIS installer **script** on GitHub-hosted Windows against a **synthetic test-only** game structure and the successfully compiled launcher. Its test installer is deleted immediately and is **never distributed**. The published artifact contains only the stand-alone launcher executable and SHA-256 hash.

[Verified passing Windows job](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42317142396): launcher compiled, missing-game check passed, synthetic layout check passed, NSIS template compiled, and launcher-only artifact uploaded. This does not verify an Unreal game, real installer contents, installation/uninstallation, or gameplay.

## Verified NSIS installation and uninstallation smoke test (2026-11-07 UTC)

[GitHub-hosted Windows run 42317552438](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42317552438) **passed** after the installer workflow was expanded to execute a silent **test-only NSIS installation** in a dedicated temporary directory, confirm installed launcher/game placeholder files, run `MoonfallLauncher.exe --check-install` from that installed directory, execute silent uninstallation, and check the placeholder game executable was removed. The synthetic installer is **deleted** after testing; it is never distributed as a playable game. Only the verified Windows launcher binary and its SHA-256 checksum are uploaded.

This validates a basic install/uninstall path for the current NSIS script on the hosted runner. It does **not** validate an actual cooked Unreal game, desktop shortcut behavior, real user installs, a final Moonfall release installer, or gameplay. The full Unreal Engine 5.4 build remains pending.

## Verified Windows desktop shortcut and uninstall registration testing (2026-11-07 UTC)

The [GitHub-hosted Windows launcher and installer smoke test](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42317794013) passed after adding checks for both **Desktop** and **Start Menu** shortcuts and their targets, Windows current-user uninstall registration and installation location, the installed launcher's `--check-install` response, and removal of shortcuts and the uninstall registry entry on uninstallation. The workflow refuses to run the test if preexisting Moonfall shortcuts or uninstall registration would be overwritten. The installer contains synthetic test-only game files and is deleted rather than published; the downloadable artifact is only the real launcher and its checksum. The full Unreal C++ game remains uncompiled.

## Fail-fast installer content validation (2026-11-07 UTC)

The NSIS installer now uses supported compile-time `!if /FileExists` checks to reject an incomplete source package before producing an installer. Four Windows negative tests verify that removing any of the following causes NSIS compilation to fail: `MoonfallUnreal.exe`, `MoonfallLauncher.exe`, `Engine/`, or `MoonfallUnreal/`. The successful test build still checks the desktop/Start Menu shortcuts, uninstall registration, installed launcher's `--check-install`, and removal of the synthetic test installation.

[Verified Windows workflow run](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42318297485) passed, and [NSIS installer script verification](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42318120407) passed. The distributed artifact remains the **launcher only**, and the tests do not establish that a real Unreal game has compiled.

## UE 5.4 version and build-tool validation (2026-11-07 UTC)

The hosted compiler job now runs `MoonfallUnreal/Build/Test-UnrealEngineVersion.ps1` immediately after extracting the authorized engine archive. The reusable check requires `Engine/Build/Build.version` with numeric `MajorVersion=5`, `MinorVersion=4`, `Engine/Build/BatchFiles/Build.bat`, and `Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll`. It rejects archives with wrong UE versions or missing toolchain files before attempting Moonfall's actual C++ compilation.

The [hosted Windows validation run](https://github.com/ericgantor-cyber/Moonfall/actions/runs/42318661572) passed **nine new synthetic engine-version/toolchain cases** and the **eleven existing engine-ZIP security cases**, totaling **20 passing tests**. The particular runner reported 219.77 GB free on a 219.98 GB disk, with VS 2022 C++ tooling detected. Neither synthetic test provides an actual licensed engine, and no Moonfall Unreal C++ compilation or playable game installer was produced.

## Optional real Unreal Win64 package on hosted Windows

The manually triggered [hosted Unreal workflow](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-hosted-windows.yml) now has a separate `package_game` checkbox (default **false**). With an authorized UE 5.4 installed-build ZIP available through the private archive URL and SHA-256 secrets, checking `package_game` automatically invokes actual `MoonfallUnrealEditor Win64 Development` compilation and then Unreal AutomationTool `BuildCookRun` for a Win64 Development game package.

The optional packaging job checks that the real `MoonfallUnreal.exe` exists exactly once, the packaged `Engine/` runtime exists, and the `MoonfallUnreal/` game-content directory is not empty. Only after the complete job passes will GitHub upload `MoonfallUnreal-REAL-Game-Windows-Development-UNTESTED`. It is a **development test artifact, not a verified playable game or release installer**. No artificial or synthetic files are added to the uploaded game. The existing `attempt_compile` option still supports compilation without packaging, and both toggles default to false to avoid unexpectedly downloading UE or using build minutes.

**Prerequisites are still unresolved until the repository owner supplies a licensed UE 5.4 installed build.** The automated Windows source/safety tests check the workflow's configuration; they cannot claim game compilation without the engine. Completing a real game package also does not validate gameplay or a finished NSIS installation.
