extends SceneTree
## Verifies malformed invasion squads cannot start or grant free rewards.
var failures:int=0
func check(ok:bool,reason:String)->void:
 if ok:print("PASS invasion guard: "+reason)
 else:
  failures+=1
  push_error("FAIL invasion guard: "+reason)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var sites:Array=game.get("v341_sites")
 check(game.call("_v341_roster_valid",0),"normal roster valid")
 var old_ids:Array=sites[0]["roster"].duplicate()
 sites[0]["roster"]=[]
 check(not game.call("_v341_roster_valid",0),"empty roster rejected")
 game.call("_v341_start",0)
 check(int(game.get("v341_active_site"))==-1 and int(game.get("v341_invasions_completed"))==0,"no phantom victory or stuck invasion")
 sites[0]["roster"]=[999999]
 check(not game.call("_v341_roster_valid",0),"out-of-bounds actor rejected")
 game.call("_v341_reset")
 check(int(game.get("v341_active_site"))==-1,"reset survives invalid actor references")
 sites[0]["roster"]=old_ids
 check(game.call("_v341_roster_valid",0),"valid squad restored")
 game.call("_v341_start",0)
 check(int(game.get("v341_wave"))==1,"normal invasion still works")
 game.queue_free()
 print("MOONFALL INVASION GUARD PASS" if failures==0 else "MOONFALL INVASION GUARD FAIL")
 quit(0 if failures==0 else 1)
