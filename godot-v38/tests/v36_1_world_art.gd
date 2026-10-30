extends SceneTree
## v36.1 world art integration test, run inside a Godot 4.7 engine.
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,description:String)->void:
 if ok:print("PASS ART: "+description)
 else:
  failures+=1
  push_error("FAIL ART: "+description)
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 var markers:Array=game.get("v361_landmarks")
 var rift_markers:Array=game.get("v361_rift_emblems")
 check(markers.size()==6,"six region landmarks created")
 check(rift_markers.size()==(game.get("rifts") as Array).size(),"every rift has an art marker")
 check(is_instance_valid(game.get("v361_aura")),"hero ground aura exists")
 check((game.get("living_environment") as Environment).background_mode==Environment.BG_SKY,"lunar panorama is the real 3D background")
 for monument in markers:
  check((monument as Node3D).get_child_count()>0,"each region monument has visible 3D structure")
 game.call("_v361_art_quality")
 check((game.get("hero_sprite") as Sprite3D).texture!=null,"original Sprite3D image is preserved")
 game.queue_free()
 print("MOONFALL v36.1 WORLD ART PASS" if failures==0 else "MOONFALL v36.1 WORLD ART FAIL")
 quit(0 if failures==0 else 1)
