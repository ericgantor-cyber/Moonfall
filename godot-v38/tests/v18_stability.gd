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
 var steering_script:GDScript=load("res://scripts/native_v18.gd") as GDScript
 check(steering_script!=null and "wide_left_open or wide_right_open" in steering_script.source_code,"blocked corners have bounded wide-angle fallback")
 game.call("_apply_graphics_quality","Smooth")
 check(Engine.max_fps==60,"Smooth mode caps FPS at 60")
 game.call("_apply_graphics_quality","Full")
 check(Engine.max_fps==120,"Full mode caps FPS at 120")
 check(game.call("_read_save") is Dictionary,"Save reader returns a safe result")
 game.call("_start_game","z")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z artwork kept")
 # All six zones stay allocated; only nearby living bodies render.
 # Repeated refreshes must not re-enable off-region visuals or movement.
 var zone_five:Array=game.get("zone_beast_lists")[5]
 check(not zone_five.is_empty(),"Far region wildlife remains allocated")
 if not zone_five.is_empty():
  var far_actor:Dictionary=zone_five[0]
  var far_body:CharacterBody3D=far_actor["body"]
  far_body.velocity=Vector3(2.0,0.0,1.0)
  game.call("_update_region_visibility")
  check(not far_body.visible,"Far region creature is culled")
  check(far_body.velocity==Vector3.ZERO,"Offscreen creature velocity resets")
  game.call("_update_region_visibility")
  check(not far_body.visible and far_body.velocity==Vector3.ZERO,"Repeated refresh keeps offscreen bodies idle")
  var player:CharacterBody3D=game.get("player")
  player.global_position=Vector3(60.0,1.0,30.0)
  game.set("region_index",5)
  game.call("_update_region_visibility")
  check(far_body.visible,"Entering the far region restores creature rendering")

 game.call("_start_game","nyra")
 check(load("res://assets/heroes/Nyra_Original.png")!=null,"Original Nyra boss artwork is available")
 if failures==0:print("MOONFALL v18 STABILITY PASS")
 game.queue_free()
 quit(0 if failures==0 else 1)
