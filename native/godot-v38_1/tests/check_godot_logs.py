#!/usr/bin/env python3
"""Fail the Windows release if any Godot log contains errors, even after a PASS.

The Windows release runner writes a deterministic set of logs. Unlike a shell
wildcard, reading names from BUILD_WINDOWS.bat also detects missing log files.
This script does not claim to execute the engine or validate actual gameplay.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
BATCH = ROOT / "BUILD_WINDOWS.bat"
# Engine logs print these on parse/runtime failures, sometimes without a failing
# process exit status. Never rely on a PASS banner alone.
ERRORS = re.compile(
    r"(?i)(?:\bSCRIPT ERROR\b|\bParse Error\b|\bERROR:|\bFAILED:|\bFATAL:|\bCRASH:|\bSegmentation fault\b)"
)
LOG_PATTERN = re.compile(r">\s*(GODOT_[A-Z0-9_]+_LOG\.txt)\s+2>&1", re.I)


def expected_release_logs(batch_text: str) -> list[str]:
    logs = LOG_PATTERN.findall(batch_text)
    # Ordered de-duplication detects accidental overwrite or duplicate log names.
    if len(logs) != len(set(logs)):
        raise ValueError("Build script writes multiple engine processes to the same log")
    if len(logs) < 63:  # import + 60 suites + export + packed startup
        raise ValueError(f"Expected import, at least 60 suites, export and packed logs; found {len(logs)}")
    for required in ("GODOT_IMPORT_LOG.txt", "GODOT_EXPORT_LOG.txt", "GODOT_PACKED_LOG.txt"):
        if required not in logs:
            raise ValueError(f"Missing release stage log: {required}")
    return logs


def scan(paths: list[Path]) -> list[str]:
    problems: list[str] = []
    for path in paths:
        if not path.is_file():
            problems.append(f"MISSING {path}")
            continue
        content = path.read_text(encoding="utf-8", errors="replace")
        for line_number, line in enumerate(content.splitlines(), 1):
            if ERRORS.search(line):
                problems.append(f"{path}:{line_number}: {line.strip()[:250]}")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--files", nargs="+", type=Path,
                        help="Check explicit log files (used by isolated regression tests)")
    args = parser.parse_args(argv)
    if args.files is not None:
        files = args.files
    else:
        try:
            files = [ROOT / name for name in expected_release_logs(BATCH.read_text(encoding="utf-8"))]
        except ValueError as error:
            print(f"FAIL: {error}", file=sys.stderr)
            return 1
    problems = scan(files)
    for issue in problems:
        print("FAIL:", issue, file=sys.stderr)
    if problems:
        print(f"FAIL: {len(problems)} Godot diagnostic issue(s); release blocked", file=sys.stderr)
        return 1
    print(f"PASS: {len(files)} Godot logs present with no parse or runtime errors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
