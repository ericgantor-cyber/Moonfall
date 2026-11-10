#!/usr/bin/env python3
"""Source-level port contract for Godot inventory capacity rules in UE5.

This does not substitute for native Unreal compilation or live pickup testing.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


def body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth and end < len(source):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    assert depth == 0
    return source[opening + 1 : end - 1]


class InventoryCapacityPort(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.loot = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")

    def test_scripted_reward_fails_before_mutation(self):
        method = body(self.hero, "void AMoonfallHeroPawn::AwardEquipmentDrop(")
        guard = "if (InventoryItemCodes.Num() >= MaxInventoryItems) return;"
        self.assertIn("MaxInventoryItems = 9999;", (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertLess(method.index(guard), method.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(method.index(guard), method.index("LastLootAffix ="))

    def test_ground_collection_fails_without_phantom_receipt(self):
        method = body(self.hero, "bool AMoonfallHeroPawn::CollectGroundEquipmentCode(")
        guard = "if (InventoryItemCodes.Num() >= MaxInventoryItems) return false;"
        self.assertIn("MaxInventoryItems = 9999;", (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertLess(method.index(guard), method.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(method.index(guard), method.index("LastLootAffix ="))

    def test_inventory_hud_displays_capacity_and_full_warning(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("AMoonfallHeroPawn::MaxInventoryItems", hud)
        self.assertIn('TEXT("BAG FULL - salvage unused gear to collect more loot")', hud)

    def test_failed_pickup_keeps_world_actor(self):
        method = body(self.loot, "bool AMoonfallGroundLoot::TryCollect()")
        self.assertLess(
            method.index("if (!Hero->CollectGroundEquipmentCode(ItemCode)) return false;"),
            method.index("bCollected = true;"),
        )
        self.assertLess(method.index("bCollected = true;"), method.index("Destroy();"))


if __name__ == "__main__":
    unittest.main()
