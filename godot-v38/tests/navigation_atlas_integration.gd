extends SceneTree
const NAV=preload("res://scripts/systems/navigation_map.gd")
var failures:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS atlas: "+description)
 else:
  failures+=1
  push_error("FAIL atlas: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var map:Control=NAV.new()
 root.add_child(map)
 map.size=Vector2(300,240)
 map.call("update_navigation",Vector3(10,0,20),Vector3(10,0,-80),true)
 check(bool(map.get("has_waypoint")),"waypoint enabled")
 check(map.get("waypoint_position")==Vector3(10,0,-80),"waypoint keeps world coordinates")
 map.set("metres_per_half_width",300.0)
 check(is_equal_approx(float(map.get("metres_per_half_width")),300.0),"zoom controls map range")
 map.call("set_discovered",[Vector3(10,0,20)])
 check(map.get("discovered").size()==1,"visited positions render as map markers")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"map-enabled main scene loads")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  check(game.has_method("_toggle_atlas"),"map toggle exists")
  check(game.has_method("_atlas_select_waypoint"),"click-to-mark handler exists")
  check(game.has_method("_atlas_zoom"),"zoom handler exists")
  var data:Dictionary=game.call("_checkpoint_extra_data")
  check(data.has("atlas_zoom_radius"),"save persists zoom")
  check(data.has("atlas_waypoint"),"save persists waypoint")
  check(data.has("atlas_discovered"),"save persists discovered locations")
  game.queue_free()
 map.queue_free()
 print("MOONFALL ATLAS "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
