"""Resolve the live Godot scene's script inheritance, not stale version labels.

A newer gameplay layer may extend v34.4 while keeping all its mechanics. This
helper fails closed on missing scripts, cycles and external paths.
"""
from pathlib import Path
import re

def active_scene_inherits(project: Path, expected_script: str) -> bool:
    project = Path(project)
    scene_path = project / 'scenes/main.tscn'
    if not scene_path.is_file():
        return False
    scene = scene_path.read_text(encoding='utf-8')
    # Exactly one authoritative main scene script (no separate play modes).
    resources = re.findall(r'\[ext_resource type="Script" path="res://(scripts/[^"]+)"', scene)
    if len(resources) != 1 or scene.count('script = ExtResource(') != 1:
        return False
    current = resources[0]
    visited = set()
    for _ in range(64):
        if current in visited or not current.startswith('scripts/') or '..' in current:
            return False
        visited.add(current)
        if Path(current).name == expected_script:
            return (project / current).is_file()
        child = project / current
        if not child.is_file():
            return False
        match = re.search(r'^extends\s+"res://(scripts/[^"]+)"', child.read_text(encoding='utf-8'), re.M)
        if not match:
            return False
        current = match.group(1)
    return False
