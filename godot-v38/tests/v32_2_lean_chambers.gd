extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var fails:int=0
func check(ok:bool,name:String)->void:
 if ok:print("PASS v32.2 "+name)
 else:
  fails+=1
  push_error("FAIL v32.2 "+name)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"Release script wired")
 check((game.get("v321_chambers") as Array).size()==9,"Original nine chambers")
 check((game.get("region_nodes") as Array).size()==6,"Original six regions")
 check((game.get("rifts") as Array).size()==3,"Original three rifts")
 game.call("_start_game","z")
 await process_frame
 game.call("_v321_update_chambers")
 var before:int=int(game.get("v322_label_writes"))
 game.call("_v321_update_chambers")
 check(int(game.get("v322_label_writes"))==before,"Stable labels do not rewrite")
 var metrics:Dictionary=game.call("_perf_report")
 check(CURRENT_RELEASE.matches_report(metrics),"Profiler release updated")
 check(metrics.has("dungeon_label_writes_skipped") and metrics.has("dungeon_visibility_switches"),"Profiler instrumentation present")
 game.queue_free()
 print("v32.2 PASS" if fails==0 else "v32.2 FAIL")
 quit(0 if fails==0 else 1)
