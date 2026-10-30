extends SceneTree
var errors:=0
func check(ok:bool,msg:String)->void:
 if ok:print("PASS v19: "+msg)
 else:
  errors+=1
  push_error("FAIL v19: "+msg)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Main scene parses")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v19_scenery_count")==54,"54 decorative clusters across six biomes")
 check(game.get("v19_scenery_roots").size()==6,"Each zone gets its own art root")
 check(game.get("v19_crystals").size()==9,"Quarry crystals ready for bounded animation")
 check(game.get("grid_blocked").size()==2400,"Navigation grid preserved")
 for zone in range(6):
  var art:Node3D=game.get("v19_scenery_roots")[zone]
  check(art.get_child_count()==9,"Zone "+str(zone)+" has nine set pieces")
  check(art.get_parent()==game.get("decorative_groups")[zone],"Set pieces are cullable with their zone")
 game.call("_start_game","z")
 game.call("_apply_graphics_quality","Smooth")
 check(Engine.max_fps==60,"Smooth graphics caps FPS")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra original art preserved")
 if errors==0:print("MOONFALL v19 WORLD ART PASS")
 game.queue_free()
 quit(0 if errors==0 else 1)
