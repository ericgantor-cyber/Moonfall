# Moonfall Phase 2 Windows handoff

This repository includes a Windows x64 export preset, installer source, and one-click build scripts. **A source preflight pass is not a built or tested Windows executable.**

From the godot-v38 project folder, run `python tests/verify_phase2_windows_handoff.py` first. On Windows 10/11, follow `README_WINDOWS_ONE_CLICK_v37_9.md` and run `BUILD_MOONFALL_ONE_CLICK.bat`. The script may download Godot templates and other required build tools. Confirm the PowerShell/CI exit status and actually launch and play the resulting Windows build before distributing.

The export preset's v34_4 filename is a legacy name; it does not represent the current gameplay version. Do not rename it casually: older release verification scripts may depend on it.

All 22 existing Phase 2 gameplay suites were previously reported to pass (178 checks). Windows export templates are **not installed** in the Linux verification environment, so no Windows executable or installer was produced or tested here.

Z/Nyra original art and M=Map, Tab=Target Cycle have not been modified.
