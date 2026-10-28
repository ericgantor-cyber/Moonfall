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
 check(str(report.get("region_at_start",""))!="","report records region")
 check(str(report.get("graphics_quality_at_start",""))!="","report records quality")
 game.call("_perf_stop",false)
 check(not bool(game.get("perf_active")),"recording stops safely")
 game.call("_perf_start")
 game.call("_perf_sample",1.0/60.0)
 game.call("_reset_new_adventure")
 check(not bool(game.get("perf_active")),"new game resets in-progress benchmark")
 game.queue_free()
 if failed==0:print("MOONFALL v30.1 PROFILER PASS")
 quit(0 if failed==0 else 1)
