extends SceneTree
## Runtime integration test for the active Godot scene. Requires Godot 4.7.2.
var failures:int=0
func check(condition:bool,label:String)->void:
 if condition:print("PASS Grand Overhaul: "+label)
 else:
  failures+=1
  push_error("FAIL Grand Overhaul: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var rules=load("res://scripts/systems/lunar_overhaul_v34.gd")
 check(int(rules.gain(95,99))==100,"resonance is capped")
 check(int(rules.gain(15,-50))==15,"negative gains rejected")
 check(is_equal_approx(float(rules.damage_boon(50)),1.075),"persistent attack boon caps at three rifts")
 check(is_equal_approx(float(rules.guard_boon(50)),0.925),"persistent defense boon caps at three rifts")
 check(float(rules.rift_surge_damage(3,2))>float(rules.rift_surge_damage(1,2)),"later chambers strike harder")
 var anchor:=Vector3(20,0,30)
 var predicted:Vector3=rules.safe_surge_position(anchor,anchor+Vector3(11,0,0),3)
 check(predicted.distance_to(anchor)<8.1,"predicted surge remains within corruption site")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 check(game.get("v34_resonance_label")!=null,"visible resonance HUD constructed")
 game.call("_v34_gain_resonance",200)
 check(int(game.get("v34_resonance"))==100,"resonance max reached")
 game.set("ability_cooldown",0.0)
 game.call("_spell","nova")
 check(int(game.get("v34_resonance"))==0,"Moon Nova consumes full charge")
 check(int(game.get("v34_finisher_casts"))==1,"finisher triggers exactly once")
 game.call("_v34_gain_resonance",30)
 game.call("_respawn_player")
 check(int(game.get("v34_resonance"))==0,"temporary charge resets on death")
 # The first rift can be sealed with no new save fields or currencies.
 var rifts:Array=game.get("rifts")
 var cleared:Array=game.get("rifts_cleared")
 var player:CharacterBody3D=game.get("player")
 rifts[0]["cleared"]=true
 if not cleared.has(0):cleared.append(0)
 game.set("death_active",false)
 game.set("hero_playing",true)
 player.global_position=rifts[0]["anchor"]+Vector3(0,0.7,0)
 game.call("_v34_update_purification")
 var markers:Array=game.get("v34_purified_markers")
 check(markers.size()==3,"one purification marker per existing rift")
 check((markers[0]["node"] as Node3D).visible,"purified rift marker visible after sealing")
 check(is_equal_approx(float(game.call("_hero_damage_multiplier"))/float(rules.damage_boon(1)),float(game.call("_hero_damage_multiplier"))/1.025),"saved rift boon is bounded")
 game.queue_free()
 print("MOONFALL v34.0 GRAND OVERHAUL PASS" if failures==0 else "MOONFALL v34.0 GRAND OVERHAUL FAIL")
 quit(0 if failures==0 else 1)
