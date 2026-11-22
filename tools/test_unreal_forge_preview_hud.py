"""Verify that the live UE5 inventory HUD uses the tested forge preview safely."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class ForgeHUDPreview(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.policy = (ROOT / "MoonfallForgeForecast.h").read_text(encoding="utf-8")

    def test_forecast_uses_selected_item_and_current_salvage(self):
        self.assertIn('#include "MoonfallForgeForecast.h"', self.hud)
        selected = self.hud.split("const int32 SelectedCode = Hero->InventoryItemCodes[Selected];", 1)[1]
        preview = selected.split("// Read-only five-tier projection",1)[1].split("const int32 ReforgeCost",1)[0]
        for item in ("if (PanelHeight >= 500.f)", "MoonfallForgeForecast::Preview(SelectedCode, Hero->Salvage, 5)",
                     "Plan.FinalTier", "Plan.TotalCost", "Plan.Upgrades", "Plan.NextTierCost"):
            self.assertIn(item, preview)
        self.assertNotIn("Salvage -=", preview)
        self.assertNotIn("InventoryItemCodes[", preview)
        self.assertNotIn("UpgradeSelectedInventoryItem(", preview)
        self.assertNotIn("SpawnActor", preview)

    def test_forecast_matches_actual_single_upgrade_cost(self):
        self.assertIn("const int Cost(int Tier, int Rarity)", self.policy.replace("constexpr int Cost(int Tier, int Rarity)", "const int Cost(int Tier, int Rarity)"))
        self.assertIn("4 + Tier * 2 + Rarity * 3", self.policy)
        self.assertIn("const int32 Cost = 4 + Tier * 2 + Rarity * 3;", self.hero)
        self.assertIn("ForgeFiveSelectedInventoryItem", self.hero)

    def test_respects_small_screens_and_keeps_existing_details(self):
        # Check the actual responsive anchors, not obsolete fixed offsets.
        # A double set change takes two lines, above the forge forecast.
        selected = self.hud.split("// A dedicated, read-only selected-item summary", 1)[1]
        details = selected.split("const int32 ReforgeCost", 1)[0]
        self.assertIn("if (PanelHeight >= 500.f)", details)
        self.assertIn("Top + PanelHeight - 230.f", details)
        self.assertIn("Top + PanelHeight - (bTwoSetChanges ? 212.f : 207.f)", details)
        self.assertIn("Top + PanelHeight - 196.f", details)
        self.assertIn("Top + PanelHeight - 177.f", details)
        self.assertIn("if (bTwoSetChanges)", details)
        self.assertIn("PanelWidth < 550.f", details)
        self.assertIn("Top + PanelHeight - 156.f", self.hud)
        self.assertIn("GetProjectedSetPieceCount", details)
        self.assertNotIn("GetAttackWarningRemaining", self.hud)

if __name__ == "__main__":
    unittest.main()
