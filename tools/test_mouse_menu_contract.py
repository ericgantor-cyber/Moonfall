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
        self.assertIn('[b]ITEM INSPECTION[/b]', self.source)
        self.assertIn('if inventory_show_vault:\n    get_viewport().set_input_as_handled()', self.source)
        self.assertIn('_atlas_execute("equip:"+action.get_slice(":",1))', self.source)
        self.assertIn('if selected_index<0 or selected_index>=inventory_source.size():return', self.source)

    def test_wow_style_item_inspection(self):
        self.assertIn('func _inventory_rarity_color(rarity:String)->String:', self.source)
        self.assertIn('"Legendary":return', self.source)
        self.assertIn('inventory_selected_index==index', self.source)
        self.assertIn('[b]ITEM INSPECTION[/b]', self.source)
        self.assertIn('[b]COMPARE WITH EQUIPPED[/b]', self.source)
        self.assertIn('selected_power-equipped_power', self.source)
        self.assertIn('In vault • Withdraw to compare or equip.', self.source)

    def test_large_inventory_grid_and_selected_actions(self):
        self.assertIn('var item_grid:=GridContainer.new()', self.source)
        self.assertIn('item_grid.columns=2', self.source)
        self.assertIn('_inventory_tile(item_grid,gear,index)', self.source)
        self.assertIn('tile.custom_minimum_size=Vector2(124,92)', self.source)
        self.assertIn('item_grid.size_flags_horizontal=Control.SIZE_EXPAND_FILL', self.source)
        self.assertIn('tile.focus_mode=Control.FOCUS_ALL', self.source)
        self.assertIn('normal_style.set_border_width_all(3 if selected else 1)', self.source)
        self.assertIn('normal_style.duplicate() as StyleBoxFlat', self.source)
        self.assertIn('tile.add_theme_stylebox_override("hover",hover_style)', self.source)
        self.assertIn('Compared with equipped:', self.source)
        self.assertIn('tile.pressed.connect(_atlas_execute.bind("inventory_select:"+str(index)))', self.source)
        self.assertIn('tile.gui_input.connect(_atlas_equip_mouse_input.bind("inventory_select:"+str(index)))', self.source)
        self.assertIn('"inventory_select_next","inventory_select_prev"', self.source)
        self.assertIn('inventory_selected_index=matching[selected_at]', self.source)
        self.assertIn('"EQUIP SELECTED: "', self.source)
        self.assertIn('"DEPOSIT SELECTED"', self.source)
        self.assertIn('"WITHDRAW SELECTED: "', self.source)
        self.assertNotIn('_atlas_action("DEPOSIT: "+str(gear.get(', self.source)

    def test_new_hero_and_load_clear_item_selection(self):
        self.assertIn('inventory_query=""\n inventory_selected_index=-1\n inventory_restore_focus=false\n inventory_page=0', self.source)
        self.assertIn('func _restore_data(data: Dictionary) -> void:\n # UI-only selection', self.source)
        self.assertIn('inventory_selected_index=-1\n inventory_restore_focus=false\n inventory_page=0\n inventory_show_vault=false', self.source)

    def test_vault_right_click_is_consumed_without_equipping(self):
        self.assertIn('if inventory_show_vault:\n    get_viewport().set_input_as_handled()\n    return', self.source)
        self.assertIn('if not menu_open or current_tab!="Inventory":return', self.source)

    def test_selection_restores_focus_after_grid_refresh(self):
        self.assertIn('var inventory_restore_focus:bool=false', self.source)
        self.assertIn('inventory_restore_focus=true', self.source)
        self.assertIn('if selected and inventory_restore_focus:tile.call_deferred("grab_focus")', self.source)
        self.assertIn('inventory_restore_focus=false', self.source)

    def test_equipped_gear_cannot_be_deposited(self):
        deposit = self.source.split('if action.begins_with("inventory_deposit:"):', 1)[1].split('elif action.begins_with("inventory_withdraw:"):', 1)[0]
        self.assertIn("if idx<0 or idx>=gear_items.size():", deposit)
        self.assertIn("if not equipped.is_empty() and equipped==gear_items[idx]:", deposit)
        self.assertIn("Unequip that item before depositing it.", deposit)
        self.assertLess(deposit.index("if not equipped.is_empty()"), deposit.index("INVENTORY_RULES.transfer_gear("))

    def test_selling_gear_resets_inventory_selection(self):
        sale = self.source.split('elif action.begins_with("wallet_sell:"):', 1)[1].split('func _create_coin_drop(', 1)[0]
        self.assertIn("gear_items.remove_at(idx)", sale)
        self.assertIn("inventory_selected_index=-1", sale)
        self.assertLess(sale.index("gear_items.remove_at(idx)"), sale.index("inventory_selected_index=-1"))
        self.assertIn("inventory_restore_focus=false", sale)

    def test_equipped_item_status_and_action_guards(self):
        self.assertIn('(" • EQUIPPED" if is_equipped else "")', self.source)
        self.assertIn('if not equipped.is_empty() and equipped==selected_item:', self.source)
        self.assertIn('button.disabled=true', self.source)
        self.assertIn('action.begins_with("inventory_deposit:")', self.source)
        self.assertIn('not equipped.is_empty() and equipped==gear_items[item_index]', self.source)

    def test_disabled_actions_keep_truthful_tooltips(self):
        self.assertIn('if not button.disabled:', self.source)
        self.assertIn('Cannot deposit equipped gear. Equip another item first.', self.source)
        self.assertIn('CURRENTLY EQUIPPED', self.source)
        self.assertIn('This equipment is already active.', self.source)
        self.assertIn('Right-click its tile to equip or use Equip Selected.', self.source)

    def test_equipped_bonus_comparison(self):
        self.assertIn("func _inventory_bonus_label(item:Dictionary)->String:", self.source)
        self.assertIn("[b]BONUS COMPARISON[/b]", self.source)
        self.assertIn("Equipped bonus: ", self.source)
        self.assertIn("Selected bonus: ", self.source)
        self.assertIn("chosen_affix==current_affix", self.source)
        self.assertIn("Same bonus difference:", self.source)
        self.assertIn("Different bonus types; compare their effects", self.source)
        self.assertIn("One item has no bonus; review its overall power and effects.", self.source)
        self.assertIn("You are already wearing this item.", self.source)

    def test_inventory_tiles_show_power_delta(self):
        self.assertIn('var difference:int=int(gear.get("power",0))-_equipment_power()', self.source)
        self.assertIn('elif difference>0:comparison="▲ +"', self.source)
        self.assertIn('elif difference<0:comparison="▼ "', self.source)
        self.assertIn('else:comparison="= same power"', self.source)
        self.assertIn('if is_equipped:comparison="EQUIPPED"', self.source)
        self.assertIn('if not inventory_show_vault:', self.source)

    def test_best_power_selection_respects_filters_and_never_auto_equips(self):
        section = self.source.split('  "inventory_select_best":', 1)[1].split('  "inventory_select_next","inventory_select_prev":', 1)[0]
        self.assertIn("if inventory_show_vault:return", section)
        self.assertIn("INVENTORY_RULES.matching_gear_indices(gear_items,inventory_query,inventory_filter,inventory_sort)", section)
        self.assertIn('int(gear_items[index].get("power",0))>int(gear_items[best_index].get("power",0))', section)
        self.assertIn("inventory_selected_index=best_index", section)
        self.assertIn("inventory_restore_focus=true", section)
        self.assertNotIn("_equip(", section)
        self.assertIn('"FIND BEST POWER","inventory_select_best"', self.source)

    def test_find_upgrade_above_current_power_without_auto_equip(self):
        section = self.source.split('  "inventory_select_upgrade":', 1)[1].split('  "inventory_select_next","inventory_select_prev":', 1)[0]
        self.assertIn("if inventory_show_vault:return", section)
        self.assertIn("INVENTORY_RULES.matching_gear_indices(gear_items,inventory_query,inventory_filter,inventory_sort)", section)
        self.assertIn("candidate_power<=equipped_power:continue", section)
        self.assertIn("candidate_power>int(gear_items[upgrade_index].get", section)
        self.assertIn('No matching power upgrades found.', section)
        self.assertIn('inventory_selected_index=upgrade_index', section)
        self.assertIn("inventory_restore_focus=true", section)
        self.assertNotIn("_equip(", section)
        self.assertIn('"FIND UPGRADE","inventory_select_upgrade"', self.source)

    def test_all_gear_tiles_have_full_hover_information(self):
        self.assertIn('tile.tooltip_text=_inventory_hover_details(gear,inventory_show_vault,is_equipped)', self.source)
        hover = self.source.split('func _inventory_hover_details(item:Dictionary,in_vault:bool,is_equipped:bool)->String:', 1)[1].split('func _inventory_bonus_label(', 1)[0]
        self.assertIn('rarity+" • Item Power "', hover)
        self.assertIn('detail+="\\nBonus: "+_inventory_bonus_label(item)', hover)
        self.assertIn('Location: Personal Vault', hover)
        self.assertIn('Location: Equipment Bag', hover)
        self.assertIn('CURRENTLY EQUIPPED', hover)
        self.assertIn('Power change:', hover)
        self.assertIn('Same bonus change:', hover)
        self.assertIn('Withdraw this item to equip it.', hover)

    def test_material_recipe_and_consumable_hover_info(self):
        self.assertIn('_material_hover_row(str(key),amount,stacks.size())', self.source)
        self.assertIn('func _material_hover_row(name:String,amount:int,stack_count:int)->void:', self.source)
        self.assertIn('row.tooltip_text=name+', self.source)
        self.assertIn('row.mouse_filter=Control.MOUSE_FILTER_STOP', self.source)
        self.assertIn('Storage: Materials (separate from gear bag)', self.source)
        self.assertIn('_recipe_hover_details(recipe)', self.source)
        self.assertIn('func _recipe_hover_details(recipe:Dictionary)->String:', self.source)
        self.assertIn('materials.get(str(material),0)', self.source)
        self.assertIn("Hunter\'s Oil", self.source)
        self.assertIn('Healing Draught', self.source)

    def test_materials_and_consumables_visible_without_hover(self):
        self.assertIn('stacks) • Crafting storage', self.source)
        self.assertIn('row.text=name+" × "+str(amount)+" • "+str(stack_count)+" stacks • Crafting storage"', self.source)
        self.assertIn('[b]SELECTED RECIPE: ', self.source)
        self.assertIn('_recipe_hover_details(chosen)', self.source)
        self.assertIn('[b]CONSUMABLES[/b]', self.source)
        self.assertIn('owned • Temporary hunting effect (25 seconds)', self.source)
        self.assertIn('owned • Restores 35% maximum health', self.source)

    def test_wow_forever_style_recipe_book(self):
        section = self.source.split('  "Professions":', 1)[1].split('  "Hunter":', 1)[0]
        self.assertIn("PROFESSIONS / RECIPE BOOK", section)
        self.assertIn('"recipe_filter"', section)
        self.assertIn('"recipe_select:"+str(i)', section)
        self.assertIn("crafting_craftable_only and issue", section)
        self.assertIn("_recipe_hover_details(chosen)", section)
        self.assertIn('"craft_selected"', section)
        self.assertIn("INVENTORY_RULES.craft_issue", section)
        self.assertIn("READY TO CRAFT", section)
        self.assertIn('var crafting_selected_index:int=0', self.source)
        self.assertIn('var crafting_craftable_only:bool=false', self.source)
        self.assertIn('if action=="craft_selected":', self.source)
        self.assertIn('_craft(crafting_selected_index)', self.source)
        self.assertIn('button.disabled=true', self.source)

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
