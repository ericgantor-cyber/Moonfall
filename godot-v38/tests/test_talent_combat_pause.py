"""Regression checks for the 141-node talent-web combat pause.

This deliberately inspects the live Godot runtime, not a mock UI. It ensures
future UI changes cannot silently re-enable attacks while talents are open.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "godot-v38/scripts/native_v38_0.gd").read_text(encoding="utf-8")


def method(name: str) -> str:
    beginning = SOURCE.index("func " + name + "(")
    ending = SOURCE.find("\nfunc ", beginning + 1)
    return SOURCE[beginning:] if ending < 0 else SOURCE[beginning:ending]


class TalentCombatPauseRegression(unittest.TestCase):
    def test_talent_visibility_is_pause_condition(self):
        self.assertIn("v398_panel.visible", method("_v406_talents_pause_enemies"))

    def test_enemy_scheduler_and_individual_step_both_pause(self):
        for name in ("_update_beasts", "_update_beast_step"):
            body = method(name)
            self.assertIn("if _v406_talents_pause_enemies():return", body)
            self.assertLess(body.index("if _v406_talents_pause_enemies():return"), body.index("super."))

    def test_opening_talents_cancels_pending_attacks(self):
        toggle = method("_v398_toggle_tree")
        self.assertIn("_v406_cancel_pending_enemy_attacks()", toggle)
        cancels = method("_v406_cancel_pending_enemy_attacks")
        for token in ('beast["windup"]=0.0', 'beast["elite_lunge_windup"]=0.0',
                      'beast["v404_attack_pending"]=false',
                      'beast["v407_release_until"]=0.0',
                      'beast["v28_sleep_dt"]=0.0',
                      'warning.visible=false'):
            self.assertIn(token, cancels)

    def test_overdue_boss_warning_does_not_fire_on_menu_close(self):
        toggle = method("_v398_toggle_tree")
        self.assertIn('beast["v404_tell_next"]=maxf(', toggle)
        self.assertIn('map_time+2.0', toggle)
        self.assertLess(toggle.index('else:\n  # The world clock'),
                        toggle.index('beast["v404_tell_next"]=maxf('))

    def test_escape_closes_talents_before_base_menu(self):
        unhandled = method("_unhandled_input")
        self.assertIn("event.keycode==KEY_ESCAPE and _v406_talents_pause_enemies()", unhandled)
        self.assertIn("_v398_toggle_tree()", unhandled)
        self.assertLess(unhandled.index("event.keycode==KEY_ESCAPE and _v406_talents_pause_enemies()"),
                        unhandled.index("super._unhandled_input(event)"))

    def test_player_attack_stays_disabled(self):
        attack = method("_attack")
        self.assertIn("if _v406_talents_pause_enemies():return", attack)
        self.assertLess(attack.index("if _v406_talents_pause_enemies():return"),
                        attack.index("super._attack()"))


if __name__ == "__main__":
    unittest.main()
