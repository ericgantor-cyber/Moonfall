extends SceneTree
## Optional Godot 4.7.2 headless world-depth interaction smoke test.
var failures:int=0
func check(ok:bool,reason:String)->void:
 if ok:print("PASS world depth: "+reason)
 else:
  failures+=1
  push_error("FAIL world depth: "+reason)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 check((game.get("depth_corruption") as Array).size()==3,"three corruption sites")
 check((game.get("depth_glyphs") as Array).size()==9,"three glyphs per rift")
 check((game.get("depth_travelers") as Array).size()==3,"three non-building lore NPCs")
 var rifts:Array=game.get("rifts")
 var site:int=0
 var anchor:Vector3=rifts[site]["anchor"]
 (game.get("player") as CharacterBody3D).global_position=anchor+Vector3(0,0.8,0)
 check(int(game.call("_depth_near_corruption"))==site,"rift corruption proximity")
 rifts[site]["cleared"]=true
 check(int(game.call("_depth_near_corruption"))==-1,"sealing a rift removes corruption")
 rifts[site]["cleared"]=false
 rifts[site]["active"]=true
 rifts[site]["stage"]=1
 game.set("active_rift",site)
 var glyphs:Array=game.get("depth_glyphs")
 var entry:Dictionary=glyphs[0]
 (game.get("player") as CharacterBody3D).global_position=(entry["node"] as Node3D).global_position+Vector3(0,0.85,0)
 game.call("_interact")
 check(int(game.get("depth_spell_charges"))==1,"first G interaction grants one charge")
 game.call("_interact")
 check(int(game.get("depth_spell_charges"))==1,"same glyph cannot be farmed")
 game.set("ability_cooldown",0.0)
 game.call("_spell","nova")
 check(int(game.get("depth_spell_charges"))==0,"Moon Nova consumes one charged echo")
 game.call("_depth_reset_glyphs")
 check((game.get("depth_glyph_charged") as Dictionary).is_empty(),"death/sanctuary clears temporary glyph state")
 game.queue_free()
 print("MOONFALL WORLD DEPTH PASS" if failures==0 else "MOONFALL WORLD DEPTH FAIL")
 quit(0 if failures==0 else 1)
