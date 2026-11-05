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
 # A diagnostic request before the first captured frame must never index
 # a negative sample position or produce a fake low-FPS value.
 game.call("_perf_start",true)
 var empty_report:Dictionary=game.call("_perf_report")
 check(int(empty_report.get("frames",-1))==0,"empty performance capture has zero frames")
 check(float(empty_report.get("one_percent_low_fps",-1.0))==0.0,"empty performance capture safely reports zero one-percent-low FPS")
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
