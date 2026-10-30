extends SceneTree
const R:=preload("res://scripts/systems/overhaul_v380.gd")
const BENCH:=preload("res://scripts/systems/combat_benchmark_v383.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS v38: "+label)
 else:
  failures+=1
  push_error("FAIL v38: "+label)
func _run()->void:
 check(R.air_urgency(8.0,true)==2,"critical air severity")
 check(R.air_urgency(18.0,true)==1,"low air severity")
 check(R.air_urgency(100.0,true)==0,"safe underwater oxygen")
 check(R.air_urgency(0.0,false)==0,"no oxygen panic on land")
 check("CRITICAL AIR" in R.threat_status(5.0,true,true,10.0,100.0),"oxygen emergency prioritized")
 check("COMBAT" in R.threat_status(100.0,false,true,90.0,100.0),"combat warning")
 check("LOW HEALTH" in R.threat_status(100.0,false,false,25.0,100.0),"low health warning")
 check("AIR: 20%" in R.safe_label("Moonmist",80.0,100.0,19.5,true,2.4,false),"accessible numeric air readout")
 var fps_samples:Array[int]=[]
 var empty_summary:Dictionary=BENCH.summarize_fps(fps_samples)
 check(int(empty_summary["count"])==0 and int(empty_summary["min"])==0,"empty FPS benchmark has no samples")
 BENCH.record_fps(fps_samples,40)
 BENCH.record_fps(fps_samples,60)
 BENCH.record_fps(fps_samples,80)
 var fps_summary:Dictionary=BENCH.summarize_fps(fps_samples)
 check(int(fps_summary["average"])==60,"rolling FPS average calculated correctly")
 check(int(fps_summary["min"])==40 and int(fps_summary["max"])==80,"rolling FPS extremes calculated correctly")
 fps_samples.clear()
 BENCH.record_fps(fps_samples,-20)
 check(int(BENCH.summarize_fps(fps_samples)["min"])==0,"invalid negative FPS safely clamps")
 fps_samples.clear()
 for fps_value in range(50):BENCH.record_fps(fps_samples,fps_value)
 fps_summary=BENCH.summarize_fps(fps_samples)
 check(int(fps_summary["count"])==BENCH.SAMPLE_LIMIT,"FPS history stays bounded at 40 samples")
 check(int(fps_summary["min"])==10 and int(fps_summary["max"])==49,"FPS history discards oldest samples")
 check(int(fps_summary["average"])==30,"FPS summary averages retained samples only")
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"v38 source scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 var sprite:Sprite3D=game.get("hero_sprite") as Sprite3D
 var pose:Node3D=game.get("v374_swim_pose_anchor") as Node3D
 check(is_instance_valid(pose),"independent swim pose anchor exists")
 check(is_instance_valid(sprite) and sprite.get_parent()==pose,"existing original hero art is mounted under pose anchor")
 check(is_instance_valid(sprite) and sprite.texture!=null,"original art sprite remains textured")
 check(is_instance_valid(pose) and pose.scale.is_equal_approx(Vector3.ONE),"pose transform has no cumulative scaling")
 check(is_instance_valid(game.get("v380_status_panel")),"optional status panel created")
 check(is_instance_valid(game.get("v384_vitals_panel")),"HP mana XP vitals panel created")
 var orb_source:String=FileAccess.get_file_as_string("res://scripts/native_v38_0.gd")
 check("res://assets/ui/moonfall_orb_hud.png" in orb_source,"original orb HUD asset path supported")
 check("HORIZONTAL_ALIGNMENT_CENTER" in orb_source and "font_shadow_color" in orb_source,"orb numerals are centered and legible")
 var rings:Control=game.get("v384_rings") as Control
 check(is_instance_valid(rings) and rings.has_method("set_vitals"),"HP and XP progress rings created")
 if is_instance_valid(rings):
  rings.call("set_vitals",0.35,0.7)
  check(is_equal_approx(float(rings.get("hp_ratio")),0.35) and is_equal_approx(float(rings.get("xp_ratio")),0.7),"circular vitals update from progress values")
 check(not (game.get("v384_vitals_panel") as PanelContainer).visible,"vitals hidden before gameplay")
 game.call("_v384_refresh_vitals")
 var vitals:RichTextLabel=game.get("v384_vitals_label") as RichTextLabel
 check(is_instance_valid(vitals) and "HP" in vitals.text and "XP" in vitals.text,"live health and XP shown in vitals")
 check(is_instance_valid(vitals) and "MANA   100 / 100" in vitals.text,"mana displays actual starting resource")
 game.set("v385_mana",5.0)
 game.call("_spell","nova")
 check(is_equal_approx(float(game.get("v385_mana")),5.0),"insufficient mana blocks spell consumption")
 game.set("v385_mana",42.5)
 var resource_snapshot:Dictionary=game.call("_checkpoint_extra_data")
 check(is_equal_approx(float(resource_snapshot.get("mana",-1)),42.5),"checkpoint captures exact remaining mana")
 game.set("v385_mana",100.0)
 game.set("v387_stamina",42.0)
 var stamina_snapshot:Dictionary=game.call("_checkpoint_extra_data")
 check(is_equal_approx(float(stamina_snapshot.get("stamina",-1.0)),42.0),"stamina included in checkpoint")
 var stamina_ui_source:String=FileAccess.get_file_as_string("res://scripts/native_v38_0.gd")
 check("V387_StaminaBar" in stamina_ui_source and "stamina_fill.bg_color" in stamina_ui_source,"custom orb HUD includes green stamina fill")
 var fallback_stamina:ProgressBar=game.get_node_or_null("NativeAtlasUI/V384_Vitals/HBoxContainer/V387_FallbackStaminaBar") as ProgressBar
 check(is_instance_valid(fallback_stamina),"green stamina bar available without custom artwork")
 game.call("_v384_refresh_vitals")
 if is_instance_valid(fallback_stamina):check(is_equal_approx(fallback_stamina.value,42.0),"fallback stamina bar reflects current resource")
 game.set("v387_stamina",0.0)
 game.call("_dodge")
 check(is_equal_approx(float(game.get("v387_stamina")),0.0),"dodge blocked with exhausted stamina")
 game.set("v387_stamina",150.0)
 game.set("dodge_cooldown",0.0)
 game.call("_dodge")
 check(is_equal_approx(float(game.get("v387_stamina")),122.0),"successful dodge spends exactly 28 stamina")
 check(float(game.get("dodge_cooldown"))>0.0,"successful dodge retains original cooldown")
 game.set("dodge_cooldown",0.0)
 game.set("v387_stamina",150.0)
 check(not (game.get("v380_status_panel") as PanelContainer).visible,"status HUD hidden at title screen")
 var obsolete_title:bool=false
 for label_node in (game.get("start_screen") as Control).find_children("*","Label",true,false):
  if "MOONFALL NATIVE v13" in (label_node as Label).text:obsolete_title=true
 check(not obsolete_title and str(game.call("_release_label")).begins_with("v3"),"no obsolete title and active v3x release")
 game.call("_menu_tab","System")
 var body:RichTextLabel=game.get("menu_body") as RichTextLabel
 check(is_instance_valid(body) and "F2" in body.text,"F2 discoverable inside system menu")
 var perf:Dictionary=game.call("_perf_report")
 check(perf.has("combat_fps") and int(perf["combat_fps"])>=0,"on-demand FPS reported")
 check(perf.has("combat_frame_process_ms") and float(perf["combat_frame_process_ms"])>=0.0,"frame process time reported")
 check(perf.has("combat_frame_physics_ms") and float(perf["combat_frame_physics_ms"])>=0.0,"frame physics time reported")
 check(perf.has("combat_render_draw_calls") and int(perf["combat_render_draw_calls"])>=0,"renderer draw call count available")
 check(perf.has("combat_render_primitives") and int(perf["combat_render_primitives"])>=0,"rendered primitive count available")
 check(perf.has("combat_scene_nodes") and int(perf["combat_scene_nodes"])>=1,"scene tree node count available")
 check(perf.has("combat_physics_collision_pairs") and int(perf["combat_physics_collision_pairs"])>=0,"3D physics collision pair count available")
 check(perf.has("combat_active_hostiles") and int(perf["combat_active_hostiles"])>=0,"active hostile count nonnegative")
 check(perf.has("combat_damage_floaters") and int(perf["combat_damage_floaters"])>=0,"damage floater count nonnegative")
 check(perf.has("combat_sample_count") and int(perf["combat_sample_count"])==0,"FPS samples disabled at title")
 check(perf.has("combat_sample_min_fps") and perf.has("combat_sample_avg_fps"),"FPS summary exported to performance report")
 check(perf.has("combat_peak_hostiles") and int(perf["combat_peak_hostiles"])==0,"hostile peak is zero before gameplay")
 check(perf.has("combat_encounter_sample_count") and int(perf["combat_encounter_sample_count"])==0,"combat-only FPS history starts empty")
 check(perf.has("combat_encounter_avg_fps") and int(perf["combat_encounter_avg_fps"])==0,"empty combat FPS average never invents data")
 check(perf.has("combat_encounter_min_fps") and int(perf["combat_encounter_min_fps"])==0,"empty combat FPS minimum never invents data")
 # Burst-hit smoke test: cap at six labels per target, recycle instead of allocating.
 var dummy_beast:=CharacterBody3D.new()
 game.add_child(dummy_beast)
 var dummy_target:Dictionary={"body":dummy_beast}
 for hit_number in range(6):
  game.call("_show_floating_damage",dummy_target,hit_number+1,false)
 check(dummy_beast.get_child_count()==6,"first six hits create exactly six floaters")
 var original_floater_ids:Array[int]=[]
 for child in dummy_beast.get_children():original_floater_ids.append(child.get_instance_id())
 var tracked_floaters:int=0
 for child in dummy_beast.get_children():
  if child is Label3D and child.has_meta("damage_float_tween"):tracked_floaters+=1
 check(tracked_floaters==6,"all six damage labels use stable metadata despite Godot node renaming")
 var oldest:Label3D=dummy_beast.get_child(0) as Label3D
 game.call("_show_floating_damage",dummy_target,77,true)
 check(dummy_beast.get_child_count()==6,"seventh hit reuses floater; hard cap preserved")
 check(is_instance_valid(oldest) and oldest.text=="77!" and is_equal_approx(oldest.scale.x,1.3),"recycled critical label restores impact styling")
 check(int(dummy_target.get("damage_float_sequence",-1))==7,"damage float lane sequence advances across recycling")
 check(is_equal_approx(oldest.position.x,-0.3),"seventh hit alternates to the left damage lane")
 game.call("_show_floating_damage",dummy_target,78,false)
 var next_floater:Label3D=dummy_beast.get_child(5) as Label3D
 check(is_instance_valid(next_floater) and next_floater.text=="78" and is_equal_approx(next_floater.position.x,0.3),"eighth hit alternates to the right damage lane")
 for hit_number in range(30):
  game.call("_show_floating_damage",dummy_target,hit_number+100,hit_number%2==0)
 check(dummy_beast.get_child_count()==6,"repeated burst hits never exceed the six-label cap")
 var same_floaters:bool=true
 for child in dummy_beast.get_children():
  if not (child.get_instance_id() in original_floater_ids):same_floaters=false
 check(same_floaters,"burst damage reuses the original six labels rather than allocating more")
 var other_beast:=CharacterBody3D.new()
 game.add_child(other_beast)
 game.call("_show_floating_damage",{"body":other_beast},55,false)
 check(other_beast.get_child_count()==1 and dummy_beast.get_child_count()==6,"floater cap stays independent per target")
 # v38.3 runtime integration guard: ensure role policy is invoked inside the live beast tick.
 # This complements pure policy checks but does not replace interactive gameplay testing.
 var enemy_script:GDScript=load("res://scripts/native_v11.gd") as GDScript
 check(enemy_script!=null,"native enemy runtime script loads")
 if enemy_script!=null:
  var source:String=enemy_script.source_code
  var tick_start:int=source.find("func _update_beast_step(")
  var tick_end:int=source.find("\\nfunc ",tick_start+1)
  var tick:String=source.substr(tick_start,(tick_end-tick_start) if tick_end>tick_start else -1) if tick_start>=0 else ""
  check(tick_start>=0 and "V383_COMBAT_RULES.combat_intent(" in tick,"combat intent policy invoked in live enemy tick")
  check("v383_intent==\"retreat\"" in tick and "body.velocity=-diff.normalized()" in tick,"low-health retreat produces actual movement")
  check("v383_intent==\"strafe\"" in tick and "body.velocity=(flank*orbit_side" in tick,"flank intent produces actual movement")
  check("if staggered:" in tick and "if staggered:return" in tick,"stagger still prevents attack resolution")
  check("if bool(beast.get(\"boss\",false)):" in tick,"boss windup attack path preserved")
 game.queue_free()
 print("MOONFALL v38.0 OVERHAUL PASS" if failures==0 else "MOONFALL v38.0 OVERHAUL FAIL")
 quit(0 if failures==0 else 1)
