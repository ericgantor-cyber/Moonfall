"""Regression contract: dungeon atlas must not reveal unexplored geometry.

Run: python -m unittest discover -s tests -p 'test_atlas_fog_contract.py'
"""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "scripts" / "systems" / "navigation_map.gd").read_text(encoding="utf-8")


class AtlasFogContract(unittest.TestCase):
    def test_unexplored_passages_hidden_before_draw(self):
        start = SOURCE.index("for passage in dungeon_passages:", SOURCE.index("func _draw()"))
        end = SOURCE.index("# Draw only authored floor meshes", start)
        passage = SOURCE[start:end]
        self.assertLess(passage.index("if not explored:continue"), passage.index("draw_colored_polygon"))

    def test_unvisited_rooms_hidden_before_geometry_and_labels(self):
        start = SOURCE.index("for room_index in range(dungeon_rooms.size()):", SOURCE.index("func _draw()"))
        end = SOURCE.index("# Local breadcrumbs", start)
        room = SOURCE[start:end]
        self.assertLess(room.index("if not visited:continue"), room.index("draw_circle(room_screen"))
        self.assertLess(room.index("if not visited:continue"), room.index("draw_string(ThemeDB.fallback_font"))


if __name__ == "__main__":
    unittest.main()
