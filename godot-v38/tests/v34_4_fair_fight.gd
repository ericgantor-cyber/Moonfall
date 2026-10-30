extends SceneTree
var failures:int=0
func check(ok:bool,description:String)->void:
 if ok:print("PASS v34.4: "+description)
 else:
  failures+=1
  push_error("FAIL v34.4: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var rules=load("res://scripts/systems/invasion_fairness_v344.gd")
 check(rules.rest_acceleration(1)==0.0,"phase 1 baseline")
 check(rules.rest_acceleration(2)>0.0 and rules.rest_acceleration(2)<rules.rest_acceleration(3),"aggression increases with health phase")
 check(rules.rest_acceleration(3)<=rules.MAX_EXTRA,"aggression capped")
 check(rules.recovered_during_hit(false,true,false),"Phoenix revive disqualifies flawless")
 check(not rules.recovered_during_hit(true,true,false),"no repeated Phoenix penalty")
 check(not rules.recovered_during_hit(false,true,true),"existing registered hit not counted twice")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 game.call("_v341_start",0)
 check(int(game.get("v341_wave"))==1,"invasions integrated")
 var ids:Array=game.get("v341_sites")[0]["roster"]
 var beasts:Array=game.get("beasts")
 for i in range(2):game.call("_defeat_beast",beasts[int(ids[i])],false)
 for i in range(3):game.call("_defeat_beast",beasts[int(ids[i])],false)
 check(int(game.get("v341_wave"))==3,"commander phase reached")
 game.set("v343_real_hits",0)
 game.set("hp",1.0)
 game.set("phoenix_used_this_expedition",false)
 game.get("materials")["Moon Phoenix Feather"]=1
 game.get("companion").clear()
 game.call("_apply_incoming_damage",30000.0)
 check(bool(game.get("phoenix_used_this_expedition")),"Phoenix triggered")
 check(int(game.get("v343_real_hits"))>=1,"revival correctly counts as non-flawless")
 game.queue_free()
 print("MOONFALL v34.4 FAIR FIGHT PASS" if failures==0 else "MOONFALL v34.4 FAIR FIGHT FAIL")
 quit(0 if failures==0 else 1)
