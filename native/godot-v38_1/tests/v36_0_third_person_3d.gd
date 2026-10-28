extends SceneTree
## Real headless engine regression for v36 perspective, movement and original art.
const RULES:=preload("res://scripts/systems/third_person_camera_v36.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,reason:String)->void:
 if ok:print("PASS 3D: "+reason)
 else:
  failures+=1
  push_error("FAIL 3D: "+reason)
func _run()->void:
 check(absf(RULES.camera_offset(0.0,RULES.DEFAULT_PITCH,12.0).length()-12.0)<0.001,"camera orbit is at 3D distance")
 check(RULES.zoom_from_legacy(17.5)<RULES.DEFAULT_DISTANCE,"wheel zooms closer")
 check(RULES.zoom_from_legacy(20.5)>RULES.DEFAULT_DISTANCE,"wheel zooms farther")
 check(RULES.safe_distance(10.0)<10.0,"collision avoids scenery")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 var follow_camera:Camera3D=game.get("camera")
 check(is_instance_valid(follow_camera),"3D camera instantiated")
 if is_instance_valid(follow_camera):
  check(follow_camera.projection==Camera3D.PROJECTION_PERSPECTIVE,"perspective projection active")
  check(absf(follow_camera.fov-66.0)<0.01,"vertical FOV is configured")
 check(is_instance_valid(game.get("hero_sprite")),"original hero billboard preserved")
 var moved:Vector3=game.call("_map_movement_to_camera",Vector3(0.0,0.0,-1.0))
 check(moved.z<0.0 and moved.x<0.0,"forward input follows camera yaw")
 var before:float=game.get("v36_distance")
 game.call("_v24_set_zoom",17.5)
 check(float(game.get("v36_distance"))<before,"legacy zoom keys map to third-person distance")
 game.queue_free()
 print("MOONFALL 3D CAMERA PASS" if failures==0 else "MOONFALL 3D CAMERA FAIL")
 quit(0 if failures==0 else 1)
