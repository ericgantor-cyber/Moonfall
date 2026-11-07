# Moonfall Unreal on GitHub-hosted Windows

The repository provides a manually triggered workflow, [Moonfall Unreal GitHub-hosted Windows feasibility and compile](https://github.com/ericgantor-cyber/Moonfall/actions/workflows/unreal-hosted-windows.yml). It does **not** require attaching a personal Windows PC.

## First: measure feasibility, no engine download

Open the workflow, select **Run workflow**, and leave **attempt_compile** unchecked. This launches a Windows 2022 hosted runner (or your configured larger hosted runner), verifies Visual Studio 2022 C++ tooling, and reports **free disk space**.

- If free space is below **35 GB**, compilation is blocked *before* any engine download. This is only a preliminary space gate: some UE 5.4 installed builds and intermediate files require considerably more storage.
- A successful preflight means that Windows prerequisites and preliminary storage were checked. It does **not** mean Unreal Engine is installed or that Moonfall compiles.

## Optional: configure a larger GitHub-hosted Windows runner

If your GitHub plan and billing permit a larger hosted runner, provision one through your organization or repository runner administration. In repository **Settings > Secrets and variables > Actions > Variables**, set `MOONFALL_HOSTED_RUNNER` to that runner's **exact configured label**. Both the preflight and the compilation jobs use that label; if omitted, they use `windows-2022`.

Larger hosted runners may cost more, and GitHub eligibility and available labels differ by account. Never invent a label; use the one GitHub shows after you provision a runner.

## Optional: real compilation with an authorized UE 5.4 installed build

Unreal Engine is **not** bundled with the standard GitHub-hosted Windows image. The optional compile job expects the owner to provide an authorized UE 5.4 installed build as a ZIP archive stored on a trusted access-controlled HTTPS endpoint. Add repository Actions **Secrets**:

- `MOONFALL_UE54_ARCHIVE_URL`: an authorized HTTPS download URL to the installed-build ZIP with `Engine/Build/BatchFiles/Build.bat` at its archive root.
- `MOONFALL_UE54_ARCHIVE_SHA256`: the verified SHA-256 hash of the **exact ZIP bytes**.

The workflow checks the 35 GB free-space threshold before downloading, verifies the ZIP hash before extracting, then attempts a **real `MoonfallUnrealEditor Win64 Development` compile** through `Build-Windows.ps1 -CompileOnly`. An archived installed build must be compatible with the target Windows/compiler setup and comply with Epic licensing. Do not upload private engine archives to a public repository. An HTTPS link or matching hash alone does not guarantee source trust.

Select `attempt_compile=true` **only after** capacity, permissions, and these secrets have been prepared. The job will fail clearly if any prerequisite is missing. This workflow does not package an installer, does not automatically provision or purchase runner capacity, and has not yet been used for a verified UE compile.

The existing **self-hosted Unreal Windows build** workflow remains available for packaging, installer creation, and follow-up Windows gameplay testing.
