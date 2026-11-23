"""Source-level safeguards: ordinary Moonfall panels never pause world combat."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "scripts"

def function(file_name: str, name: str) -> str:
    source = (ROOT / file_name).read_text(encoding="utf-8")
    start = source.index("func " + name + "(")
    end = source.find("\nfunc ", start + 1)
    return source[start:end if end >= 0 else None]

class AllPanelsRemainLive(unittest.TestCase):
    def test_raven_atlas_still_has_all_tabs(self):
        source = function("native_v11.gd", "_build_ui")
        for tab in ("Character", "World", "Wallet", "Inventory", "Materials",
                    "Hero Skills", "Tamer", "Professions", "Hunter", "Bestiary", "System"):
            self.assertIn('"' + tab + '"', source)

    def test_open_menu_does_not_cancel_scheduling(self):
        toggle = function("native_v11.gd", "_toggle_menu")
        self.assertIn("menu_open=not menu_open", toggle)
        self.assertNotIn("windup", toggle)
        self.assertNotIn("attack_pending", toggle)

    def test_all_menu_tabs_keep_movement_and_enemy_ticks(self):
        physics = function("native_v11.gd", "_physics_process")
        self.assertNotIn("or menu_open", physics)
        for token in ("map_time+=delta", "player.move_and_slide()", "_update_beasts(delta)"):
            self.assertIn(token, physics)
        self.assertIn("or wait_open or wait_transition_active:return", physics)

    def test_boss_telegraphs_and_rift_hazards_continue(self):
        for filename, name in (("native_v34_0.gd", "_depth_surge_tick"),
                               ("native_v34_2.gd", "_v342_tick")):
            self.assertNotIn("or menu_open", function(filename, name))

    def test_companions_and_buffs_do_not_freeze(self):
        for filename, name in (("native_v38_4.gd", "_process"),
                               ("native_v38_5.gd", "_process"),
                               ("native_v36_3.gd", "_process")):
            body = function(filename, name)
            self.assertNotIn("or menu_open", body)
            self.assertNotIn("and not menu_open", body)

    def test_live_boss_warnings_survive_open_menus(self):
        divine = function("native_v38_8.gd", "_update_beast_step")
        self.assertIn("beast.get(\"divine_warn_until\"", divine)
        self.assertIn("_clear_divine_warning(beast)", divine)
        self.assertNotIn("or menu_open", divine)
        self.assertIn("or death_active or wait_open or wait_transition_active", divine)
        commander = function("native_v34_4.gd", "_process")
        self.assertIn("v342_phase_clock+=clampf(delta,0.0,0.15)*extra", commander)
        self.assertNotIn("or menu_open", commander)

    def test_mana_and_stamina_regen_in_all_panels(self):
        process = function("native_v38_0.gd", "_process")
        self.assertIn("if hero_playing and not death_active:", process)
        self.assertIn("v385_mana=minf(", process)
        self.assertIn("v387_stamina=minf(", process)
        start = process.index("if hero_playing and not death_active:")
        end = process.index("if is_instance_valid(v384_vitals_panel):", start)
        self.assertNotIn("menu_open", process[start:end])
        self.assertIn("v387_stamina_regen_wait-=delay_used", process[start:end])

    def test_underwater_danger_remains_real(self):
        oxygen = function("native_v37_2.gd", "_physics_process")
        self.assertIn("can_dive(v37_swimming,hero_playing,false,mounted,death_active)", oxygen)
        self.assertIn("V372_OXYGEN.advance(v372_oxygen,delta,v372_diving)", oxygen)
        self.assertNotIn("elif not menu_open:", oxygen)
        self.assertIn("if hero_playing and not death_active:", function("native_v37_3.gd", "_physics_process"))

if __name__ == "__main__":
    unittest.main()
