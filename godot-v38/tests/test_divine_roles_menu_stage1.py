"""Issue #477 stage 1: new-game menu no longer offers Z/Nyra as avatars."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ROOT / "scripts/native_v11.gd"
CODEX = ROOT / "scripts/systems/tips_codex_v376.gd"


class DivineRolesMenuTests(unittest.TestCase):
    def test_new_game_uses_existing_custom_avatar_creator(self):
        text = NATIVE.read_text(encoding="utf-8")
        menu = text.split("start_screen=_make_menu(", 1)[1].split("func _start_custom_game(", 1)[0]
        self.assertNotIn('"NEW GAME AS "+str(entry["name"])', menu)
        self.assertNotIn('_start_game.bind(str(entry["id"]))', menu)
        self.assertIn('custom_launch.text="CREATE & PLAY"', menu)
        self.assertIn('hero_start_button=custom_launch', menu)
        self.assertIn('_start_custom_game(', menu)
        self.assertIn('Z & NYRA • GODS OF MOONFALL', menu)

    def test_existing_save_path_still_restores_legacy_ids(self):
        text = NATIVE.read_text(encoding="utf-8")
        self.assertIn('hero=selected if selected in ["z","nyra","custom"]', text)
        self.assertIn('_start_game(str(saved["hero"]))', text)
        self.assertIn('hero=str(data.get("hero","z"))', text)
        self.assertIn('SAVE_STORE.commit_checkpoint(', text)

    def test_codex_no_longer_recommends_playing_gods(self):
        text = CODEX.read_text(encoding="utf-8")
        self.assertIn('"title":"Create your hero"', text)
        self.assertNotIn("Pick the hero whose abilities suit you", text)


if __name__ == "__main__":
    unittest.main()
