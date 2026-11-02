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
 "SHA256 integrity manifest before Play": "moonfall-integrity.json" in ps and "Get-FileHash" in ps and "SHA256" in ps,
 "X64 PE integrity check": "0x00004550" in ps and "0x8664" in ps and "0x3c" in ps,
 "Missing game disables Play": "if (Test-Path -LiteralPath $game -PathType Leaf)" in ps and "$play.Enabled = $false" in ps,
 "Keyboard focus starts on Play": "$form.Add_Shown(" in ps and "$play.Focus()" in ps,
 "Controller support handled by game": "InputEventJoypad" not in ps,
 "Batch launcher calls PowerShell": "MoonfallLauncher.ps1" in cmd,
 "Installer includes launcher files": 'Source: "MoonfallLauncher.ps1"' in iss and 'Source: "MoonfallLauncher.cmd"' in iss,
 "Installer retains direct game shortcut": 'Name: "{autoprograms}\\Moonfall Native"' in iss,
 "Installer adds launcher shortcut": 'Name: "{autoprograms}\\Moonfall Launcher"' in iss,
}
for name,ok in checks.items(): print(("PASS" if ok else "FAIL")+": "+name)
raise SystemExit(0 if all(checks.values()) else 1)
