"""Source regression guard for explicit Unreal ground-loot collection boundaries."""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" /
          "MoonfallUnreal" / "MoonfallGroundLoot.cpp")


class GroundLootCollectionSafety(unittest.TestCase):
    def test_collection_requires_proximity_and_visibility(self):
        source = SOURCE.read_text(encoding="utf-8")
        method = source.split("bool AMoonfallGroundLoot::TryCollect()", 1)[1]
        for required in (
            "HeroPosition.ContainsNaN()",
            "LootPosition.ContainsNaN()",
            "FMath::Abs(LootPosition.Z - HeroPosition.Z) > 150.f",
            "const float DistanceSq = FVector::DistSquared2D(HeroPosition, LootPosition);",
            "!FMath::IsFinite(DistanceSq)",
            "DistanceSq > FMath::Square(250.f)",
            "LineTraceSingleByChannel",
            "ECC_Visibility",
            "SightParams.AddIgnoredActor(Hero)",
            "SightParams.AddIgnoredActor(this)",
        ):
            with self.subTest(required=required):
                self.assertIn(required, method)
        self.assertIn('#include "CollisionQueryParams.h"', source)
        self.assertIn('#include "Engine/World.h"', source)
        self.assertLess(method.index("!FMath::IsFinite(DistanceSq)"),
                        method.index("LineTraceSingleByChannel"))
        self.assertLess(method.index("LineTraceSingleByChannel"),
                        method.index("Hero->CollectGroundEquipmentCode(ItemCode)"))
        self.assertLess(method.index("Hero->CollectGroundEquipmentCode(ItemCode)"),
                        method.index("bCollected = true"))


if __name__ == "__main__":
    unittest.main()
