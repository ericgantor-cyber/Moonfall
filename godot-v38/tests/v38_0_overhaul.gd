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
 check(perf.has("combat_active_hostiles") and int(perf["combat_active_hostiles"])>=0,"active hostile count nonnegative")
 check(perf.has("combat_damage_floaters") and int(perf["combat_damage_floaters"])>=0,"damage floater count nonnegative")
 check(perf.has("combat_sample_count") and int(perf["combat_sample_count"])==0,"FPS samples disabled at title")
 check(perf.has("combat_sample_min_fps") and perf.has("combat_sample_avg_fps"),"FPS summary exported to performance report")
 check(perf.has("combat_peak_hostiles") and int(perf["combat_peak_hostiles"])==0,"hostile peak is zero before gameplay")
 game.queue_free()
 print("MOONFALL v38.0 OVERHAUL PASS" if failures==0 else "MOONFALL v38.0 OVERHAUL FAIL")
 quit(0 if failures==0 else 1)
