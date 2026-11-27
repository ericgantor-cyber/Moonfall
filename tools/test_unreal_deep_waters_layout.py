import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Tools"))
from deep_waters_layout import grotto_layout, grotto_geometry, MIN_HALF_WIDTH, MIN_HALF_LENGTH


class DeepWatersLayoutTests(unittest.TestCase):
    def test_six_unique_rooms(self):
        rooms = grotto_layout()
        self.assertEqual(len(rooms), 6)
        self.assertEqual(len({name for name, _ in rooms}), 6)
        self.assertEqual(len({position for _, position in rooms}), 6)

    def test_geometry_within_pool_and_no_air_pocket_obstruction(self):
        for name, (x, y, z) in grotto_layout():
            for label, (px, py, pz), scale in grotto_geometry(x, y):
                hx, hy, hz = (s * 50 for s in scale)
                self.assertLess(abs(px) + hx, MIN_HALF_WIDTH, (name, label))
                self.assertLess(abs(py) + hy, MIN_HALF_LENGTH, (name, label))
                self.assertLess(pz + hz, 0, (name, label))
                self.assertGreater(pz - hz, -500, (name, label))
                # Sphere-to-box clearance: at least 160 cm radius, floor may be tangent.
                d2 = sum(max(0, abs(a-b)-h)**2 for a,b,h in
                         zip((x,y,z),(px,py,pz),(hx,hy,hz)))
                self.assertGreaterEqual(d2, 150**2, (name, label))

    def test_rooms_do_not_overlap(self):
        rooms = grotto_layout()
        for index, (_, (x,y,_)) in enumerate(rooms):
            for _, (ox,oy,_) in rooms[index+1:]:
                self.assertTrue(abs(x-ox) > 700 or abs(y-oy) > 610)


if __name__ == "__main__":
    unittest.main()
