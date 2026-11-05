"""Offline contracts for the Moonfall Native Windows launchpad and installer."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
launcher_path = root / "launcher" / "MoonfallLauncher.ps1"
installer_path = root / "Moonfall_Native_v34_4_Setup.iss"
assert launcher_path.is_file(), "Moonfall native launcher script is missing"
launcher = launcher_path.read_text(encoding="utf-8")
installer = installer_path.read_text(encoding="utf-8")

checks = {
    "Native launcher creates a Windows GUI": "System.Windows.Forms.Form" in launcher,
    "Launch uses the correct packaged native game": "MoonfallNative_v34_4.exe" in launcher and "Start-Process -FilePath $game" in launcher,
    "Missing game fails closed": "Test-Path -LiteralPath $game -PathType Leaf" in launcher and "$play.Enabled = $false" in launcher,
    "Launch errors report useful diagnostics": "Launch error" in launcher and "Update-MoonfallStatus" in launcher,
    "File check does not claim cryptographic verification": "does not verify integrity" in launcher,
    "Accessible keyboard and focus support": "AcceptButton = $play" in launcher and "Keys]::F5" in launcher and "AccessibleName" in launcher,
    "Game folder and local log access": "GAME FOLDER" in launcher and "GAME LOGS" in launcher and "Godot\\app_userdata" in launcher,
    "Help points to in-game camera settings": "System > Camera Settings" in launcher,
    "No elevated permissions, account or download loop": all(word not in launcher for word in ("Start-Process -Verb RunAs", "Invoke-WebRequest", "Invoke-RestMethod", "winget ")),
    "Installer bundles launcher": 'Source: "launcher\\MoonfallLauncher.ps1"' in installer,
    "Start menu keeps direct game entry": "Moonfall Native (Direct Play)" in installer,
    "Desktop shortcut opens launcher": 'Name: "{autodesktop}\\Moonfall Native"' in installer,
    "Installer runs launcher after install": 'Description: "Open Moonfall Launcher"' in installer,
    "Installer remains per-user": "PrivilegesRequired=lowest" in installer,
    "Installer game payload remains unchanged": 'Source: "{#BuildDir}\\{#ExeName}"' in installer,
}

for name, passed in checks.items():
    print(("PASS" if passed else "FAIL") + ": " + name)
assert all(checks.values()), "Moonfall launcher packaging contract failed"
