extends SceneTree
var failures := 0
func check(ok:bool, text:String) -> void:
 if ok:print("PASS stability: "+text)
 else:
  failures += 1
  push_error("FAIL stability: "+text)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Main scene parses")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.has_method("_terrain_grid_steer"),"Invisible terrain navigation retained")
 check(game.get("grid_regions").size()==2400,"Grid allocations are bounded")
 game.call("_apply_graphics_quality","Smooth")
 check(Engine.max_fps==60,"Smooth mode caps FPS at 60")
 game.call("_apply_graphics_quality","Full")
 check(Engine.max_fps==120,"Full mode caps FPS at 120")
 check(game.call("_read_save") is Dictionary,"Save reader returns a safe result")
 game.call("_start_game","z")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z artwork kept")
 game.call("_start_game","nyra")
 check(load("res://assets/heroes/Nyra_Original.png")!=null,"Original Nyra boss artwork is available")
 if failures==0:print("MOONFALL v18 STABILITY PASS")
 game.queue_free()
 quit(0 if failures==0 else 1)
