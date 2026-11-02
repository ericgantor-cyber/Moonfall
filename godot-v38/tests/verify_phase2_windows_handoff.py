"""Conservative source-only Windows handoff preflight; does not build an .exe."""
from pathlib import Path
import configparser,sys
root=Path(__file__).resolve().parents[1]
checks={
"Godot project":(root/"project.godot").is_file(),
"Main scene":(root/"scenes/main.tscn").is_file(),
"Windows export preset":(root/"export_presets.cfg").is_file(),
"Windows build batch":(root/"BUILD_MOONFALL_ONE_CLICK.bat").is_file(),
"Windows build PowerShell":(root/"BUILD_MOONFALL_ONE_CLICK.ps1").is_file(),
"Installer source":any(root.glob("*.iss")),
"Hero artwork":(root/"assets/heroes").is_dir(),
"Enemy AI":(root/"scripts/systems/enemy_combat_rules.gd").is_file(),
"Guardian combat rules":(root/"scripts/systems/guardian_attack_rules.gd").is_file(),
"Phase 2 lifecycle tests":(root/"tests/enemy_search_lifecycle_v448.gd").is_file(),
}
p=configparser.ConfigParser(interpolation=None)
p.read(root/"export_presets.cfg",encoding="utf-8")
checks["Windows x64 target"]=p.get("preset.0","platform",fallback="").strip('"')=="Windows Desktop" and p.get("preset.0.options","binary_format/architecture",fallback="").strip('"')=="x86_64"
checks["Executable output target"]=p.get("preset.0","export_path",fallback="").strip('"').lower().endswith(".exe")
for label,passed in checks.items():print(("PASS" if passed else "FAIL")+": "+label)
print(f"{sum(checks.values())}/{len(checks)} preflight checks passed")
print("Preflight does not verify export, installer, gameplay, or Windows hardware.")
sys.exit(0 if all(checks.values()) else 1)
