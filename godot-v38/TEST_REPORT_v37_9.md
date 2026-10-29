# Moonfall v37.9: Automated Python 3 and Inno Setup 6 bootstrap

## Completed source changes
- One-click PowerShell build script auto-detects Python 3 or installs WinGet `Python.Python.3.13`.
- Detects machine/user Inno Setup 6 or installs WinGet `JRSoftware.InnoSetup` with version 6.7.3.
- Checks Python major version and Inno compiler's file version before use.
- Ensures the previously running PowerShell session can resolve the new Python path for legacy batch tests.
- Fails closed on missing WinGet, installer failures, compiler failure, failed Godot checks or missing outputs. A normal build requires both EXE and installer; `-PortableOnly` explicitly opts out of Inno.
- The original Godot image assets and active scene were not changed.
- New source verification script in both Windows CI manifest paths.

## Executed verification (non-Windows)
- 39/39 offline `tests/verify_*.py` scripts passed. The run was split due to execution time limits (35 + 4).
- New v37.9 bootstrap contract: 18/18 checks passed.
- Original 70 image SHA256 digests match the existing manifest.
- ZIP integrity checked after packaging.

## Not executed
- WinGet or installation of Python 3 / Inno Setup 6 on a Windows machine.
- PowerShell native runtime checks (PowerShell is unavailable in this environment).
- Godot 4.7.2 gameplay tests, Windows EXE generation, Inno compilation, actual installer behavior, 4K FPS or controller checks.

**Game source** v37.7; **build-hand-off tooling** v37.9. There is no compiled Windows installer in this ZIP.
