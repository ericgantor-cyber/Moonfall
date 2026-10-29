#!/usr/bin/env python3
"""Run Moonfall engine tests with exit/marker checks. No third-party dependencies."""
import os,sys,subprocess,shutil,pathlib,tempfile
ROOT=pathlib.Path(__file__).resolve().parent.parent
DEFAULT_WINDOWS=ROOT/'Godot_4.7.2'/'Godot_v4.7.2-stable_win64.exe'
ENGINE=os.environ.get('GODOT_BIN') or (str(DEFAULT_WINDOWS) if DEFAULT_WINDOWS.exists() else shutil.which('godot') or shutil.which('godot4'))
SUITES={
 'smoke_v11':'MOONFALL NATIVE v11 SMOKE PASS',
 'v13_features':'MOONFALL NATIVE v13 FEATURES PASS',
 'v14_visuals':'MOONFALL NATIVE v14 VISUALS PASS',
 'v15_living_art':'MOONFALL NATIVE v15 LIVING ART PASS',
 'v16_visuals':'MOONFALL NATIVE v16 VISUALS PASS',
 'v17_release':'MOONFALL NATIVE v17 RELEASE PASS',
 'v18_grid':'MOONFALL NATIVE v18 GRID PASS',
 'v18_stability':'MOONFALL v18 STABILITY PASS',
 'v19_world_art':'MOONFALL v19 WORLD ART PASS',
 'v20_guardians':'MOONFALL v20 GUARDIANS PASS',
 'v21_grand_art':'MOONFALL v21 GRAND ART PASS',
 'v22_art':'MOONFALL v22 ART PASS',
 'v23_animation':'MOONFALL v23 ANIMATION PASS',
 'v24_overhaul':'MOONFALL v24 OVERHAUL PASS',
 'v24_1_accessibility':'MOONFALL v24.1 ACCESSIBILITY PASS',
 'live_v11':'MOONFALL NATIVE v11 LIVE PASS',
 'v25_legacy_modernization':'MOONFALL v25 LEGACY MODERNIZATION PASS',
 'v26_modular_core':'MOONFALL v26 MODULAR CORE PASS',
 'v27_system_modules':'MOONFALL v27 SYSTEM MODULES PASS',
 'v28_ai_scheduler':'MOONFALL v28 AI SCHEDULER PASS',
 'v29_performance':'MOONFALL v29 PERFORMANCE PASS',
 'v30_release':'MOONFALL v30 RELEASE PASS',
 'v30_1_profiler':'MOONFALL v30.1 PROFILER PASS',
 'v30_3_world_cycle':'MOONFALL v30.3 WORLD CYCLE PASS',
 'v30_4_inventory_rules':'MOONFALL v30.4 INVENTORY RULES PASS',
 'v30_5_inventory_expansion':'MOONFALL v30.5 INVENTORY EXPANSION PASS',
 'v30_6_money_economy':'MOONFALL v30.6 CURRENCY & MERCHANT PASS',
 'v30_7_forge_treasure':'MOONFALL v30.7 FORGE & TREASURE PASS',
 'v30_8_shop_ui':'MOONFALL v30.8 SHOP UI PASS',
 'v30_9_graphics':'MOONFALL v30.9 GRAPHICS PASS',
 'v31_0_graphics':'MOONFALL v31.0 GRAPHICS PASS',
 'v31_1_realism':'MOONFALL v31.1 REALISM PASS',
 'v31_2_visual_performance':'MOONFALL v31.2 VISUAL PERFORMANCE PASS',
 'v31_2_presets':'MOONFALL v31.2 PRESET SWEEP PASS',
 'v31_3_gameplay':'MOONFALL v31.3 GAMEPLAY PASS',
 'v31_3_art':'MOONFALL v31.3 ART PASS',
 'v34_4_4k_display':'MOONFALL 4K DISPLAY PASS',
 'v31_4_bestiary':'MOONFALL v31.4 BESTIARY PASS',
 'v31_5_living_meshes':'MOONFALL v31.5 LIVING MESH PASS',
 'v31_6_anatomy_ai':'MOONFALL v31.6 ANATOMY AND AI PASS',
 'v31_6_second_pass':'MOONFALL v31.6 SECOND PASS PASS',
 'v32_0_world_reborn':'MOONFALL v32 WORLD REBORN PASS',
 'v32_0_stream_budget':'MOONFALL v32 STREAM BUDGET PASS',
 'v32_1_world_reborn_stage2':'MOONFALL v32.1 STAGE 2 PASS',
 'v32_2_lean_chambers':'v32.2 PASS',
 'v33_8_death_resurrection':'MOONFALL v33.8 DEATH PASS',
 'v33_8_moon_sigil_forge':'MOON SIGIL FORGE PASS',
 'v33_8_world_depth':'MOONFALL WORLD DEPTH PASS',
 'v33_8_living_rifts':'MOONFALL LIVING RIFTS PASS',
 'v34_0_grand_overhaul':'MOONFALL v34.0 GRAND OVERHAUL PASS',
 'v34_1_dynamic_invasions':'MOONFALL v34.1 DYNAMIC INVASIONS PASS',
 'v34_2_elemental_invasions':'MOONFALL v34.2 ELEMENTAL INVASIONS PASS',
 'v34_3_lunar_war':'MOONFALL v34.3 LUNAR WAR PASS',
 'v34_4_fair_fight':'MOONFALL v34.4 FAIR FIGHT PASS',
 'v34_4_invasion_guard':'MOONFALL INVASION GUARD PASS',
 'v36_0_third_person_3d':'MOONFALL 3D CAMERA PASS',
 'v36_1_world_art':'MOONFALL v36.1 WORLD ART PASS',
 'v36_2_water_professions':'MOONFALL v36.2 WATER PROFESSIONS PASS',
 'v36_3_living_world':'MOONFALL v36.3 LIVING WORLD PASS',
 'v37_0_living_realm':'MOONFALL v37 LIVING REALM PASS',
 'v37_2_oxygen':'MOONFALL v37.2 OXYGEN PASS',
 'v37_3_deep_water':'MOONFALL v37.3 DEEP WATER PASS',
 'v37_4_solid_grottos':'MOONFALL v37.4 SOLID GROTTOS PASS',
 'v37_6_all_tips':'MOONFALL v37.6 ALL TIPS PASS',
 'v37_7_playability':'MOONFALL v37.7 PLAYABILITY PASS',
 'v38_0_overhaul':'MOONFALL v38.0 OVERHAUL PASS',
}
def main():
 selected=sys.argv[1:]
 if selected==['--list']:
  print('\n'.join(SUITES));return 0
 if selected==['--all']: selected=[]
 unknown=[name for name in selected if name not in SUITES]
 if unknown:
  print('Unknown suites: '+', '.join(unknown),file=sys.stderr);return 2
 if not ENGINE:
  print('ERROR: Set GODOT_BIN to Godot 4.7.2, or place Windows Godot under Godot_4.7.2',file=sys.stderr);return 2
 def run(args, expect=None):
  env=os.environ.copy()
  env['MOONFALL_TEST_PROFILE']=str(TEST_PROFILE)
  env['XDG_DATA_HOME']=str(TEST_PROFILE)
  env['APPDATA']=str(TEST_PROFILE/'Roaming')
  env['LOCALAPPDATA']=str(TEST_PROFILE/'Local')
  try:
   p=subprocess.run([ENGINE,'--headless','--path',str(ROOT),*args],cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace',timeout=90)
  except subprocess.TimeoutExpired as exc:
   print('FAIL timeout (90s):', ' '.join(args), file=sys.stderr)
   return False
  log=p.stdout
  markers=('SCRIPT ERROR','Parse Error','ERROR:','FAIL v','FAILED:')
  good=p.returncode==0 and all(m not in log for m in markers) and (expect is None or expect in log)
  if not good:print(log[-8000:])
  return good
 with tempfile.TemporaryDirectory(prefix='moonfall-tests-') as temporary_profile:
  global TEST_PROFILE
  TEST_PROFILE=pathlib.Path(temporary_profile)
  (TEST_PROFILE/'Roaming').mkdir()
  (TEST_PROFILE/'Local').mkdir()
  print('Using Godot:',ENGINE)
  if not run(['--editor','--import','--quit']):print('FAIL import');return 1
  failures=[]
  for name,expected in SUITES.items():
   if selected and name not in selected:continue
   ok=run(['--script',f'res://tests/{name}.gd'],expected)
   print(('PASS' if ok else 'FAIL'),name,flush=True)
   if not ok:failures.append(name)
  total=len(selected) if selected else len(SUITES)
  print(f'{total-len(failures)}/{total} suites passed')
  return 1 if failures else 0
if __name__=='__main__':sys.exit(main())
