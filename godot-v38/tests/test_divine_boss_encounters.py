"""Issue 477 stage 2: source-level regression guards for divine boss world actors."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
LIVE = (ROOT / "scripts/native_v38_8.gd").read_text(encoding="utf-8")
BASE = (ROOT / "scripts/native_v11.gd").read_text(encoding="utf-8")
SCENE = (ROOT / "scenes/main.tscn").read_text(encoding="utf-8")


class DivineBossIntegrationTests(unittest.TestCase):
    def test_live_scene_executes_divine_encounters(self):
        self.assertIn("native_v38_8.gd", SCENE)
        self.assertIn("func _build_scene()->void:\n super._build_scene()\n for spec in DIVINE_ENCOUNTERS:", LIVE)
        self.assertIn('{"id":"z","title":"Z • VOID REAPER"', LIVE)
        self.assertIn('{"id":"nyra","title":"NYRA • ECLIPSE QUEEN"', LIVE)
        self.assertIn('_spawn_beast(location,kind,zone,"elder",true)', LIVE)

    def test_original_images_and_existing_boss_system(self):
        self.assertIn('texture:Texture2D=Z_ART if str(spec["id"])=="z" else NYRA_ART', LIVE)
        self.assertIn('portrait.name="OriginalDivineBossArt"', LIVE)
        self.assertIn('actor["divine_id"]=str(spec["id"])', LIVE)
        self.assertIn('super._defeat_beast(beast,pet_kill)', LIVE)
        self.assertIn('super._reset_native_beast(beast)', LIVE)
        self.assertIn('if bool(beast.get("boss",false)):', BASE)
        self.assertIn('defeated_boss_ids.append(boss_id)', BASE)
        self.assertIn('data.get("bosses",[])', BASE)

    def test_permanent_divine_blessings_use_boss_victory_ids(self):
        self.assertIn('const Z_VOID_DAMAGE_BONUS:float=0.15', LIVE)
        self.assertIn('const NYRA_ECLIPSE_MAX_HP_BONUS:float=30.0', LIVE)
        self.assertIn('func _divine_blessing_earned(divine_id:String)->bool:', LIVE)
        self.assertIn('return defeated_boss_ids.has(int(actor.get("boss_id",-1)))', LIVE)
        self.assertIn('if _divine_blessing_earned("z") else base_damage', LIVE)
        self.assertIn('if _divine_blessing_earned("nyra"):', LIVE)
        self.assertIn('hp=clampf(float(data.get("hp",hp)),1.0,max_hp)', LIVE)
        self.assertIn('if first_clear and divine=="nyra":', LIVE)
        self.assertIn('suppress_autosave_message=true\\n  _save_game()', LIVE)
        self.assertIn('super._defeat_beast(beast,pet_kill)', LIVE)
        self.assertIn('const Z_VOID_ON_KILL_HEAL:float=3.0', LIVE)
        self.assertIn('const NYRA_ECLIPSE_REDUCTION:float=0.08', LIVE)
        self.assertIn('super._apply_incoming_damage(mitigated)', LIVE)

    def test_signature_attacks_are_visible_dodgeable_and_bounded(self):
        self.assertIn('warning.name="DivineSignatureAttackWarning"', LIVE)
        self.assertIn('actor["divine_disk"]=warning', LIVE)
        self.assertIn('func _update_beast_step(beast:Dictionary,delta:float)->void:', LIVE)
        self.assertIn('super._update_beast_step(beast,delta)', LIVE)
        self.assertIn('dodge_time<=0.0 and _has_clear_sight(body,player)', LIVE)
        self.assertIn('beast["divine_attack_radius"]=radius', LIVE)
        self.assertIn('if phase==3 and cycle%2==1:aimed=not aimed', LIVE)
        self.assertIn('var strike_pos:Vector3=player.global_position if aimed else body.global_position', LIVE)
        self.assertIn('beast["divine_pattern_seq"]=cycle+1', LIVE)
        self.assertIn('func _refresh_boss_phase(beast:Dictionary)->void:', LIVE)
        self.assertIn('func _update_hud()->void:', LIVE)

    def test_existing_save_ids_are_stable_and_no_new_waves(self):
        self.assertIn('var prior:int=beasts.size()', LIVE)
        self.assertIn('if beasts.size()!=prior+1:return', LIVE)
        self.assertNotIn("_start_rift", LIVE.split("# Issue #477 stage 2:", 1)[1])
        self.assertNotIn("_start_game(", LIVE.split("# Issue #477 stage 2:", 1)[1])


if __name__ == "__main__":
    unittest.main()
