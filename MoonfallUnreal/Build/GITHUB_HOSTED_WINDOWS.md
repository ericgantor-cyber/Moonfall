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
