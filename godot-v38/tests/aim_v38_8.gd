extends SceneTree
## Headless aiming smoke checks. Run with: godot --headless --path godot-v38 --script res://tests/aim_v38_8.gd
var failures:int=0
func check(value:bool,message:String)->void:
 if value:print("PASS aim: "+message)
 else:
  failures+=1
  push_error("FAIL aim: "+message)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var base:GDScript=load("res://scripts/native_v38_7.gd") as GDScript
 var ui:GDScript=load("res://scripts/native_v38_8.gd") as GDScript
 check(base!=null and ui!=null,"aim scripts parse and load")
 if base!=null:
  var source:String=base.source_code
  check("JOY_AXIS_RIGHT_X" in source and "JOY_AXIS_RIGHT_Y" in source,"controller right stick wired")
  check("project_ray_origin" in source and "project_ray_normal" in source,"mouse targeting uses camera ray")
  check("aim_locked_id" in source and "_toggle_aim_lock" in source,"target lock toggle exists")
  check("event.button_index==MOUSE_BUTTON_MIDDLE" in source,"mouse aim lock uses middle-click")
  check(not ("event.button_index==MOUSE_BUTTON_RIGHT" in source),"aim lock does not capture right-click guard")
  var guard:GDScript=load("res://scripts/native_v38_0.gd") as GDScript
  check(guard!=null and "event.button_index==MOUSE_BUTTON_RIGHT" in guard.source_code and "v392_guard_started=map_time" in guard.source_code,"right-click remains bound to inherited parry window")
  check(guard!=null and "not event.alt_pressed" in guard.source_code,"Alt-right camera orbit does not activate guard/parry")
  check(guard!=null and "or v36_dragging or not Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT)" in guard.source_code,"camera orbit cannot block incoming damage as held guard")
  check(guard!=null and "super._apply_incoming_damage(raw)" in guard.source_code,"camera-orbit hit follows inherited damage path")
  var camera_script:GDScript=load("res://scripts/native_v34_4.gd") as GDScript
  check(camera_script!=null,"third-person camera script loads")
  if camera_script!=null:
   var camera_source:String=camera_script.source_code
   check("elif event.alt_pressed and hero_playing" in camera_source,"camera orbit requires Alt-right-drag")
   check("v36_dragging=false" in camera_source,"releasing right mouse ends camera drag")
   check("get_viewport().set_input_as_handled()" in camera_source,"camera drag consumes its own orbit input")
  var tips:GDScript=load("res://scripts/systems/tips_codex_v376.gd") as GDScript
  check(tips!=null and "Alt and right-drag" in tips.source_code,"in-game controls describe separate orbit and guard inputs")
  check("if aim_locked_id>=0 and locked.is_empty():aim_locked_id=-1" in source,"invalid lock-on targets are released")
  check("body.get_instance_id()==aim_locked_id" in source,"target lock compares actor identity without scanning the beast catalog")
  check("aim_locked_id=target_body.get_instance_id()" in source,"target lock captures stable actor instance ID")
  check(not ("beasts.find(target)" in source or "beasts.find(beast)" in source),"aiming avoids dictionary equality scans")
  check("if not hero_playing or menu_open or death_active or wait_open or wait_transition_active:return" in source,"aimed flame blocked during non-gameplay states")
  check("if which!=\"fire\":" in source and "super._spell(which)" in source,"other spells retain inherited behavior")
 if ui!=null:
  var source:String=ui.source_code
  check("AIM_PREFS_PATH" in source and "config.save(AIM_PREFS_PATH)" in source,"aim assist setting persisted")
  check("aim_input_source==\"mouse\"" in source,"mouse aiming restored with connected controller")
  check("body.get_instance_id()==aim_locked_id" in source,"locked marker uses stable actor identity")
  check("aim_options.visible=hero_playing and not menu_open and not death_active" in source,"aim controls hidden outside gameplay")
 # Divine final-phase attack telegraphs have a bounded, visible second strike.
 var divine:GDScript=load("res://scripts/native_v38_8.gd") as GDScript
 if divine!=null:
  var source:String=divine.source_code
  check("beast[\"divine_chain_pending\"]=true" in source,"phase-three divine attack schedules a follow-up")
  check("if not chained and int(beast.get(\"boss_phase\",1))==3" in source,"divine follow-up occurs once only in phase three")
  check("beast[\"divine_warn_until\"]=map_time+1.15" in source,"second divine strike has a visible dodge window")
  check("actor[\"divine_chain_pending\"]=false" in source,"divine chain cancels when warning is cleared")
  check("warning.visible=true" in source,"follow-up reuses visible warning mesh")
  check("or wait_open or wait_transition_active:" in source,"boss signatures clear during rest and travel")
  check("if int(beast.get(\"zone\",-1))!=region_index:" in source,"off-region signature attacks cancel")
  check("distance_squared_to(player.global_position)>25.0*25.0" in source,"out-of-range telegraphs cancel before damage")
  check("v387_stamina=minf(V387_MAX_STAMINA,v387_stamina+5.0)" in source,"timed dodges earn bounded stamina")
  check("VOID REBOUND!" in source and "ECLIPSE REBOUND!" in source,"boss follow-up alerts identify attack type")
 # WoW-inspired optional action layers: eight gamepad combos, six keyboard
 # shortcuts, and no interference with inherited unmodified controls.
 if ui!=null:
  var combo_game:Node3D=load("res://scenes/main.tscn").instantiate()
  root.add_child(combo_game)
  await process_frame
  var left_buttons:Array[int]=[JOY_BUTTON_X,JOY_BUTTON_Y,JOY_BUTTON_B,JOY_BUTTON_A]
  var left_actions:Array[String]=["dash","nova","fire","jump"]
  var right_actions:Array[String]=["senses","companion","mount","interact"]
  for i in range(left_buttons.size()):
   check(str(combo_game.call("_wow_combo_action",left_buttons[i],true,false))==left_actions[i],"LT modifier routes face button "+str(i))
   check(str(combo_game.call("_wow_combo_action",left_buttons[i],false,true))==right_actions[i],"RT modifier routes face button "+str(i))
   check(str(combo_game.call("_wow_combo_action",left_buttons[i],false,false)).is_empty(),"unmodified face button "+str(i)+" untouched")
   check(str(combo_game.call("_wow_combo_action",left_buttons[i],true,true)).is_empty(),"simultaneous triggers do not double-fire")
  var keys:Array[int]=[KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6]
  var key_actions:Array[String]=["dash","nova","fire","senses","companion","mount"]
  for i in range(keys.size()):
   check(str(combo_game.call("_wow_keyboard_action",keys[i]))==key_actions[i],"Alt+number shortcut "+str(i+1))
  check(str(combo_game.call("_wow_keyboard_action",KEY_SPACE)).is_empty(),"spacebar is not remapped as an attack")
  combo_game.queue_free()
 quit(1 if failures>0 else 0)
