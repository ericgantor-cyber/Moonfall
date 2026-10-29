extends SceneTree
## v34.2 Godot runtime regression, must run in a sandboxed test profile.
var failed:int=0
func verify(ok:bool,label:String)->void:
 if ok:print("PASS elemental: "+label)
 else:
  failed+=1
  push_error("FAIL elemental: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var rules=load("res://scripts/systems/invasion_overhaul_v342.gd")
 verify(rules.marker_count(0)==1 and rules.marker_count(1)==2 and rules.marker_count(2)==1,"bounded region warning markers")
 verify(rules.bonus_material(0)=="Moon Sigil" and rules.bonus_material(1)=="Dusk Cinder" and rules.bonus_material(2)=="Elder Rootstone","regional material rewards")
 verify(rules.warning_seconds(0)>1.0 and rules.warning_seconds(2)>1.0,"visible warning windows")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var warnings:Array=game.get("v342_warnings")
 verify(warnings.size()==3,"three regions have prebuilt hazards")
 var nodes:int=0
 for site in warnings:
  nodes+=(site["markers"] as Array).size()
 verify(nodes==6,"fixed six warning nodes, no per-wave creation")
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 verify(int(game.get("v341_wave"))==1,"invasion stage starts with waves")
 var roster:Array=game.get("v341_sites")[0]["roster"]
 var beasts:Array=game.get("beasts")
 for slot in range(2):game.call("_defeat_beast",beasts[int(roster[slot])],false)
 for slot in range(3):game.call("_defeat_beast",beasts[int(roster[slot])],false)
 verify(int(game.get("v341_wave"))==3,"commander stage reached")
 game.call("_v342_begin_strike")
 verify(int(game.get("v342_phase"))==1,"strike is telegraphed")
 game.call("_v342_hide_warnings")
 verify(int(game.get("v342_phase"))==0,"hazards can be cancelled")
 game.call("_defeat_beast",beasts[int(roster[3])],false)
 verify(int(game.get("v342_faction_rewards"))==1,"faction reward granted once on commander defeat")
 game.queue_free()
 print("MOONFALL v34.2 ELEMENTAL INVASIONS PASS" if failed==0 else "MOONFALL v34.2 ELEMENTAL INVASIONS FAIL")
 quit(0 if failed==0 else 1)
