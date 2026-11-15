#!/usr/bin/env python3
"""Source-contract tests only; real Godot/Windows mouse testing is still required."""
from pathlib import Path
import unittest

SCRIPT = (Path(__file__).resolve().parents[1] / "godot-v38/scripts/native_v11.gd")


class MouseMenusContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SCRIPT.read_text(encoding="utf-8")

    def test_inventory_right_click_equip_only(self):
        self.assertIn('button.gui_input.connect(_atlas_equip_mouse_input.bind(action))', self.source)
        self.assertIn('if action.begins_with("equip:"):', self.source)
        self.assertIn('event.button_index==MOUSE_BUTTON_RIGHT', self.source)
        self.assertIn('if not menu_open or current_tab!="Inventory":return', self.source)
        self.assertIn('_atlas_execute(action)', self.source)

    def test_clickable_item_selection_and_preview(self):
        self.assertIn('var inventory_selected_index:int=-1', self.source)
        self.assertIn('"inventory_select:"+str(index)', self.source)
        self.assertIn('action.begins_with("inventory_select:")', self.source)
        self.assertIn('inventory_selected_index=selected_index', self.source)
        self.assertIn('[b]SELECTED ITEM[/b]', self.source)
        self.assertIn('if inventory_show_vault:return', self.source)
        self.assertIn('_atlas_execute("equip:"+action.get_slice(":",1))', self.source)
        self.assertIn('if selected_index<0 or selected_index>=inventory_source.size():return', self.source)

    def test_inventory_search_clear_and_mouse_scroll(self):
        self.assertIn('inventory_search_input.clear_button_enabled=true', self.source)
        self.assertIn('inventory_search_input.text_changed.connect(_on_inventory_query_changed)', self.source)
        self.assertIn('action_scroll.follow_focus=true', self.source)
        self.assertIn('scroller.follow_focus=true', self.source)
        self.assertIn('button.focus_mode=Control.FOCUS_ALL', self.source)

    def test_character_and_settings_keep_clickable_tabs(self):
        self.assertIn('tab_button.pressed.connect(_menu_tab.bind(tab))', self.source)
        self.assertIn('for tab in ["Character","World"', self.source)
        self.assertIn('"System"]:', self.source)


if __name__ == "__main__":
    unittest.main()
