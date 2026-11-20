#!/usr/bin/env python3
"""Regression tests for the read-only artwork checker, using only disposable files."""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

CHECKER = Path(__file__).with_name("verify_artwork_inventory.py")
spec = importlib.util.spec_from_file_location("moonfall_art_check", CHECKER)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)

SOURCE = "godot-v38/assets/heroes/test_original.png"
UNITY = "engine-comparison/unity/Assets/Resources/MoonfallArt/heroes/test_original.png"
UNREAL = "engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallArt/heroes/test_original.png"


def sha(data):
    return hashlib.sha256(data).hexdigest()


class ArtworkIntegrityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest_path = self.root / "engine-comparison/ART_IMPORT_MANIFEST.json"
        data = b"approved original art bytes"
        self.source_bytes = data
        self.manifest = {
            "source_count": 1,
            "imported_files": 2,
            "assets": [{
                "source": SOURCE,
                "source_sha256": sha(data),
                "outputs": [
                    {"engine": "unity", "path": UNITY, "sha256": sha(data)},
                    {"engine": "unreal_source_art", "path": UNREAL, "sha256": sha(data)},
                ],
            }],
        }
        for name in (SOURCE, UNITY, UNREAL):
            self.write_file(name, data)

    def write_file(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def run_checker(self, exports=True):
        self.manifest_path.write_text(json.dumps(self.manifest), encoding="utf-8")
        output = io.StringIO()
        args = ["verify_artwork_inventory.py"]
        if exports:
            args.append("--check-exports")
        with patch.object(checker, "ROOT", self.root), patch.object(checker, "MANIFEST", self.manifest_path), patch.object(sys, "argv", args), contextlib.redirect_stdout(output):
            status = checker.check()
        return status, output.getvalue()

    def assert_rejected(self, phrase, exports=True):
        status, output = self.run_checker(exports)
        self.assertEqual(status, 1, output)
        self.assertIn(phrase, output)

    def test_intact_originals_and_exports_pass(self):
        self.assertEqual(self.run_checker()[0], 0)
        self.assertEqual(self.run_checker(False)[0], 0)

    def test_modified_original_is_rejected(self):
        self.write_file(SOURCE, b"altered pixels")
        self.assert_rejected("hash mismatch")

    def test_missing_original_is_rejected(self):
        (self.root / SOURCE).unlink()
        self.assert_rejected("missing or invalid path")

    def test_modified_export_is_rejected(self):
        self.write_file(UNREAL, b"changed converted asset")
        self.assert_rejected("hash mismatch")

    def test_untracked_original_image_is_rejected(self):
        self.write_file("godot-v38/assets/heroes/new_unregistered.png", b"other")
        self.assert_rejected("unmanifested source image")

    def test_duplicate_source_is_rejected(self):
        self.manifest["assets"].append(copy.deepcopy(self.manifest["assets"][0]))
        self.manifest["source_count"] = 2
        self.assert_rejected("duplicate source")

    def test_duplicate_export_is_rejected(self):
        self.manifest["assets"][0]["outputs"][1]["engine"] = "unity"
        self.manifest["assets"][0]["outputs"][1]["path"] = UNITY
        self.assert_rejected("duplicate export destination")

    def test_missing_export_is_rejected_even_without_export_check(self):
        self.manifest["assets"][0]["outputs"].pop()
        self.assert_rejected("expected two conversion exports", False)

    def test_path_traversal_is_rejected(self):
        self.manifest["assets"][0]["source"] = "godot-v38/assets/../../escape.png"
        self.assert_rejected("missing or invalid path")

    def test_missing_art_directory_is_rejected(self):
        for path in (self.root / "godot-v38/assets").rglob("*"):
            if path.is_file():
                path.unlink()
        self.assert_rejected("missing or invalid path")


if __name__ == "__main__":
    unittest.main()
