extends SceneTree
var failed:int=0
func check(ok:bool,description:String)->void:
 if ok:print("PASS v30.1: "+description)
 else:
  failed+=1
  push_error("FAIL v30.1: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"main scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"new profiler script active")
 game.call("_start_game","z")
 # Combat feedback cleanup uses the live game scene and must retain active
 # effects while reclaiming expired and already-freed ones without a copy.
 var active_cue:=Node3D.new()
 var expired_cue:=Node3D.new()
 var freed_cue:=Node3D.new()
 game.add_child(active_cue)
 game.add_child(expired_cue)
 freed_cue.free()
 var effects:Array=game.get("combat_cues")
 var initial_fx_count:int=effects.size()
 effects.append({"node":active_cue,"age":0.0,"life":1.0,"kind":"attack"})
 effects.append({"node":expired_cue,"age":0.99,"life":1.0,"kind":"spell"})
 effects.append({"node":freed_cue,"age":0.0,"life":1.0,"kind":"dodge"})
 game.call("_v16_tick_combat_cues",0.05)
 check(effects.size()==initial_fx_count+1,"combat FX removes expired and invalid cues in place")
 check(is_instance_valid(active_cue) and absf(float(effects[effects.size()-1]["age"])-0.05)<0.001,"combat FX preserves and advances active cue")
 game.call("_v16_tick_combat_cues",1.0)
 check(effects.size()==initial_fx_count,"combat FX drains completed cues")
 # A diagnostic request before the first captured frame must never index
 # a negative sample position or produce a fake low-FPS value.
 game.call("_perf_start",true)
 var empty_report:Dictionary=game.call("_perf_report")
 check(int(empty_report.get("frames",-1))==0,"empty performance capture has zero frames")
 check(float(empty_report.get("one_percent_low_fps",-1.0))==0.0,"empty performance capture safely reports zero one-percent-low FPS")
 game.call("_perf_stop",false)
 # A partial report must survive before the 60-second benchmark completes.
 game.call("_perf_start",true)
 for i in range(901):game.call("_perf_sample",1.0/60.0)
 var snapshot_path:String="user://moonfall_v30_1_performance.json"
 check(FileAccess.file_exists(snapshot_path),"15-second performance snapshot is written automatically")
 if FileAccess.file_exists(snapshot_path):
  var snapshot:Variant=JSON.parse_string(FileAccess.get_file_as_string(snapshot_path))
  check(snapshot is Dictionary and int(snapshot.get("frames",0))>=899 and int(snapshot.get("frames",0))<=901 and float(snapshot.get("seconds",0.0))>=14.9,"partial JSON snapshot records approximately 15 seconds of gameplay")
 check(bool(game.get("perf_active")),"15-second snapshot keeps the benchmark running")
 game.call("_perf_stop",false)
 var key:=InputEventKey.new()
 key.keycode=KEY_F9
 key.pressed=true
 game.call("_unhandled_key_input",key)
 check(bool(game.get("perf_active")),"F9 begins gameplay FPS recording")
 for i in range(120):game.call("_perf_sample",1.0/60.0)
 var report:Dictionary=game.call("_perf_report")
 check(int(report.get("frames",0))==120,"records every measured frame")
 check(absf(float(report.get("average_fps",0.0))-60.0)<0.02,"average FPS computed from elapsed time")
 check(absf(float(report.get("p99_frame_ms",0.0))-16.6667)<0.1,"p99 frame time computed")
 check(absf(float(report.get("p50_frame_ms",0.0))-16.6667)<0.1,"median frame time computed")
 check(absf(float(report.get("p95_frame_ms",0.0))-16.6667)<0.1,"p95 frame time computed")
 check(absf(float(report.get("one_percent_low_fps",0.0))-60.0)<0.02,"one-percent-low FPS computed")
 # A single 100ms frame must lower the tail metric while preserving the
 # regular median, making occasional hitching visible in performance reports.
 game.call("_perf_sample",0.1)
 var stutter_report:Dictionary=game.call("_perf_report")
 check(absf(float(stutter_report.get("p50_frame_ms",0.0))-16.6667)<0.1,"single hitch does not distort median")
 check(float(stutter_report.get("p99_frame_ms",0.0))<100.0 and absf(float(stutter_report.get("worst_frame_ms",0.0))-100.0)<0.1,"p99 and worst metrics distinguish isolated hitch")
 check(float(stutter_report.get("one_percent_low_fps",0.0))<25.0,"one-percent-low FPS detects isolated long frame")
 check(str(report.get("region_at_start",""))!="","report records region")
 check(str(report.get("graphics_quality_at_start",""))!="","report records quality")
 game.call("_perf_stop",false)
 check(not bool(game.get("perf_active")),"recording stops safely")
 # Profiling should resume itself while gameplay continues without F9.
 # SceneTree emits process_frame before game nodes receive _process.
 await process_frame
 await process_frame
 check(bool(game.get("perf_active")),"gameplay automatically resumes performance capture")
 game.call("_perf_start")
 game.call("_perf_sample",1.0/60.0)
 game.call("_reset_new_adventure")
 check(not bool(game.get("perf_active")),"new game resets in-progress benchmark")
 game.queue_free()
 if failed==0:print("MOONFALL v30.1 PROFILER PASS")
 quit(0 if failed==0 else 1)
