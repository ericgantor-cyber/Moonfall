"""Source-contract tests for safe world loot initialization and pickup."""
from pathlib import Path
import unittest
SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" /
          "MoonfallUnreal" / "MoonfallGroundLoot.cpp")
class GroundLootCollectionSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
    def test_reject_invalid_uncollectable_actors(self):
        body = self.source.split("void AMoonfallGroundLoot::InitializeLoot(", 1)[1].split(
            "void AMoonfallGroundLoot::InitializeDungeonReward(", 1)[0]
        for token in ("ItemCode != 0 || bCollected",
                      "if (InItemCode <= 0) { Destroy(); return; }",
                      "Rarity < 0 || Rarity > 5", "Tier < 1 || Tier > 25", "Destroy();"):
            self.assertIn(token, body)
    def test_pickup_proximity_visibility_and_persistent_saves(self):
        body = self.source.split("bool AMoonfallGroundLoot::TryCollect()", 1)[1]
        for token in ("HeroPosition.ContainsNaN()", "LootPosition.ContainsNaN()",
                      "!FMath::IsFinite(DistanceSq)", "DistanceSq > FMath::Square(250.f)",
                      "LineTraceSingleByChannel", "ECC_Visibility",
                      "CollectPersistedGroundEquipmentCode", "CollectDungeonTreasureCode",
                      "if (!bSaved) return false;", "bCollected = true"):
            self.assertIn(token, body)
        self.assertLess(body.index("LineTraceSingleByChannel"), body.index("if (!bSaved)"))
        self.assertLess(body.index("if (!bSaved)"), body.index("bCollected = true"))
if __name__ == "__main__":
    unittest.main()
