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
        for token in ("ItemCode != 0 || bCollected || IsActorBeingDestroyed()",
                      "if (InItemCode <= 0) { Destroy(); return; }",
                      "Rarity < 0 || Rarity > 5", "Tier < 1 || Tier > 25", "Destroy();"):
            self.assertIn(token, body)
    def test_world_drop_guid_is_valid_and_write_once(self):
        body = self.source.split("void AMoonfallGroundLoot::SetPersistentDropId(", 1)[1].split(
            "void AMoonfallGroundLoot::InitializeDungeonReward(", 1)[0]
        for token in ("!InId.IsValid()", "bCollected || IsActorBeingDestroyed()",
                      "ItemCode != 0", "PersistentDropId.IsValid()",
                      "RewardZone != INDEX_NONE", "RewardDungeon != INDEX_NONE",
                      "RewardChamber != INDEX_NONE", "PersistentDropId = InId;"):
            self.assertIn(token, body)
            if token != "PersistentDropId = InId;":
                self.assertLess(body.index(token), body.index("PersistentDropId = InId;"))
        header = (SOURCE.parent / "MoonfallGroundLoot.h").read_text(encoding="utf-8")
        self.assertIn("void SetPersistentDropId(const FGuid& InId);", header)
        self.assertNotIn("PersistentDropId = InId;", header)
    def test_dungeon_reward_identity_is_write_once(self):
        body = self.source.split("void AMoonfallGroundLoot::InitializeDungeonReward(", 1)[1].split(
            "bool AMoonfallGroundLoot::TryCollect()", 1)[0]
        for token in ("bCollected || IsActorBeingDestroyed()", "RewardZone != INDEX_NONE",
                      "RewardDungeon != INDEX_NONE", "RewardChamber != INDEX_NONE",
                      "PersistentDropId.IsValid()", "ItemCode <= 0",
                      "RewardZone = Zone;", "RewardDungeon = Dungeon;",
                      "RewardChamber = Chamber;"):
            self.assertIn(token, body)
        self.assertLess(body.index("RewardZone != INDEX_NONE"), body.index("RewardZone = Zone;"))
        self.assertLess(body.index("PersistentDropId.IsValid()"), body.index("RewardZone = Zone;"))
    def test_dungeon_spawn_checks_reward_identity_before_registration(self):
        header = (SOURCE.parent / "MoonfallGroundLoot.h").read_text(encoding="utf-8")
        layout = (SOURCE.parent / "MoonfallDungeonLayout.cpp").read_text(encoding="utf-8")
        treasure = layout.split("bool AMoonfallDungeonLayout::SpawnTreasureChamber(", 1)[1].split(
            "bool AMoonfallDungeonLayout::StartFinalBossChamber(", 1)[0]
        for token in ("HasDungeonRewardIdentity(int32 Zone, int32 Dungeon, int32 Chamber) const",
                      "RewardZone == Zone", "RewardDungeon == Dungeon",
                      "RewardChamber == Chamber", "!PersistentDropId.IsValid()"):
            self.assertIn(token, header)
        for token in ("Loot->InitializeDungeonReward(ZoneIndex, DungeonIndex, ChamberIndex);",
                      "if (!Loot->HasDungeonRewardIdentity(ZoneIndex, DungeonIndex, ChamberIndex)) {",
                      "Loot->Destroy();", "return false;",
                      "SpawnedTreasure[ChamberIndex] = Loot;"):
            self.assertIn(token, treasure)
        self.assertLess(treasure.index("Loot->InitializeDungeonReward("),
                        treasure.index("if (!Loot->HasDungeonRewardIdentity("))
        self.assertLess(treasure.index("if (!Loot->HasDungeonRewardIdentity("),
                        treasure.index("SpawnedTreasure[ChamberIndex] = Loot;"))

    def test_pickup_proximity_visibility_and_persistent_saves(self):
        body = self.source.split("bool AMoonfallGroundLoot::TryCollect()", 1)[1]
        for token in ("HeroPosition.ContainsNaN()", "LootPosition.ContainsNaN()",
                      "!FMath::IsFinite(DistanceSq)", "DistanceSq > FMath::Square(250.f)",
                      "LineTraceSingleByChannel", "ECC_Visibility",
                      "CollectPersistedGroundEquipmentCode", "CollectDungeonTreasureCode",
                      "if (!bSaved) return false;", "bCollected = true",
                      "bCollected || ItemCode <= 0 || IsActorBeingDestroyed()"):
            self.assertIn(token, body)
        self.assertLess(body.index("LineTraceSingleByChannel"), body.index("if (!bSaved)"))
        self.assertLess(body.index("if (!bSaved)"), body.index("bCollected = true"))
if __name__ == "__main__":
    unittest.main()
