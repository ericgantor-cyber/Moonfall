"""Source checks for the optional Moonfall Windows launcher."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
ps=(root/"MoonfallLauncher.ps1").read_text(encoding="utf-8")
cmd=(root/"MoonfallLauncher.cmd").read_text(encoding="utf-8")
iss=(root/"Moonfall_Native_v34_4_Setup.iss").read_text(encoding="utf-8")
checks={
 "Launcher Play starts shipped game": "MoonfallNative_v34_4.exe" in ps and "Start-Process -FilePath $game" in ps,
 "Fullscreen option persists": "launcher-settings.json" in ps and "--fullscreen" in ps and "--windowed" in ps,
 "Update action opens GitHub releases": "https://github.com/ericgantor-cyber/Moonfall/releases" in ps,
 "Duplicate game launch guard": "Get-Process -Name" in ps and "already running" in ps,
 "Official HTTPS release URL": "officialRelease.Scheme -ne 'https'" in ps and "officialRelease.Host -ne 'github.com'" in ps,
 "Release notes and asset preview": "latest.body" in ps and "latest.assets" in ps,
 "Manual updates only": "Downloads and installation remain manual." in ps,
 "SHA256 integrity manifest before Play": "moonfall-integrity.json" in ps and "Get-FileHash" in ps and "SHA256" in ps,
 "Report rejects invalid hash matches": "Matches: " in ps and "Expected SHA256:" in ps and "Actual SHA256:" in ps,
 "Open repair report directory": "Open the saved repair report folder?" in ps and "Start-Process -FilePath explorer.exe" in ps,
 "Persistent repair reports": "Join-Path $settingsDir 'Reports'" in ps and "New-Item -ItemType Directory -Force -Path $reportsDir" in ps,
 "Local repair diagnostics": "Moonfall-Integrity-Report-" in ps and "Expected SHA256:" in ps and "Actual SHA256:" in ps,
 "Safe repair guidance exists": "Show safe Moonfall game repair instructions" in ps and "Back up your saved games." in ps and "does not automatically replace files" in ps,
 "Verify button visible and accessible": "Verify game files" in ps and "Verify Moonfall installed game files" in ps,
 "Verify button checks SHA256": "$verify.Add_Click({" in ps and "$hash -ine $manifest.game_sha256" in ps,
 "Verify button retains saves": "Saved games were not changed." in ps,
 "X64 PE integrity check": "0x00004550" in ps and "0x8664" in ps and "0x3c" in ps,
 "Missing game disables Play": "if (Test-Path -LiteralPath $game -PathType Leaf)" in ps and "$play.Enabled = $false" in ps,
 "Keyboard focus starts on Play": "$form.Add_Shown(" in ps and "$play.Focus()" in ps,
 "Controller support handled by game": "InputEventJoypad" not in ps,
 "Batch launcher calls PowerShell": "MoonfallLauncher.ps1" in cmd,
 "Installer includes launcher files": 'Source: "MoonfallLauncher.ps1"' in iss and 'Source: "MoonfallLauncher.cmd"' in iss,
 "Installer retains direct game shortcut": 'Name: "{autoprograms}\\Moonfall Native"' in iss,
 "Installer adds launcher shortcut": 'Name: "{autoprograms}\\Moonfall Launcher"' in iss,
}
# Fail closed if a merge accidentally appends duplicate test blocks or stray source.
assert __name__ == "__main__", "Run this regression script directly"
assert list(checks).count("Local repair diagnostics") == 1
for name,ok in checks.items(): print(("PASS" if ok else "FAIL")+": "+name)
raise SystemExit(0 if all(checks.values()) else 1)
