from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
w=(r/'.github/workflows/windows-v34_4.yml').read_text()
checks={
 "Pull-request trigger": "pull_request:" in w,
 "Manual workflow trigger": "workflow_dispatch:" in w,
 "Windows runner": "runs-on: windows-2022" in w,
 "Runtime smoke test": "--quit-after 180" in w,
 "Install and uninstall": "unins000.exe" in w and "/VERYSILENT" in w,
 "Failure diagnostics": "installed-run.log" in w and "GODOT_*_LOG.txt" in w,
 "No old workflow": not (r/'.github/workflows/windows-v33_6.yml').exists(),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+' '+label)
sys.exit(0 if all(checks.values()) else 1)
