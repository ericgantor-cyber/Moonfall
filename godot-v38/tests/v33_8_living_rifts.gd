extends SceneTree
## Godot 4.7.2 headless gameplay checks, run with an isolated test profile.
var failures:int=0
func check(ok:bool,what:String)->void:
 if ok:print("PASS living rifts: "+what)
 else:
  failures+=1
  push_error("FAIL living rifts: "+what)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var rifts:Array=game.get("rifts")
 var hazards:Array=game.get("depth_surge_nodes")
 check(hazards.size()==3,"one noncolliding telegraph per rift")
 var player:CharacterBody3D=game.get("player")
 var hazard:Node3D=hazards[0]["node"]
 player.global_position=hazard.global_position+Vector3(0.0,0.7,0.0)
 game.set("menu_open",false)
 for tick in range(45):game.call("_depth_surge_tick",0.15)
 check(int(game.get("depth_surge_phase"))==1,"surge provides warning before impact")
 check(hazard.visible,"telegraph is visible during warning")
 var previous_hp:float=game.get("hp")
 player.global_position=hazard.global_position+Vector3(7.0,0.7,0.0)
 for tick in range(12):game.call("_depth_surge_tick",0.15)
 check(not hazard.visible,"telegraph disappears after impact")
 check(is_equal_approx(float(game.get("hp")),previous_hp),"walking away avoids damage")
 # Dashing prevents damage even while standing in the surge.
 game.call("_depth_reset_surge")
 player.global_position=hazard.global_position+Vector3(0.0,0.7,0.0)
 for tick in range(45):game.call("_depth_surge_tick",0.15)
 game.set("dodge_time",2.0)
 previous_hp=float(game.get("hp"))
 for tick in range(12):game.call("_depth_surge_tick",0.15)
 check(is_equal_approx(float(game.get("hp")),previous_hp),"dodge avoids damage")
 game.set("dodge_time",0.0)
 # Attuning all chambers creates an optional reward condition, never a new currency.
 game.call("_depth_reset_glyphs")
 var glyph_charged:Dictionary=game.get("depth_glyph_charged")
 for stage in range(1,4):glyph_charged["0:"+str(stage)]=true
 check(int(game.call("_depth_attunement_count",0))==3,"three unique glyphs tracked")
 check(int(game.call("_depth_attunement_count",1))==0,"other rifts stay independent")
 var entry:Dictionary=rifts[0]
 entry["cleared"]=false
 entry["active"]=true
 entry["stage"]=3
 game.set("active_rift",0)
 for index in entry["slots"]:
  var beast:Dictionary=(game.get("beasts") as Array)[int(index)]
  beast["alive"]=false
 var mats:Dictionary=game.get("materials")
 var sigils:int=int(mats.get("Moon Sigil",0))
 var cinders:int=int(mats.get("Dusk Cinder",0))
 game.call("_on_rift_enemy_defeated",{"dungeon_site":0})
 check(bool(entry["cleared"]),"final chamber seals the same world rift")
 check(int(mats.get("Moon Sigil",0))==sigils+2 and int(mats.get("Dusk Cinder",0))==cinders+1,"one-time bonus awarded for three glyphs")
 game.call("_on_rift_enemy_defeated",{"dungeon_site":0})
 check(int(mats.get("Moon Sigil",0))==sigils+2,"sealed rift cannot pay again")
 game.queue_free()
 print("MOONFALL LIVING RIFTS PASS" if failures==0 else "MOONFALL LIVING RIFTS FAIL")
 quit(0 if failures==0 else 1)
