extends SceneTree
const BUDGET:=preload("res://scripts/systems/ai_budget.gd")
var failed:int=0
func check(condition:bool,message:String)->void:
 if condition:print("PASS v30: "+message)
 else:
  failed+=1
  push_error("FAIL v30: "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"v30 script is active")
 game.call("_start_game","z")
 var creature_lists:Array=game.get("zone_beast_lists")
 check(creature_lists.size()==6,"all six regions remain available")
 var player:CharacterBody3D=game.get("player")
 check(is_instance_valid(player),"player exists")
 check(int(game.get("GRID_TILES"))==2400,"invisible collision grid retained")
 # Time slicing should distribute different idle slots across four simulation frames.
 var slots:Array[int]=[0,0,0,0]
 for index in range(40):
  for frame in range(1,5):
   if BUDGET.should_tick(frame,index):slots[posmod(frame-1,4)]+=1
 check(slots.min()>=10 and slots.max()<=10,"40 background actors evenly stagger over four frames")
 var first:Dictionary={}
 var boss:Dictionary={}
 for beast in creature_lists[0]:
  if str(beast.get("kind",""))=="stag" and first.is_empty():first=beast
  if bool(beast.get("boss",false)):boss=beast
 check(not first.is_empty(),"passive wildlife present")
 if not first.is_empty():
  var body:CharacterBody3D=first["body"]
  body.global_position=Vector3(-87.0,0.6,-56.0)
  body.velocity=Vector3.ZERO
  first["home"]=body.global_position
  first["engaged"]=false
  first["ecology_target"]={}
  first["scared_until"]=0.0
  var skipped_before:int=game.get("v28_idle_skips")
  for step in range(28):game.call("_update_beasts",1.0/60.0)
  check(int(game.get("v28_idle_skips"))>skipped_before,"distant inactive wildlife gets a CPU budget")
  check(float(first.get("v28_sleep_dt",0.0))<=0.25,"accumulated idle time is capped")
 var profile:Dictionary=game.call("v30_diagnostics")
 check(int(profile.get("sampled_frames",0))>=28,"new profile collects direct update samples")
 check(is_finite(float(profile.get("average_ai_usec",-1.0))) and float(profile.get("average_ai_usec",-1.0))>=0.0,"AI profile is finite")
 check(int(profile.get("active_updates",0))>0,"active AI continues to update")
 game.call("_v24_set_zoom",25.0)
 var key:=InputEventKey.new()
 key.pressed=true
 key.keycode=KEY_0
 game.call("_unhandled_input",key)
 check(is_equal_approx(float(game.get("v24_camera_size")),19.0),"keyboard camera reset")
 game.call("_v24_set_zoom",24.0)
 var pad:=InputEventJoypadButton.new()
 pad.pressed=true
 pad.button_index=JOY_BUTTON_RIGHT_STICK
 game.call("_unhandled_input",pad)
 # The right-stick click was reassigned to target lock in v38; camera zoom must remain unchanged.
 check(is_equal_approx(float(game.get("v24_camera_size")),24.0),"controller target-lock input does not reset camera zoom")
 game.queue_free()
 if failed==0:print("MOONFALL v30 RELEASE PASS")
 quit(0 if failed==0 else 1)
