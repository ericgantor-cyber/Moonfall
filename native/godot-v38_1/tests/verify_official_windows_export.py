#!/usr/bin/env python3
"""Build pipeline regression: verifies release templates and proper Godot export, not execution."""
from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
installer=(r/'INSTALL_GODOT_WINDOWS_TEMPLATES.ps1').read_text()
bat=(r/'BUILD_WINDOWS.bat').read_text()
config=(r/'export_presets.cfg').read_text()
workflows=[(r/'.github/workflows/windows-v34_4.yml').read_text(), (r/'tools/github_windows_v34_4.yml').read_text()]
checks={
    'official export template download': 'Godot_v$Version-stable_export_templates.tpz' in installer,
    'official SHA256 checked': 'f298490b8d44d934be425a5a65a51bf15f422428b229a06a6e11d9ffea248011' in installer and 'Get-FileHash' in installer,
    'only required Windows x64 templates extracted': all(x in installer for x in ('templates/windows_release_x86_64.exe','templates/windows_debug_x86_64.exe')), 
    'runtime template installed to Godot expected directory': 'Godot\export_templates\$exportVersion' in installer,
    'Windows batch requires template': 'GODOT_TEMPLATE' in bat and 'if not exist "%GODOT_TEMPLATE%"' in bat,
    'Godot official release exported': '--export-release "Windows Desktop"' in bat,
    'no manual editor executable embedding': 'EMBED_SINGLE_EXE.ps1' not in bat,
    'PCK embedded by export preset': 'binary_format/embed_pck=true' in config,
    'both CI workflows install template': all('INSTALL_GODOT_WINDOWS_TEMPLATES.ps1' in w and 'actions/cache@v4' in w for w in workflows),
    'runtime 4K regression retained': 'v34_4_4k_display.gd' in bat,
    'sandboxed test profile can still export': 'GODOT_TEMPLATE_SOURCE' in bat and 'GODOT_TEMPLATE_DIR' in bat and 'copy /y' in bat,
    'existing signed release executable still checked': 'GODOT_PACKED_LOG.txt' in bat,
}
for label,good in checks.items():print(('PASS' if good else 'FAIL')+': '+label)
sys.exit(0 if all(checks.values()) else 1)
