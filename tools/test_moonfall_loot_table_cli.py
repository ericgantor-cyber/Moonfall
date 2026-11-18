"""Source-level Python checks for the offline loot table validator and odds CLI.

These test deterministic probability math, not a real UE5.4 native build.
"""
import csv
import io
from pathlib import Path
import tempfile
import unittest

from tools import moonfall_loot_table as loot


class LootTableEditorContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = loot.load_rows(loot.DEFAULT_CSV)

    def test_default_data_and_rarity_floor(self):
        self.assertEqual(len(self.rows), 41)
        self.assertEqual(loot.rarity_weights(self.rows, "Boss", 0, 60),
                         (108, [0, 0, 0, 83, 20, 5]))
        self.assertEqual(loot.rarity_weights(self.rows, "Treasure", 0, 1),
                         (98, [0, 0, 60, 38, 0, 0]))

    def test_slot_and_trait_odds_match_selected_row_math(self):
        odds = loot.detailed_odds(self.rows, "Boss", 0, 60)
        self.assertEqual(odds["total"], 108)
        self.assertEqual(odds["rarity"], [0, 0, 0, 83, 20, 5])
        self.assertAlmostEqual(sum(odds["slots"]), odds["total"])
        self.assertAlmostEqual(sum(odds["traits"]), odds["total"])
        self.assertAlmostEqual(odds["slots"][0], 100 / 3 + 8)
        self.assertAlmostEqual(odds["traits"][1], 100 / 3 + 8)
        self.assertEqual(odds["traits"][0], 0)

    def test_regular_uncommon_has_chance_to_be_untraited(self):
        odds = loot.detailed_odds(self.rows, "Regular", 0, 1)
        self.assertAlmostEqual(odds["traits"][0], 64.4)
        self.assertAlmostEqual(sum(odds["traits"]), odds["total"])

    def test_row_names_follow_editor_and_unreal_fname_rules(self):
        rows = [dict(value) for value in self.rows]
        rows[1]["Name"] = rows[0]["Name"].lower()
        with self.assertRaisesRegex(ValueError, "case-insensitive"):
            loot.validate_rows(rows)
        rows[1]["Name"] = "not a valid name"
        with self.assertRaisesRegex(ValueError, "Invalid loot row name"):
            loot.validate_rows(rows)

    def test_csv_column_order_is_contractual(self):
        data = {key: self.rows[0][key] for key in reversed(loot.FIELDS)}
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / "reordered.csv"
            with file.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=list(reversed(loot.FIELDS)))
                writer.writeheader()
                writer.writerow(data)
            with self.assertRaisesRegex(ValueError, "header and order"):
                loot.load_rows(file)

    def test_cap_and_sorted_row_ties(self):
        # Sorting determines deterministic seeded dungeon treasure.
        rows = [dict(row) for row in self.rows]
        for i in range(125):
            rows.append({"Name": f"Boss_Custom_{i:03d}", "Source": "Boss",
                         "ZoneIndex": -1, "MinimumLevel": 1,
                         "Rarity": 3, "Weight": 10000,
                         "SlotMask": 7, "ForcedTrait": 0})
        valid = loot.validate_rows(rows)
        total, rarity = loot.rarity_weights(valid, "Boss", -1, 60)
        odds = loot.detailed_odds(valid, "Boss", -1, 60)
        self.assertEqual(total, 1_000_000)
        self.assertEqual(sum(rarity), total)
        self.assertEqual(odds["total"], total)
        self.assertEqual(odds["rarity"], rarity)


if __name__ == "__main__":
    unittest.main()
