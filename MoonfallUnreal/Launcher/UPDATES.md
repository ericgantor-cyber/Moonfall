# Moonfall Unreal launcher updates

The launcher checks GitHub's latest release when it opens and provides an **UPDATES** button for manual checks.

A release is offered only when:
- Its tag is a higher numeric version than the launcher's assembly version (currently 0.1.0).
- It contains an asset named exactly `MoonfallUnreal-Setup.exe`.
- GitHub provides a valid `sha256:` asset digest.
- Its download URL uses HTTPS at `github.com`.

The launcher requests permission before downloading, streams the installer to a temporary directory, verifies SHA-256, then requests a second confirmation before executing the installer. **It never silently installs an update.**

Publish real tested releases with a numeric tag such as `v0.2.0`, attach the packaged Unreal installer under the exact asset name, and update the launcher project's `<Version>` for each build. Keep the installed version consistent with the release tag; the launcher version is used for comparisons. GitHub release assets without a SHA-256 digest or without that exact installer name are not eligible.

The existing self-hosted Unreal game and installer build is **not yet verified**. A complete release pipeline and end-to-end updater test are still needed. The updater detects new versions while the launcher runs; it is not a background Windows service and does not patch individual files.
