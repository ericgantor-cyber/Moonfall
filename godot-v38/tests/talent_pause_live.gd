extends SceneTree
## Run the actual playable scene and prove the talent web freezes world combat.
## The test profile must be isolated from a real player's checkpoints.
var failures:int=0

func check(ok:bool,label:String)->void:
 if ok:print("PASS live talent pause: "+label)
 else:
  failures+=1
  push_error("FAIL live talent pause: "+label)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var sandbox:String=OS.get_environment("MOONFALL_TEST_PROFILE").replace("\\","/").trim_suffix("/").to_lower()
 var data_dir:String=OS.get_user_data_dir().replace("\\","/").to_lower()
 if sandbox.is_empty() or not data_dir.begins_with(sandbox+"/"):
  push_error("Unsafe test save location: run with an isolated MOONFALL_TEST_PROFILE")
  quit(2)
  return
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 if scene==null:
  _finish(false,"Playable scene loads")
  return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 for frame in range(3):
  await physics_frame
 var panel:PanelContainer=game.get("v398_panel") as PanelContainer
 if not is_instance_valid(panel) or not bool(game.get("hero_playing")):
  check(false,"Talent panel and playable hero initialize")
  game.queue_free()
  quit(1)
  return
 check(not panel.visible,"Talent web starts closed")
 var beast:Dictionary={}
 for candidate in game.get("beasts"):
  if bool(candidate.get("alive",false)) and bool(candidate.get("hostile",false)):
   beast=candidate
   break
 if beast.is_empty():
  check(false,"An active hostile exists for live pause verification")
  game.queue_free()
  quit(1)
  return
 # Give a real hostile multiple pending attack states before opening the web.
 beast["windup"]=0.8
 beast["elite_lunge_windup"]=0.7
 beast["v404_attack_pending"]=true
 beast["v404_tell_until"]=20.0
 beast["v407_release_until"]=20.0
 var player:CharacterBody3D=game.get("player") as CharacterBody3D
 var start_pos:Vector3=player.global_position
 var start_hp:float=float(game.get("hp"))
 game.call("_v398_toggle_tree")
 check(panel.visible and bool(game.call("_v406_talents_pause_enemies")),"Opening constellation enters full combat pause")
 check(is_zero_approx(float(beast.get("windup",-1.0))) and is_zero_approx(float(beast.get("elite_lunge_windup",-1.0))),"Pending ordinary and elite strikes are cancelled")
 check(not bool(beast.get("v404_attack_pending",true)) and is_zero_approx(float(beast.get("v407_release_until",-1.0))),"Pending bloodline and chained attacks are cancelled")
 var frozen_time:float=float(game.get("map_time"))
 game.call("_apply_incoming_damage",10.0)
 check(is_equal_approx(float(game.get("hp")),start_hp),"Incoming damage is ignored while the talent web is open")
 for frame in range(18):
  await physics_frame
 check(is_equal_approx(float(game.get("map_time")),frozen_time),"World physics clock stays frozen during 18 frames of talent browsing")
 check(player.global_position.distance_squared_to(start_pos)<0.0001,"Player is not moved by gameplay physics behind the talent menu")
 check(is_zero_approx(float(beast.get("windup",-1.0))),"Enemy attack countdown does not resume while paused")
 game.call("_v398_toggle_tree")
 check(not panel.visible and not bool(game.call("_v406_talents_pause_enemies")),"Closing constellation restores gameplay")
 for frame in range(12):
  await physics_frame
 check(float(game.get("map_time"))>frozen_time,"World physics clock resumes on menu close")
 game.queue_free()
 print("MOONFALL LIVE TALENT PAUSE "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)

func _finish(ok:bool,label:String)->void:
 check(ok,label)
 quit(0 if failures==0 else 1)
