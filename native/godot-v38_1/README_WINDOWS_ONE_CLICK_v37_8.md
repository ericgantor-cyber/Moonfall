# Moonfall v37.8: one-click Windows release build

**Game source:** v37.7. **Export executable name:** remains the historical `MoonfallNative_v34_4.exe` because changing it before a successful Windows release would break legacy setup checks. The game inside has v37.7 source features.

1. Extract the source ZIP to a local folder on Windows 10/11.
2. Double-click `BUILD_MOONFALL_ONE_CLICK.bat`. It now auto-installs Python 3 and Inno Setup 6 when missing, using WinGet (see `README_WINDOWS_ONE_CLICK_v37_9.md`). Internet is required for initial downloads.
3. The launcher downloads the official Godot 4.7.2 Windows editor (checks SHA-256), downloads official Windows export templates (also SHA-256 checked, first download approximately 1.3 GB), runs all 39 offline checks and all 65 Godot gameplay suites, then exports and headless-tests an embedded PCK `.exe`.
4. In normal mode it now ensures Inno Setup 6 is installed and requires a successful installer build with the desktop-shortcut option. Only `-PortableOnly` skips the installer.
5. The launcher writes `build/BUILD_STATUS_v37_9.json` with hashes **only if the executable passes its scripted gates**; if a required step fails, it exits nonzero and does not write success evidence.

**Not yet tested here:** local Windows launcher execution, 65 Godot suites, 4K GPU FPS, gamepad, combat or real install/uninstall. A successful automated check is not the same as human gameplay validation.

## GitHub alternative

The original `PUBLISH_NATIVE_TO_GITHUB.bat` can stage the complete Godot project into a separate branch of `ericgantor-cyber/Moonfall` (requires authenticated Git on Windows). Open the resulting PR to let the Windows runner test and produce artifacts. The original browser game and DirectX prototype remain untouched. Do not claim that the GitHub workflow ran until it actually reports success.
