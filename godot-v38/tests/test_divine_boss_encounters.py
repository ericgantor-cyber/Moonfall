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
        self.assertIn('divine_boss_ids[str(spec["id"])]=int(actor.get("boss_id",-1))', LIVE)
        self.assertIn('return defeated_boss_ids.has(int(divine_boss_ids[divine_id]))', LIVE)
        self.assertNotIn('for actor in beasts:\\n  if str(actor.get("divine_id",""))==divine_id:', LIVE)
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
        self.assertIn('if dist_sq<radius*radius and _has_clear_sight(body,player):', LIVE)
        self.assertIn('if dodge_time>0.0:', LIVE)
        self.assertIn('beast["divine_attack_radius"]=radius', LIVE)
        self.assertIn('if phase>=2 and cycle%2==1:aimed=not aimed', LIVE)
        self.assertIn('var strike_pos:Vector3=player.global_position if aimed else body.global_position', LIVE)
        self.assertIn('beast["divine_pattern_seq"]=cycle+1', LIVE)
        self.assertIn('beast["divine_last_aimed"]=aimed', LIVE)
        self.assertIn('var follow_pos:Vector3=body.global_position if bool(beast.get("divine_last_aimed",false)) else player.global_position', LIVE)
        self.assertIn('var urgency:float=clampf(1.0-(remaining-map_time)/1.7,0.0,1.0)', LIVE)
        self.assertIn('6.0+urgency*8.0', LIVE)
        self.assertIn('func _refresh_boss_phase(beast:Dictionary)->void:', LIVE)
        self.assertIn('if phase>previous:\\n  # A telegraphed strike', LIVE)
        self.assertIn('_clear_divine_warning(beast)\\n  if is_instance_valid(label):', LIVE)
        self.assertIn('func _update_hud()->void:', LIVE)
        self.assertIn('func _update_region_visibility()->void:', LIVE)
        self.assertIn('func _clear_divine_warning(actor:Dictionary)->void:', LIVE)
        self.assertIn('func _clear_all_divine_warnings()->void:', LIVE)
        self.assertIn('if divine_last_visible_zone==region_index:return', LIVE)
        self.assertIn('for id in divine_boss_ids.values():', LIVE)
        self.assertIn('if is_instance_valid(disk):disk.hide()', LIVE)
        self.assertIn('if not divine.is_empty():_clear_divine_warning(beast)', LIVE)
        self.assertIn('_clear_all_divine_warnings()\\n _update_hud()', LIVE)

    def test_phase_three_chained_strikes_are_fair_and_bounded(self):
        self.assertIn('actor["divine_chain_pending"]=false', LIVE)
        self.assertIn('beast["divine_chain_pending"]=false', LIVE)
        self.assertIn('var chained:bool=bool(beast.get("divine_chain_pending",false))', LIVE)
        self.assertIn('if not chained and int(beast.get("boss_phase",1))==3', LIVE)
        self.assertIn('beast["divine_warn_until"]=map_time+1.15', LIVE)
        self.assertIn('beast["divine_chain_pending"]=true', LIVE)
        self.assertIn('beast["divine_warning_pos"]=follow_pos', LIVE)
        self.assertIn('warning.visible=true\n   _show_message("SECOND STRIKE!', LIVE)
        self.assertIn('actor["divine_chain_pending"]=false\n var disk:', LIVE)
        self.assertEqual(LIVE.count('warning:=MeshInstance3D.new()'), 1)
        self.assertNotIn('for actor in beasts:', LIVE.split('# Signature divine attacks', 1)[1])

    def test_boss_encounter_safety_and_dodge_counterplay(self):
        # Five gameplay improvements: rest/travel safety, regional cancellation,
        # distance cancellation, skillful dodge reward, boss-specific rebound cues.
        self.assertIn('or wait_open or wait_transition_active:', LIVE)
        self.assertIn('if int(beast.get("zone",-1))!=region_index:', LIVE)
        self.assertIn('if body.global_position.distance_squared_to(player.global_position)>25.0*25.0:', LIVE)
        self.assertIn('if dodge_time>0.0:', LIVE)
        self.assertIn('v387_stamina=minf(V387_MAX_STAMINA,v387_stamina+5.0)', LIVE)
        self.assertIn('PERFECT EVADE! +5 stamina', LIVE)
        self.assertIn('VOID REBOUND!', LIVE)
        self.assertIn('ECLIPSE REBOUND!', LIVE)
        self.assertIn('body.global_position.y+0.07', LIVE)
        self.assertIn('_clear_divine_warning(beast)\\n  return', LIVE)
        # No save migration or extra meshes required for either encounter.
        self.assertEqual(LIVE.count('warning:=MeshInstance3D.new()'), 1)

    def test_wow_style_keyboard_and_controller_layers_preserve_base_controls(self):
        self.assertIn('func _wow_combo_action(button:int,left:bool,right:bool)->String:', LIVE)
        self.assertIn('if left==right:return ""', LIVE)
        for action in ('dash', 'nova', 'fire', 'jump', 'senses', 'companion', 'mount', 'interact'):
            self.assertIn('return "' + action + '"', LIVE)
        self.assertIn('JOY_AXIS_TRIGGER_LEFT', LIVE)
        self.assertIn('JOY_AXIS_TRIGGER_RIGHT', LIVE)
        self.assertIn('func _unhandled_key_input(event:InputEvent)->void:', LIVE)
        self.assertIn('super._unhandled_key_input(event)', LIVE)
        self.assertIn('super._unhandled_input(event)', LIVE)
        self.assertIn('event.alt_pressed and not event.ctrl_pressed', LIVE)
        self.assertIn('not wait_open and not wait_transition_active', LIVE)
        self.assertNotIn('KEY_SPACE:return "attack"', LIVE)

    def test_fov_settings_exclusive_and_accessible(self):
        camera = (ROOT / "scripts/native_v34_4.gd").read_text(encoding="utf-8")
        self.assertIn('func _v36_set_fov(value:float)->void:', LIVE)
        self.assertIn('v36_fov=clampf(value,58.0,76.0)', LIVE)
        for control in ('camera_fov_fine_down', 'camera_fov_fine_up',
                        'camera_fov_down', 'camera_fov_up',
                        'camera_fov_focused', 'camera_fov_reset', 'camera_fov_wide'):
            self.assertIn(control, LIVE)
        self.assertIn('if is_instance_valid(camera):camera.fov=v36_fov', LIVE)
        self.assertIn('_v36_save_camera_preferences()', LIVE)
        self.assertIn('if action=="camera_settings_open":', LIVE)
        self.assertNotIn('KEY_F11', camera)
        self.assertNotIn('v36_fov=clampf(v36_fov+fov_step', camera)

    def test_existing_save_ids_are_stable_and_no_new_waves(self):
        self.assertIn('var prior:int=beasts.size()', LIVE)
        self.assertIn('if beasts.size()!=prior+1:return', LIVE)
        self.assertNotIn("_start_rift", LIVE.split("# Issue #477 stage 2:", 1)[1])
        self.assertNotIn("_start_game(", LIVE.split("# Issue #477 stage 2:", 1)[1])


if __name__ == "__main__":
    unittest.main()
