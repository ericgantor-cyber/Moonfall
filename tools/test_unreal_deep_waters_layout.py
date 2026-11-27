import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Tools"))
from deep_waters_layout import grotto_layout, grotto_geometry, MIN_HALF_WIDTH, MIN_HALF_LENGTH, grotto_art, GROTTO_ART, shoreline_geometry, ruin_geometry


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

    def test_art_landmarks_and_lights(self):
        self.assertEqual(len(GROTTO_ART), 6)
        self.assertEqual(len({entry[2] for entry in GROTTO_ART}), 6)
        for index, (_, (x, y, z)) in enumerate(grotto_layout()):
            ornaments, light, color = grotto_art(index, x, y)
            self.assertEqual(len(ornaments), 4)
            self.assertEqual(len({item[0] for item in ornaments}), 4)
            self.assertTrue(all(0 <= value <= 1 for value in color))
            self.assertLess(light[2], 0)
            for _, mesh, (px, py, pz), scale in ornaments:
                self.assertIn(mesh, ("Cone", "Sphere", "Cylinder"))
                self.assertLess(abs(px) + scale[0] * 50, MIN_HALF_WIDTH)
                self.assertLess(abs(py) + scale[1] * 50, MIN_HALF_LENGTH)
                self.assertLess(pz + scale[2] * 50, 0)
                self.assertGreater(pz - scale[2] * 50, -500)

    def test_shoreline_is_continuous_and_step_height_is_bounded(self):
        for width in (MIN_HALF_WIDTH, 4000.0):
            steps = shoreline_geometry(width)
            self.assertEqual(len(steps), 4)
            for (_, (x, _, z), scale), (_, (nx, _, nz), next_scale) in zip(steps, steps[1:]):
                self.assertLessEqual(nx - next_scale[0]*50, x + scale[0]*50)
                self.assertEqual(z - nz, 25.0)
            self.assertEqual(steps[0][1][0] + steps[0][2][0]*50, -width)

    def test_ruin_landmarks_clear_of_grottoes_and_current_lanes(self):
        ruins = ruin_geometry()
        self.assertEqual(len(ruins), 4)
        for _, (x, y, z), scale in ruins:
            self.assertGreater(abs(x)-scale[0]*50, 1100)
            self.assertLess(abs(x)+scale[0]*50, MIN_HALF_WIDTH)
            self.assertLess(abs(y)+scale[1]*50, MIN_HALF_LENGTH)
            self.assertGreaterEqual(z-scale[2]*50, -430)
            for _, (gx, gy, _) in grotto_layout():
                self.assertTrue(abs(x-gx)>350+scale[0]*50 or abs(y-gy)>305+scale[1]*50)

    def test_rooms_do_not_overlap(self):
        rooms = grotto_layout()
        for index, (_, (x,y,_)) in enumerate(rooms):
            for _, (ox,oy,_) in rooms[index+1:]:
                self.assertTrue(abs(x-ox) > 700 or abs(y-oy) > 610)


if __name__ == "__main__":
    unittest.main()
