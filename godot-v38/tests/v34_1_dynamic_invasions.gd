extends SceneTree
## Requires Godot 4.7.2; automatically executed during BUILD_WINDOWS.bat.
var failures:int=0
func check(ok:bool,reason:String)->void:
 if ok:print("PASS lunar invasions: "+reason)
 else:
  failures+=1
  push_error("FAIL lunar invasions: "+reason)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var rules=load("res://scripts/systems/lunar_invasions_v341.gd")
 check(rules.wave_size(1)==2 and rules.wave_size(2)==3 and rules.wave_size(3)==1,"bounded wave sizes")
 check(rules.add_pressure(3,true)==4 and rules.add_pressure(4,true)==4,"pressure cap")
 check(rules.add_pressure(2,false)==2,"ineligible kills never escalate")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var sites:Array=game.get("v341_sites")
 check(sites.size()==3,"three invasion sites")
 var all_dormant:bool=true
 for entry in sites:
  var roster:Array=entry["roster"]
  if roster.size()!=4:all_dormant=false
  for id in roster:
   if bool(game.get("beasts")[int(id)].get("alive",false)):all_dormant=false
 check(all_dormant,"all squads dormant at game start")
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 game.call("_v341_add_pressure",0)
 check(int(game.get("v341_active_site"))==0,"hostile defeat pressure opens the first invasion")
 check(int(game.get("v341_wave"))==1,"starts with first wave")
 var roster:Array=sites[0]["roster"]
 var active_count:int=0
 var beasts:Array=game.get("beasts")
 for id in roster:
  if bool(beasts[int(id)].get("alive",false)):active_count+=1
 check(active_count==2,"first wave is two enemies")
 # Advance via normal death processing, not by mutating stages directly.
 for slot in range(2):game.call("_defeat_beast",beasts[int(roster[slot])],false)
 check(int(game.get("v341_wave"))==2,"second wave starts after all enemies die")
 for slot in range(3):game.call("_defeat_beast",beasts[int(roster[slot])],false)
 check(int(game.get("v341_wave"))==3,"commander appears after wave two")
 check(bool(beasts[int(roster[3])].get("alive",false)),"commander alive")
 game.call("_defeat_beast",beasts[int(roster[3])],false)
 check(int(game.get("v341_active_site"))==-1,"commander victory seals the invasion")
 check(int(game.get("v341_invasions_completed"))==1,"one victory counted")
 game.queue_free()
 print("MOONFALL v34.1 DYNAMIC INVASIONS PASS" if failures==0 else "MOONFALL v34.1 DYNAMIC INVASIONS FAIL")
 quit(0 if failures==0 else 1)
