"""Source regressions for the live (non-pausing) 141-node talent web.

Only talent widget events and explicit close shortcuts are consumed. Opening
the constellation must not modify pending enemy attacks, world clocks, player
physics, incoming damage, or gameplay attack/dodge/spell paths.
A separate Godot headless scene test verifies real runtime behavior.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "godot-v38/scripts/native_v38_0.gd").read_text(encoding="utf-8")
AIMED = (ROOT / "godot-v38/scripts/native_v38_7.gd").read_text(encoding="utf-8")


def method(name: str, source: str = SOURCE) -> str:
    beginning = source.index("func " + name + "(")
    ending = source.find("\nfunc ", beginning + 1)
    return source[beginning:] if ending < 0 else source[beginning:ending]


class TalentCombatLiveRegression(unittest.TestCase):
    def test_visibility_controls_only_shortcuts_not_combat_pause(self):
        self.assertIn("v398_panel.visible", method("_v406_talents_visible"))
        self.assertNotIn("_v406_talents_pause_enemies", SOURCE + AIMED)
        self.assertNotIn("_v406_cancel_pending_enemy_attacks", SOURCE)

    def test_live_physics_enemy_scheduler_and_attack_steps(self):
        # Inherited player physics and enemy scheduler must always run.
        self.assertNotIn("func _physics_process(", SOURCE)
        self.assertNotIn("func _update_beasts(", SOURCE)
        enemy_step = method("_update_beast_step")
        self.assertIn("super._update_beast_step(beast,delta)", enemy_step)
        self.assertNotIn("_v406_talents_visible()", enemy_step)

    def test_opening_and_closing_do_not_reset_pending_attacks(self):
        toggle = method("_v398_toggle_tree")
        self.assertIn("v398_panel.visible=not v398_panel.visible", toggle)
        self.assertIn("_v406_fit_panel_to_viewport()", toggle)
        self.assertIn("_v398_refresh_tree()", toggle)
        self.assertNotIn("for beast in beasts:", toggle)
        self.assertNotIn("windup", toggle)
        self.assertNotIn("v404_tell_next", toggle)
        self.assertNotIn("divine_next_at", toggle)

    def test_incoming_damage_and_player_combat_are_not_blocked(self):
        damage = method("_apply_incoming_damage")
        self.assertIn("super._apply_incoming_damage(raw)", damage)
        for name in ("_apply_incoming_damage", "_attack", "_spell", "_dodge"):
            self.assertNotIn("_v406_talents_visible()", method(name))
        self.assertIn("super._attack()", method("_attack"))
        self.assertIn("super._dodge()", method("_dodge"))
        self.assertIn("super._spell(which)", method("_spell"))
        aimed = method("_spell", AIMED)
        self.assertIn('if which!="fire":', aimed)
        self.assertIn("var target:Dictionary=_aim_candidate", aimed)
        self.assertIn("v385_mana-=cost", aimed)
        self.assertNotIn("_v406_talents_visible()", aimed)

    def test_keyboard_shortcuts_close_and_world_input_continues(self):
        key_handler = method("_unhandled_key_input")
        self.assertIn("event.keycode in [KEY_ESCAPE,KEY_F10] and _v406_talents_visible()", key_handler)
        self.assertIn("event.keycode==KEY_K", key_handler)
        self.assertLess(key_handler.index("event.keycode==KEY_K"),
                        key_handler.index("super._unhandled_key_input(event)"))
        self.assertNotIn("if _v406_talents_visible():\n  get_viewport().set_input_as_handled()", key_handler)
        unhandled = method("_unhandled_input")
        self.assertIn("event.button_index in [JOY_BUTTON_B,JOY_BUTTON_START]", unhandled)
        self.assertIn("if _v406_talents_visible() and event is InputEventJoypadButton", unhandled)
        self.assertLess(unhandled.index("_v398_toggle_tree()"),
                        unhandled.index("super._unhandled_input(event)"))
        self.assertNotIn("if _v406_talents_visible():", unhandled)
        self.assertIn("super._unhandled_input(event)", unhandled)


if __name__ == "__main__":
    unittest.main()
