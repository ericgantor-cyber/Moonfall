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
 # Forty-four additional camera safety cases: 11 each for zoom, orbit, pitch and collision.
 var zoom_inputs:=[-100.0,-10.0,0.0,12.0,15.0,17.5,19.0,20.5,25.0,31.0,100.0]
 var last_zoom:float=RULES.MIN_DISTANCE
 for value in zoom_inputs:
  var distance:float=RULES.zoom_from_legacy(value)
  check(distance>=RULES.MIN_DISTANCE and distance<=RULES.MAX_DISTANCE and distance>=last_zoom,"zoom bounds and ordering "+str(value))
  last_zoom=distance
 var yaws:=[-6.28,-3.14,-1.57,-0.78,0.0,0.78,1.57,2.35,3.14,4.71,6.28]
 for yaw in yaws:
  var offset:Vector3=RULES.camera_offset(yaw,RULES.DEFAULT_PITCH,RULES.DEFAULT_DISTANCE)
  check(absf(offset.length()-RULES.DEFAULT_DISTANCE)<0.001,"3D orbit radius "+str(yaw))
 var pitch_inputs:=[-10.0,-3.0,-1.0,-0.5,-0.1,0.0,0.1,0.5,1.0,3.0,10.0]
 for delta in pitch_inputs:
  var pitch:float=RULES.orbit_pitch(RULES.DEFAULT_PITCH,delta)
  check(pitch>=RULES.MIN_PITCH and pitch<=RULES.MAX_PITCH,"orbit vertical safety "+str(delta))
 var hit_distances:=[0.0,0.1,1.0,2.0,2.9,3.0,4.0,6.0,10.0,20.0,40.0]
 for hit in hit_distances:
  var safe:float=RULES.safe_distance(hit)
  check(safe>=2.4 and is_equal_approx(safe,maxf(2.4,hit-0.55)),"camera collision clearance "+str(hit))
 # 44 additional deterministic movement and camera geometry cases.
 # Eight direction vectors across multiple yaw angles, plus edge cases.
 var directions:=[Vector3(0,0,-1),Vector3(0,0,1),Vector3(-1,0,0),Vector3(1,0,0),Vector3(-1,0,-1),Vector3(1,0,-1),Vector3(-1,0,1),Vector3(1,0,1)]
 var movement_yaws:=[0.0,0.78539816339,1.57079632679,3.14159265359,4.71238898038]
 for yaw in movement_yaws:
  for direction in directions:
   var mapped:Vector3=RULES.move_relative(direction,yaw)
   check(mapped.is_finite() and is_equal_approx(mapped.y,0.0) and mapped.length()<=1.001 and mapped.length()>0.01,"camera-relative movement direction "+str(yaw)+" "+str(direction))
 for direction in [Vector3.ZERO,Vector3(0,2,0),Vector3(0,-2,0),Vector3(0.0001,0,0.0001)]:
  var mapped:Vector3=RULES.move_relative(direction,0.0)
  check(mapped.is_finite() and is_equal_approx(mapped.y,0.0) and mapped.length()<=1.001,"degenerate movement input "+str(direction))
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
