#!/usr/bin/env python3
"""Source contract for native classless talent UI and additive checkpoint data."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCREEN = ROOT / "godot-v38/scripts/native_v38_0.gd"
MODEL = ROOT / "godot-v38/scripts/systems/moonfall_talent_tree.gd"


class ClasslessTalentUIScreen(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.screen = SCREEN.read_text(encoding="utf-8")
        cls.model = MODEL.read_text(encoding="utf-8")

    def test_keyboard_panel_uses_interactive_classless_web(self):
        self.assertIn("event.keycode==KEY_K", self.screen)
        self.assertIn("CLASSLESS TALENTS", self.screen)
        self.assertIn("CLASSLESS_TALENTS.evaluate(", self.screen)
        self.assertIn("CLASSLESS_TALENTS.spend(", self.screen)
        self.assertIn("v406_purchase_button.disabled=not bool(preview.get(", self.screen)
        self.assertIn('V406_WEB.new()', self.screen)
        self.assertIn('v406_web.talent_selected.connect(_v406_web_selected)', self.screen)

    def test_web_has_pan_zoom_keyboard_and_large_controls(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        for token in ("_draw()", "draw_line(", "draw_circle(",
                      "MOUSE_BUTTON_WHEEL_UP", "MOUSE_BUTTON_WHEEL_DOWN",
                      "drag_distance", "KEY_LEFT", "KEY_HOME", "KEY_EQUAL",
                      "talent_selected.emit(", "func focus_branch(", "func fit_all("):
            self.assertIn(token, web)
        self.assertIn('v406_web.custom_minimum_size=Vector2(760,565)', self.screen)
        self.assertIn('focus.custom_minimum_size=Vector2(148,52)', self.screen)
        self.assertIn('v406_purchase_button.custom_minimum_size.y=56', self.screen)
        self.assertIn('index_button.custom_minimum_size.y=42.0', self.screen)
        self.assertIn('v406_respec_button.disabled=v406_ranks.is_empty()', self.screen)

    def test_constellation_cursor_zoom_and_hover_inspection(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        self.assertIn("func zoom_at(factor: float, screen_point: Vector2)", web)
        self.assertIn('zoom_at(1.18, mouse.position)', web)
        self.assertIn('zoom_at(1.0 / 1.18, mouse.position)', web)
        self.assertIn('mouse_down_on_node', web)
        self.assertIn('hovered_id', web)
        self.assertIn('tooltip_text =', web)
        self.assertIn('elif id == hovered_id:', web)
        self.assertIn("func focus_node(talent_id: String)", web)
        self.assertIn("v406_web.focus_node(id)", self.screen)
        self.assertIn('v406_active_branch=str(node.get("branch",v406_active_branch))', self.screen)
        self.assertIn('v406_selected_node=id', self.screen)
        self.assertIn("_v406_index_selected.bind(node_id)", self.screen)

    def test_unlockable_navigation_and_prerequisite_highlights(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        for token in ('func next_available(', 'func available_talent_ids(',
                      'func _update_prerequisite_path()', 'prerequisite_path.has(id)',
                      'KEY_N', 'func select_node('):
            self.assertIn(token, web)
        self.assertIn('next_button.text="NEXT AVAILABLE (N)"', self.screen)
        self.assertIn('v406_web.next_available(v406_selected_node)', self.screen)
        self.assertIn('v406_web.select_node(v406_selected_node)', self.screen)
        self.assertNotIn('key.keycode == KEY_N or key.keycode == KEY_TAB', web)

    def test_constellation_layout_is_cached_between_allocations(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        self.assertIn('var layout_changed: bool = points.is_empty()', web)
        self.assertIn('if layout_changed:', web)
        self.assertIn('layout_rebuild_count += 1', web)
        self.assertIn('if dragging and mouse_down_on_node == "" or hover_changed:', web)

    def test_large_capstone_hitboxes_match_drawn_stars(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        self.assertIn("func _visual_radius(talent_id: String)", web)
        self.assertIn("_visual_radius(str(id)) + 10.0", web)
        self.assertIn("var radius: float = _visual_radius(id)", web)
        runtime = (ROOT / "godot-v38/tests/talent_web_runtime.gd").read_text(encoding="utf-8")
        self.assertIn("capstone_radius - 1.0", runtime)
        self.assertIn("Capstone edge must be clickable", runtime)

    def test_responsive_constellation_panel(self):
        layout = (ROOT / "godot-v38/scripts/systems/moonfall_talent_panel_layout.gd").read_text(encoding="utf-8")
        self.assertIn("static func placement(viewport_size: Vector2)", layout)
        self.assertIn("VIEWPORT_MARGIN", layout)
        self.assertIn("get_viewport().size_changed.connect(_v406_fit_panel_to_viewport)", self.screen)
        self.assertIn("V406_PANEL_LAYOUT.placement(", self.screen)
        self.assertIn("v398_panel.scale=Vector2(factor,factor)", self.screen)
        self.assertIn('v398_panel.position=layout["position"]', self.screen)
        self.assertIn('Control.PRESET_TOP_LEFT', self.screen)
        self.assertNotIn('v398_panel.position=Vector2(-550,-365)', self.screen)
        self.assertIn("_v406_fit_panel_to_viewport()\n  _v398_refresh_tree()", self.screen)

    def test_save_restores_classless_without_clobbering_legacy(self):
        self.assertIn('"classless_ranks":v406_ranks.duplicate(true)', self.screen)
        self.assertIn('resources.get("classless_ranks",{})', self.screen)
        self.assertIn('resources.get("celestial_ranks",{})', self.screen)
        self.assertIn('v406_ranks=restored["allocations"].duplicate(true)', self.screen)
        self.assertIn('if bool(restored.get("ok",false)):', self.screen)
        self.assertIn('v406_ranks={}', self.screen)
        self.assertIn('_save_game()', self.screen)

    def test_full_tree_respec_not_individual_refund(self):
        self.assertIn("CLASSLESS_TALENTS.respec(", self.screen)
        self.assertIn("RESET ALL TALENT POINTS", self.screen)
        self.assertNotIn("CLASSLESS_TALENTS.refund(", self.screen)

    def test_respec_requires_explicit_confirmation(self):
        self.assertIn('var reset_confirm:ConfirmationDialog=ConfirmationDialog.new()', self.screen)
        self.assertIn('reset_confirm.confirmed.connect(_v406_respec)', self.screen)
        self.assertIn('v406_respec_button.pressed.connect(func():reset_confirm.popup_centered())', self.screen)
        self.assertNotIn('v406_respec_button.pressed.connect(_v406_respec)', self.screen)

    def test_rank_feedback_and_point_cost_gate(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        self.assertIn('available >= int(node.get("cost_per_rank", 1))', web)
        self.assertIn('var rank_arc: float = TAU * float(rank) / float(max_rank)', web)
        self.assertIn('func tier_summary()', web)
        self.assertIn('func focus_tier(target_tier: int)', web)

    def test_classless_bonuses_reach_live_damage_calculations(self):
        self.assertIn('func _hero_damage_multiplier()->float:', self.screen)
        self.assertIn('super._hero_damage_multiplier()', self.screen)
        self.assertIn('v406_damage_context="melee"', self.screen)
        self.assertIn('v406_damage_context="magic"', self.screen)
        self.assertIn('_v406_bonus("melee_damage","percent")', self.screen)
        self.assertIn('_v406_bonus("magic_damage","percent")', self.screen)
        self.assertIn('v406_bonuses_cache=state.get("bonuses",{})', self.screen)
        self.assertIn('v406_damage_context=""', self.screen)

    def test_bonus_resources_restore_before_cap_and_update_hud(self):
        restore = self.screen.split('func _restore_data(data:Dictionary)->void:', 1)[1].split('func _v384_refresh_vitals()', 1)[0]
        self.assertLess(restore.index('v406_ranks=restored["allocations"].duplicate(true)'),
                        restore.index('v385_mana=clampf(float(resources.get("mana"'))
        self.assertIn('_v406_resource_max("mana",V385_MAX_MANA)', restore)
        self.assertIn('_v406_resource_max("stamina",V387_MAX_STAMINA)', restore)
        self.assertIn('v387_stamina_bar.max_value=_v406_resource_max("stamina",V387_MAX_STAMINA)', self.screen)
        self.assertIn('v387_fallback_stamina_bar.max_value=_v406_resource_max("stamina",V387_MAX_STAMINA)', self.screen)
        self.assertIn('v385_mana=minf(_v406_resource_max("mana",V385_MAX_MANA)', self.screen)
        self.assertIn('v387_stamina=minf(_v406_resource_max("stamina",V387_MAX_STAMINA)', self.screen)

    def test_health_talent_caps_and_no_free_healing(self):
        self.assertIn('func _v406_sync_health()->void:', self.screen)
        self.assertIn('var base_cap:float=maxf(1.0,old_cap-v406_applied_health_bonus)', self.screen)
        self.assertIn('var target_cap:float=_v406_resource_max("health",base_cap)', self.screen)
        self.assertIn('hp=clampf(hp,0.0,max_hp)', self.screen)
        self.assertIn('hp=clampf(float(data.get("hp",hp)),1.0,max_hp)', self.screen)
        self.assertIn('v406_applied_health_bonus=0.0', self.screen)
        self.assertGreaterEqual(self.screen.count('_v406_sync_health()'), 4)

    def test_health_talents_recalculate_after_respec_and_new_game(self):
        respec = self.screen.split('func _v406_respec()->void:', 1)[1].split('func _v398_toggle_tree()', 1)[0]
        self.assertLess(respec.index('v406_ranks=changed["allocations"].duplicate(true)'),
                        respec.index('_v406_sync_health()'))
        self.assertLess(respec.index('_v406_sync_health()'), respec.index('_save_game()'))
        start = self.screen.split('func _start_game(selected_hero:String)->void:', 1)[1].split('func _checkpoint_extra_data()', 1)[0]
        self.assertIn('v406_applied_health_bonus=0.0', start)

    def test_original_art_assets_still_preloaded(self):
        self.assertIn('Z_Original.webp', (ROOT / "godot-v38/scripts/main.gd").read_text(encoding="utf-8"))
        self.assertIn('Nyra_Original.png', (ROOT / "godot-v38/scripts/main.gd").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
