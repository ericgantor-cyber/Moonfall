#!/usr/bin/env python3
"""Guard the Godot Windows prerelease against broken automatic handoffs."""
from pathlib import Path

workflow = (Path(__file__).resolve().parents[2] / ".github/workflows/godot-v38-5-prerelease.yml").read_text(encoding="utf-8")
run_source = "github.event_name == 'workflow_run' && github.event.workflow_run.id || inputs.source_run_id"
tag_source = "github.event_name == 'workflow_run' && 'v38.5-rc1' || inputs.release_tag"
checks = {
    "validated and downloaded run IDs use the same event-aware source": workflow.count("RUN_ID: ${{ " + run_source + " }}") == 2,
    "all release stages use the same event-aware tag": workflow.count("RELEASE_TAG: ${{ " + tag_source + " }}") == 3,
    "installer archive tracks selected tag": '"../../Moonfall-${RELEASE_TAG}-Windows-Installer.zip"' in workflow,
    "portable archive tracks selected tag": '"../../Moonfall-${RELEASE_TAG}-Windows-Portable.zip"' in workflow,
    "published archives track selected tag": 'gh release create "$RELEASE_TAG" "Moonfall-${RELEASE_TAG}-Windows-Installer.zip" "Moonfall-${RELEASE_TAG}-Windows-Portable.zip"' in workflow,
    "release remains a prerelease": "--target main --prerelease" in workflow,
    "source run must be successful": '"$RUN_ID" --jq .conclusion' in workflow or '/actions/runs/$RUN_ID" --jq .conclusion' in workflow,
}
for label, good in checks.items():
    print(("PASS" if good else "FAIL") + " prerelease: " + label)
raise SystemExit(0 if all(checks.values()) else 1)
