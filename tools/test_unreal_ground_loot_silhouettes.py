"""Source regression: ground-loot silhouettes must not modify pickup semantics."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class GroundLootVisualSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        cls.header = (ROOT / "MoonfallGroundLoot.h").read_text(encoding="utf-8")

    def test_three_equipment_silhouettes(self):
        source = self.cpp.split("void AMoonfallGroundLoot::InitializeLoot",1)[1].split("void AMoonfallGroundLoot::SetPersistentDropId",1)[0]
        for expected in ("if (Slot == 0)", "else if (Slot == 1)", "else {",
                         "PickupDetailA->SetRelativeLocation(", "PickupDetailB->SetRelativeLocation(",
                         "GearCube", "GearSphere", "const float SizeAccent",
                         "const float BeaconWidth", "RarityBeacon->SetRelativeScale3D("):
            self.assertIn(expected, source)

    def test_existing_codes_and_collection_unmodified(self):
        source = self.cpp
        for text in ("InItemCode != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier",
                     "if (ItemCode != 0 || bCollected || IsActorBeingDestroyed()) return;",
                     "PersistentDropId", "CollectPersistedGroundEquipmentCode",
                     "CollectDungeonTreasureCode", "DistanceSq > FMath::Square(250.f)",
                     "LineTraceSingleByChannel", "SetActorEnableCollision(false)"):
            self.assertIn(text, source)

    def test_no_collisions_extra_actors_or_ticking(self):
        source = self.cpp
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", source)
        for name in ("PickupDetailA", "PickupDetailB"):
            self.assertIn("UStaticMeshComponent* " + name, self.header)
            self.assertIn(name + "->SetCollisionEnabled(ECollisionEnabled::NoCollision);", source)
            self.assertIn(name + "->SetHiddenInGame(true);", source)
        self.assertNotIn("SpawnActor", source)
        self.assertNotIn("PrimaryActorTick.bCanEverTick = true;", source)
        self.assertEqual(source.count("PickupMesh->CreateAndSetMaterialInstanceDynamic(0)"), 1)
        self.assertNotIn("PickupDetailA->CreateAndSetMaterialInstanceDynamic", source)
        self.assertNotIn("PickupDetailB->CreateAndSetMaterialInstanceDynamic", source)

if __name__ == "__main__":
    unittest.main()
