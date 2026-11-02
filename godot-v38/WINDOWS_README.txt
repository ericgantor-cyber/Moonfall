MOONFALL WINDOWS - INSTALLATION AND VERIFICATION

Windows 10/11, 64-bit.

OPTION A: Installer
1. Download Moonfall_Native_v34_4_Windows_Setup.exe from the successful GitHub Actions run.
2. Verify its SHA256 against Moonfall-Windows-Installer.sha256 using:
   Get-FileHash .\Moonfall_Native_v34_4_Windows_Setup.exe -Algorithm SHA256
3. Run the installer. Use its Moonfall Launcher Start Menu shortcut.

OPTION B: Portable ZIP
1. Download Moonfall-Windows-Portable.zip and Moonfall-Windows-Portable.sha256 together.
2. Compare the ZIP SHA256 using:
   Get-FileHash .\Moonfall-Windows-Portable.zip -Algorithm SHA256
3. Extract the ZIP into a normal writable folder. Keep the EXE,
   moonfall-integrity.json, MoonfallLauncher.cmd, and MoonfallLauncher.ps1 together.
4. Run MoonfallLauncher.cmd. Alternatively run MoonfallNative_v34_4.exe directly.

INTEGRITY AND REPAIR
The launcher verifies the EXE against the integrity manifest. If verification
fails, get a fresh copy of the verified installer or portable ZIP. Do not
modify the manifest or executable to bypass a failed check. An integrity hash
only confirms that the file matches the manifest bundled in the same package,
not that the publisher is independently trusted.

SAVES
Do not delete user game data when reinstalling. Keep a backup before replacing
a game build. Launcher settings are stored under %LOCALAPPDATA%\Moonfall.

SUPPORT
Report problems with the GitHub Actions run link and relevant error message:
https://github.com/ericgantor-cyber/Moonfall/issues

This project has CI-based build tests; real hardware, controller, graphics,
and Windows usability tests must still be performed separately.
