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
        self.assertLess(passage.index('if not bool(passage.get("explored",false)):continue'), passage.index("passage_near_chart"))
        self.assertLess(passage.index('if not bool(passage.get("explored",false)):continue'), passage.index("draw_colored_polygon"))

    def test_undiscovered_pillars_hidden_before_draw(self):
        start = SOURCE.index("for pillar in dungeon_pillars:", SOURCE.index("func _draw()"))
        end = SOURCE.index("# Visited authored chambers", start)
        pillar = SOURCE[start:end]
        self.assertLess(pillar.index("if absf(pillar_offset.x)>"), pillar.index("for room_index in range(dungeon_rooms.size()):"))
        self.assertLess(pillar.index("if not pillar_discovered:continue"), pillar.index("draw_rect("))
        self.assertIn("for visited_position in discovered_chambers:", pillar)

    def test_unvisited_rooms_hidden_before_geometry_and_labels(self):
        start = SOURCE.index("for room_index in range(dungeon_rooms.size()):", SOURCE.index("func _draw()"))
        end = SOURCE.index("# Local breadcrumbs", start)
        room = SOURCE[start:end]
        self.assertLess(room.index("if not visited:continue"), room.index("draw_circle(room_screen"))
        self.assertLess(room.index("if not visited:continue"), room.index("draw_string(ThemeDB.fallback_font"))


    def test_hidden_rooms_do_not_snap_waypoints(self):
        start = SOURCE.index("if dungeon_active:", SOURCE.index("func _gui_input("))
        end = SOURCE.index("waypoint_selected.emit(location)", start)
        click = SOURCE[start:end]
        self.assertIn("for visited_position in discovered_chambers:", click)
        self.assertLess(click.index("for visited_position in discovered_chambers:"), click.index("location=candidate"))
        self.assertLess(click.index("if not room_discovered:return"), click.index("location=candidate"))
        self.assertIn("room_discovered=true", click)
        self.assertNotIn("if nearest_room>=0:location=dungeon_rooms[nearest_room]", click)


if __name__ == "__main__":
    unittest.main()
