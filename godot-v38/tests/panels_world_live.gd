extends SceneTree
## Playable-scene test: every Raven Atlas tab and the separate map overlay
## must leave the world live. Use the isolated CI save profile, never user saves.
var failures:int=0

func check(ok:bool, label:String)->void:
 if ok:print("PASS live panels: "+label)
 else:
  failures+=1
  push_error("FAIL live panels: "+label)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var sandbox:String=OS.get_environment("MOONFALL_TEST_PROFILE").replace("\\","/").trim_suffix("/").to_lower()
 var home:String=OS.get_user_data_dir().replace("\\","/").to_lower()
 if sandbox.is_empty() or not home.begins_with(sandbox+"/"):
  push_error("Unsafe user profile. Set an isolated MOONFALL_TEST_PROFILE.")
  quit(2)
  return
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 if scene==null:
  check(false,"Playable scene loads")
  quit(1)
  return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 for frame in range(3):await physics_frame
 var player:CharacterBody3D=game.get("player") as CharacterBody3D
 var menu:Control=game.get("menu_panel") as Control
 var atlas:Control=game.get("atlas_panel") as Control
 if not is_instance_valid(player) or not is_instance_valid(menu) or not is_instance_valid(atlas):
  check(false,"Hero, Raven Atlas menu, and map overlay exist")
  game.queue_free()
  quit(1)
  return

 var enemy:Dictionary={}
 for candidate in game.get("beasts"):
  if bool(candidate.get("alive",false)) and bool(candidate.get("hostile",false)):
   enemy=candidate
   break
 if enemy.is_empty():
  check(false,"Hostile exists for queued-attack regression")
  game.queue_free()
  quit(1)
  return
 var planned:float=float(game.get("map_time"))+10.0
 enemy["windup"]=4.0
 enemy["v404_attack_pending"]=true
 enemy["v404_tell_next"]=planned
 enemy["v407_release_until"]=planned

 # Resources must recover during informational panels while the world
 # continues to advance. Make the recovery deterministic and observable.
 game.set("v385_mana",15.0)
 game.set("v387_stamina",15.0)
 game.set("v387_stamina_regen_wait",0.0)
 game.call("_toggle_menu")
 check(menu.visible and bool(game.get("menu_open")),"Raven Atlas menu opens")
 check(float(enemy.get("windup",-1.0))==4.0 and bool(enemy.get("v404_attack_pending",false)) and is_equal_approx(float(enemy.get("v404_tell_next",-1.0)),planned),"Opening main menu does not cancel pending attacks")

 var tabs:PackedStringArray=["Character","World","Wallet","Inventory","Materials","Hero Skills","Tamer","Professions","Hunter","Bestiary","System"]
 for tab in tabs:
  game.call("_menu_tab",tab)
  var before:float=float(game.get("map_time"))
  for frame in range(3):await physics_frame
  check(menu.visible and bool(game.get("menu_open")),"Tab remains open: "+tab)
  check(float(game.get("map_time"))>before,"World time advances in "+tab)

 var move_down:InputEventKey=InputEventKey.new()
 move_down.keycode=KEY_D
 move_down.physical_keycode=KEY_D
 move_down.pressed=true
 Input.parse_input_event(move_down)
 var position_before:Vector3=player.global_position
 for frame in range(18):await physics_frame
 var move_up:InputEventKey=InputEventKey.new()
 move_up.keycode=KEY_D
 move_up.physical_keycode=KEY_D
 move_up.pressed=false
 Input.parse_input_event(move_up)
 check(player.global_position.distance_squared_to(position_before)>0.01,"Hero can move behind the main menu")

 game.set("hp",float(game.get("max_hp")))
 var hp_before:float=float(game.get("hp"))
 game.call("_apply_incoming_damage",12.0)
 check(float(game.get("hp"))<hp_before,"Incoming combat damage applies during main menu")

 game.call("_toggle_menu")
 check(not menu.visible and not bool(game.get("menu_open")),"Raven Atlas closes")
 game.call("_toggle_atlas")
 check(atlas.visible,"Separate map overlay opens")
 var atlas_clock:float=float(game.get("map_time"))
 for frame in range(5):await physics_frame
 check(float(game.get("map_time"))>atlas_clock,"World time advances under the map")
 game.set("hp",float(game.get("max_hp")))
 hp_before=float(game.get("hp"))
 game.call("_apply_incoming_damage",12.0)
 check(float(game.get("hp"))<hp_before,"Incoming damage applies under the map")
 game.call("_toggle_atlas")
 check(not atlas.visible,"Separate map closes")
 check(is_equal_approx(float(enemy.get("v404_tell_next",-1.0)),planned),"Closing overlays does not postpone enemy attacks")

 # The death screen is a genuine game-state transition, not an inventory pause.
 game.call("_toggle_menu")
 game.set("phoenix_used_this_expedition",true)
 game.set("hp",10.0)
 game.call("_apply_incoming_damage",100000.0)
 check(bool(game.get("death_active")),"Fatal damage in main menu reaches death UI")
 check(not menu.visible,"Main menu cannot conceal death UI")
 game.queue_free()
 print("MOONFALL ALL PANELS LIVE "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
