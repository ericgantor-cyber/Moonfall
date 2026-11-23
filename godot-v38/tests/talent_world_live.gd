extends SceneTree
## Live playable-scene test: the talent web is an overlay, not combat pause.
## Isolate all checkpoint/config writes from the player's real save profile.
var failures:int=0

func check(ok:bool,label:String)->void:
 if ok:print("PASS live talent world: "+label)
 else:
  failures+=1
  push_error("FAIL live talent world: "+label)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var sandbox:String=OS.get_environment("MOONFALL_TEST_PROFILE").replace("\\","/").trim_suffix("/").to_lower()
 var home:String=OS.get_user_data_dir().replace("\\","/").to_lower()
 if sandbox.is_empty() or not home.begins_with(sandbox+"/"):
  push_error("Unsafe user profile. Use an isolated MOONFALL_TEST_PROFILE.")
  quit(2)
  return
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 if scene==null:
  check(false,"Playable Moonfall scene loads")
  quit(1)
  return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 for frame in range(3):await physics_frame
 var panel:PanelContainer=game.get("v398_panel") as PanelContainer
 var player:CharacterBody3D=game.get("player") as CharacterBody3D
 if not is_instance_valid(panel) or not is_instance_valid(player) or not bool(game.get("hero_playing")):
  check(false,"Playable hero and constellation panel initialize")
  game.queue_free()
  quit(1)
  return
 var enemy:Dictionary={}
 for candidate in game.get("beasts"):
  if bool(candidate.get("alive",false)) and bool(candidate.get("hostile",false)):
   enemy=candidate
   break
 if enemy.is_empty():
  check(false,"Live hostile is available")
  game.queue_free()
  quit(1)
  return
 var scheduled:float=float(game.get("map_time"))+5.0
 enemy["windup"]=0.8
 enemy["elite_lunge_windup"]=0.7
 enemy["v404_attack_pending"]=true
 enemy["v404_tell_until"]=scheduled
 enemy["v404_tell_next"]=scheduled
 enemy["v407_release_until"]=scheduled
 enemy["divine_warn_until"]=scheduled
 enemy["divine_next_at"]=scheduled
 var hp_before:float=float(game.get("hp"))
 var clock_before:float=float(game.get("map_time"))
 game.call("_v398_toggle_tree")
 check(panel.visible,"Constellation opens")
 check(is_equal_approx(float(enemy.get("windup",-1.0)),0.8) and is_equal_approx(float(enemy.get("elite_lunge_windup",-1.0)),0.7),"Opening talents keeps pending ordinary and elite attacks")
 check(bool(enemy.get("v404_attack_pending",false)) and is_equal_approx(float(enemy.get("v404_tell_next",-1.0)),scheduled),"Opening talents does not cancel or postpone boss tells")
 check(is_equal_approx(float(enemy.get("v407_release_until",-1.0)),scheduled) and is_equal_approx(float(enemy.get("divine_next_at",-1.0)),scheduled),"Opening talents retains combo and divine schedules")
 # Direct incoming combat damage must still reach the real player HP system.
 game.call("_apply_incoming_damage",12.0)
 check(float(game.get("hp"))<hp_before,"Incoming damage is not ignored behind talent panel")
 # Simulated D key remains held across physics ticks, even with UI visible.
 var down:InputEventKey=InputEventKey.new()
 down.keycode=KEY_D
 down.physical_keycode=KEY_D
 down.pressed=true
 Input.parse_input_event(down)
 var position_before:Vector3=player.global_position
 for frame in range(18):await physics_frame
 var up:InputEventKey=InputEventKey.new()
 up.keycode=KEY_D
 up.physical_keycode=KEY_D
 up.pressed=false
 Input.parse_input_event(up)
 check(float(game.get("map_time"))>clock_before+0.15,"World physics clock advances while browsing talents")
 check(player.global_position.distance_squared_to(position_before)>0.01,"Hero can move while the talent web stays open")
 check(panel.visible,"Constellation stays open while time advances")
 game.call("_v398_toggle_tree")
 check(not panel.visible,"Constellation closes")
 check(is_equal_approx(float(enemy.get("v404_tell_next",-1.0)),scheduled),"Closing talents does not delay a pending boss attack")
 game.queue_free()
 print("MOONFALL LIVE TALENT WORLD "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
