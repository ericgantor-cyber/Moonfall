extends SceneTree
var failures := 0
func check(ok: bool, message: String) -> void:
 if ok:print("PASS v24.1: " + message)
 else:
  failures += 1
  push_error("FAIL v24.1: " + message)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene != null, "project scene loads")
 if scene == null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("grid_regions").size() == 2400, "invisible grid unchanged")
 check(game.get("v24_elite_markers").size() == 8, "elite warning pool retained")
 game.call("_start_game", "z")
 game.call("_v24_set_zoom", 22.0)
 var zoom_key := InputEventKey.new()
 zoom_key.keycode=KEY_EQUAL
 zoom_key.pressed=true
 game.call("_unhandled_input", zoom_key)
 check(game.get("camera").projection==Camera3D.PROJECTION_PERSPECTIVE and is_equal_approx(float(game.get("v24_camera_size")),20.5) and is_equal_approx(float(game.get("v36_distance")),13.2), "keyboard zoom in updates third-person distance")
 zoom_key.keycode=KEY_MINUS
 game.call("_unhandled_input", zoom_key)
 check(is_equal_approx(float(game.get("v24_camera_size")),22.0) and is_equal_approx(float(game.get("v36_distance")),14.4), "keyboard zoom out updates third-person distance")
 game.set("menu_open", true)
 game.call("_unhandled_input", zoom_key)
 check(is_equal_approx(float(game.get("v24_camera_size")),22.0) and is_equal_approx(float(game.get("v36_distance")),14.4), "menus block gameplay zoom")
 game.set("menu_open", false)
 var victim:Dictionary={}
 for beast in game.get("beasts"):
  if int(beast.get("zone", -1)) == 0 and bool(beast.get("alive",false)) and not bool(beast.get("boss",false)):
   victim=beast
   break
 check(not victim.is_empty(), "test target found")
 if not victim.is_empty():
  game.get("player").global_position=victim["body"].global_position+Vector3(1.4,0,0)
  game.call("_apply_graphics_quality", "Smooth")
  victim["hp"]=float(victim["max"])
  game.call("_apply_player_hit",victim,4.0)
  check(game.get("accessible_hit_count") >= 1,"combat label receives damage events")
  check(game.get("accessible_hit_text").visible,"damage feedback visible in Smooth mode")
  game.call("_reset_new_adventure")
  check(not game.get("accessible_hit_text").visible,"new game clears damage feedback")
 game.call("_start_game", "nyra")
 check(game.get("hero_sprite").texture == load("res://assets/heroes/Nyra_Original.png"),"Nyra original art unchanged")
 game.call("_start_game", "z")
 check(game.get("hero_sprite").texture == load("res://assets/heroes/Z_Original.webp"),"Z original art unchanged")
 game.queue_free()
 if failures == 0:print("MOONFALL v24.1 ACCESSIBILITY PASS")
 quit(0 if failures == 0 else 1)
