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
 # Real animation polling must continue behind the live main menu.
 # Previously its menu_open guard froze imported GLB creature poses despite
 # the world clock and enemy attack AI continuing to advance.
 var animation_before:int=int(game.get("ai_art_anim_poll_cycle"))
 game.set("ai_art_anim_clock",0.0)
 game.call("_ai_art_update_animations",0.1)
 check(int(game.get("ai_art_anim_poll_cycle"))>animation_before,"Imported creature animations keep polling behind Raven Atlas")
 # Simulate a loaded regional guardian using a live enemy body, so this
 # regression exercises the real trait-attack loop without spawning actors.
 var guardian_body:CharacterBody3D=enemy.get("body") as CharacterBody3D
 if is_instance_valid(guardian_body):
  var original_guard_pos:Vector3=guardian_body.global_position
  guardian_body.global_position=player.global_position+Vector3(0.0,0.0,4.0)
  var test_guardian:Dictionary={
   "alive":true,"zone":int(game.get("region_index")),"body":guardian_body,
   "regional_guardian_trait":0,
   "trait_warning_until":float(game.get("map_time"))+4.0,
   "trait_next_at":float(game.get("map_time"))+10.0
  }
  var guardian_actors:Dictionary=game.get("atlas_regional_guardian_actors")
  guardian_actors[-9999]=test_guardian
  game.set("atlas_regional_guardian_actors",guardian_actors)
  var warning_before:float=float(test_guardian["trait_warning_until"])
  game.set("guardian_trait_attack_clock",0.0)
  game.call("_guardian_update_trait_attacks",0.12)
  check(is_equal_approx(float(test_guardian["trait_warning_until"]),warning_before),"Regional guardian warning stays queued while Raven Atlas is open")
  game.set("death_active",true)
  game.set("guardian_trait_attack_clock",0.0)
  game.call("_guardian_update_trait_attacks",0.12)
  check(is_zero_approx(float(test_guardian["trait_warning_until"])),"Real death still cancels regional guardian warning")
  game.set("death_active",false)
  # A wall encountered DURING an announced attack must block the hit.
  # Exercise real physics line-of-sight with an actual layer-1 collider,
  # not a mocked boss decision or an invented warning path.
  var saved_hero_position:Vector3=player.global_position
  var saved_hero_hp:float=float(game.get("hp"))
  var was_physics_active:bool=game.is_physics_processing()
  game.set_physics_process(false)
  guardian_body.global_position=Vector3(-40.0,80.0,0.0)
  player.global_position=Vector3(-36.0,80.0,0.0)
  var wall:StaticBody3D=StaticBody3D.new()
  wall.collision_layer=1
  wall.collision_mask=1
  var shape_node:CollisionShape3D=CollisionShape3D.new()
  var wall_shape:BoxShape3D=BoxShape3D.new()
  wall_shape.size=Vector3(0.6,3.0,5.0)
  shape_node.shape=wall_shape
  wall.add_child(shape_node)
  game.add_child(wall)
  wall.global_position=Vector3(-38.0,80.45,0.0)
  await physics_frame
  var blocked:bool=not bool(game.call("_has_clear_sight",guardian_body,player))
  check(blocked,"Physical cover breaks sight between regional guardian and hero")
  test_guardian["trait_origin"]=guardian_body.global_position
  test_guardian["trait_target"]=player.global_position
  test_guardian["trait_facing"]=Vector2.RIGHT
  test_guardian["trait_warning_until"]=float(game.get("map_time"))-0.001
  game.set("mounted",false)
  game.set("dodge_time",0.0)
  var guardian_hits_before:int=int(game.get("guardian_trait_hits"))
  var guardian_counters_before:int=int(game.get("guardian_trait_counters"))
  game.set("guardian_trait_attack_clock",0.0)
  game.call("_guardian_update_trait_attacks",0.12)
  check(int(game.get("guardian_trait_hits"))==guardian_hits_before,"Regional guardian strike cannot damage through newly reached cover")
  check(int(game.get("guardian_trait_counters"))>guardian_counters_before,"Cover is counted as a successful guardian counter")
  wall.queue_free()
  await physics_frame
  check(bool(game.call("_has_clear_sight",guardian_body,player)),"Without cover the guardian has clear sight again")
  test_guardian["trait_warning_until"]=float(game.get("map_time"))-0.001
  game.set("guardian_trait_attack_clock",0.0)
  game.call("_guardian_update_trait_attacks",0.12)
  check(int(game.get("guardian_trait_hits"))==guardian_hits_before+1,"Unobstructed marked guardian strike still lands")
  player.global_position=saved_hero_position
  game.set("hp",saved_hero_hp)
  game.set_physics_process(was_physics_active)
  guardian_actors.erase(-9999)
  game.set("atlas_regional_guardian_actors",guardian_actors)
  guardian_body.global_position=original_guard_pos
 else:
  check(false,"Regional guardian live-menu regression has an actual enemy body")
 check(float(enemy.get("windup",-1.0))==4.0 and bool(enemy.get("v404_attack_pending",false)) and is_equal_approx(float(enemy.get("v404_tell_next",-1.0)),planned),"Opening main menu does not cancel pending attacks")

 var tabs:PackedStringArray=["Character","World","Wallet","Inventory","Materials","Hero Skills","Tamer","Professions","Hunter","Bestiary","System"]
 for tab in tabs:
  game.call("_menu_tab",tab)
  var before:float=float(game.get("map_time"))
  for frame in range(3):await physics_frame
  check(menu.visible and bool(game.get("menu_open")),"Tab remains open: "+tab)
  check(float(game.get("map_time"))>before,"World time advances in "+tab)

 check(float(game.get("v385_mana"))>15.0,"Mana recovers across all Raven Atlas tabs")
 check(float(game.get("v387_stamina"))>15.0,"Stamina recovers across all Raven Atlas tabs")

 # The two divine world bosses must not lose queued signature windups just
 # because a player is browsing a regular menu.
 var divine:Dictionary={}
 for candidate in game.get("beasts"):
  if bool(candidate.get("alive",false)) and candidate.has("divine_id"):
   divine=candidate
   break
 if divine.is_empty():
  check(false,"Divine boss available for menu telegraph verification")
 else:
  var boss_body:CharacterBody3D=divine.get("body") as CharacterBody3D
  if is_instance_valid(boss_body):
   boss_body.global_position=player.global_position+Vector3(0.0,0.0,4.0)
   divine["zone"]=int(game.get("region_index"))
   divine["boss_phase"]=1
   var tell_until:float=float(game.get("map_time"))+5.0
   divine["divine_warn_until"]=tell_until
   divine["divine_next_at"]=tell_until+8.0
   divine["divine_chain_pending"]=true
   game.call("_update_beast_step",divine,0.0)
   check(is_equal_approx(float(divine.get("divine_warn_until",0.0)),tell_until),"Open menu preserves divine boss windup")
   check(bool(divine.get("divine_chain_pending",false)),"Open menu preserves divine follow-up")
   divine["divine_warn_until"]=0.0
   divine["divine_chain_pending"]=false
  else:
   check(false,"Divine boss has a playable body")

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
 animation_before=int(game.get("ai_art_anim_poll_cycle"))
 game.set("ai_art_anim_clock",0.0)
 game.call("_ai_art_update_animations",0.1)
 check(int(game.get("ai_art_anim_poll_cycle"))>animation_before,"Imported creature animations keep polling beneath the map")
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

 # Raven Atlas should never open beneath another full-screen overlay.
 var talents:Control=game.get("v398_panel") as Control
 check(is_instance_valid(talents),"Talent overlay exists")
 if is_instance_valid(talents):
  game.call("_v398_toggle_tree")
  check(talents.visible,"Talent overlay opens before Raven Atlas")
  game.call("_toggle_menu")
  check(menu.visible and not talents.visible,"Raven Atlas immediately closes talent overlay")
  game.call("_toggle_menu")
 game.call("_toggle_atlas")
 check(atlas.visible,"Map reopens for menu-stacking check")
 game.call("_toggle_menu")
 check(menu.visible and not atlas.visible,"Raven Atlas immediately closes map overlay")
 game.call("_toggle_menu")

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
