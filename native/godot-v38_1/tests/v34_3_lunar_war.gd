extends SceneTree
## Runtime sanity for phase-based commanders. Execute only in isolated test profile.
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS lunar war: "+label)
 else:
  failures+=1
  push_error("FAIL lunar war: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var tactics=load("res://scripts/systems/invasion_tactics_v343.gd")
 check(tactics.commander_phase(100.0,100.0)==1,"high-health phase")
 check(tactics.commander_phase(55.0,100.0)==2,"middle-health phase")
 check(tactics.commander_phase(20.0,100.0)==3,"final eclipse phase")
 check(tactics.marker_count(0,3)==2 and tactics.marker_count(1,1)==2,"telegraph budget")
 check(tactics.target_positions(0,Vector3.ZERO,2,3).size()==2,"lightning crossover")
 check(tactics.target_positions(2,Vector3.ZERO,2,3).size()==2,"void echo")
 check(tactics.perfect_seal(2,0) and not tactics.perfect_seal(1,0) and not tactics.perfect_seal(2,1),"clean-seal rules")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 game.call("_v341_start",0)
 check(int(game.get("v341_wave"))==1,"invasion starts")
 var ids:Array=game.get("v341_sites")[0]["roster"]
 var bs:Array=game.get("beasts")
 for i in range(2):game.call("_defeat_beast",bs[int(ids[i])],false)
 for i in range(3):game.call("_defeat_beast",bs[int(ids[i])],false)
 check(int(game.get("v341_wave"))==3,"commander reached")
 var boss:Dictionary=bs[int(ids[-1])]
 boss["hp"]=float(boss["max"])*0.2
 game.call("_v343_update_phase")
 check(int(game.get("v343_phase"))==3,"commander transitions to final phase")
 game.call("_v342_begin_strike")
 var marks:Array=game.get("v342_warnings")[0]["markers"]
 check((marks[0] as MeshInstance3D).visible and (marks[1] as MeshInstance3D).visible,"final phase has two dodgeable marks")
 game.call("_v342_hide_warnings")
 game.queue_free()
 print("MOONFALL v34.3 LUNAR WAR PASS" if failures==0 else "MOONFALL v34.3 LUNAR WAR FAIL")
 quit(0 if failures==0 else 1)
