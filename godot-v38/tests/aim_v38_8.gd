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
  var aimed_spell:String=source.split("func _spell(which:String)->void:",true,1)[1]
  check('var cost:float=_v385_spell_mana_cost("fire")' in aimed_spell,"aimed fire uses shared mana cost")
  check(aimed_spell.find("if target.is_empty():")<aimed_spell.find("v385_mana-=cost"),"no-target fire cannot consume mana")
  check(aimed_spell.find("if v385_mana<cost:")<aimed_spell.find("v385_mana-=cost"),"insufficient mana stops aimed fire")
  check(aimed_spell.find("v385_mana-=cost")<aimed_spell.find("ability_cooldown="),"valid fire spends mana before cooldown")
  check('v406_damage_context="magic"' in aimed_spell and 'v406_damage_context=""' in aimed_spell,"aimed fire uses magic talent scaling")
  var mana_base:GDScript=load("res://scripts/native_v38_0.gd") as GDScript
  check(mana_base!=null and "func _v385_spell_mana_cost(" in mana_base.source_code and "var cost:float=_v385_spell_mana_cost(which)" in mana_base.source_code,"regular spells and aimed fire share talent-aware cost")
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
  # Regression: persistent boss IDs are identifiers, not array offsets.
  # Verify each divine actor resolves through the new lookup and that
  # absent IDs never accidentally select an unrelated beast.
  var divine_ids:Dictionary=combo_game.get("divine_boss_ids")
  for divine_id in divine_ids:
   var boss_id:int=int(divine_ids[divine_id])
   var actor:Dictionary=combo_game.call("_divine_actor_by_boss_id",boss_id)
   check(not actor.is_empty() and str(actor.get("divine_id",""))==str(divine_id),"divine boss ID resolves to correct actor: "+str(divine_id))
  var absent:Dictionary=combo_game.call("_divine_actor_by_boss_id",-999999)
  check(absent.is_empty(),"missing divine boss ID never resolves to another actor")
  combo_game.call("_clear_all_divine_warnings")
  for divine_id in divine_ids:
   var actor:Dictionary=combo_game.call("_divine_actor_by_boss_id",int(divine_ids[divine_id]))
   check(float(actor.get("divine_warn_until",-1.0))==0.0 and not bool(actor.get("divine_chain_pending",true)),"warning cleanup cancels queued divine strikes: "+str(divine_id))
  # Live negative-path test: an invalid region has no valid target, so
  # Eclipse Flame must leave both mana and cooldown untouched.
  combo_game.set("region_index",-1)
  combo_game.set("hero_playing",true)
  combo_game.set("menu_open",false)
  combo_game.set("death_active",false)
  combo_game.set("wait_open",false)
  combo_game.set("wait_transition_active",false)
  combo_game.set("mounted",false)
  combo_game.set("ability_cooldown",0.0)
  combo_game.set("v385_mana",100.0)
  combo_game.set("v398_ranks",{})
  check(is_equal_approx(float(combo_game.call("_v385_spell_mana_cost","fire")),30.0),"base Eclipse Flame costs 30 mana")
  combo_game.call("_spell","fire")
  check(is_equal_approx(float(combo_game.get("v385_mana")),100.0),"aimed fire does not consume mana without a target")
  check(is_zero_approx(float(combo_game.get("ability_cooldown"))),"aimed fire does not start cooldown without a target")
  # Exercise the real enemy catalog and target acquisition without creating
  # replacement combat actors or touching the original hero artwork.
  var catalogs:Array=combo_game.get("zone_beast_lists")
  var target:Dictionary={}
  if not catalogs.is_empty():
   for candidate in catalogs[0]:
    var candidate_body:CharacterBody3D=candidate.get("body") as CharacterBody3D
    if bool(candidate.get("alive",false)) and bool(candidate.get("hostile",false)) and is_instance_valid(candidate_body):
     target=candidate
     break
  check(not target.is_empty(),"starting region has a live hostile for aimed spell tests")
  if not target.is_empty():
   var hero_body:CharacterBody3D=combo_game.get("player") as CharacterBody3D
   var target_body:CharacterBody3D=target.get("body") as CharacterBody3D
   hero_body.global_position=Vector3(0.0,50.0,0.0)
   target_body.global_position=Vector3(0.0,50.0,-3.0)
   target["hp"]=10000.0
   target["max"]=10000.0
   combo_game.set("region_index",0)
   combo_game.set("aim_direction",Vector3.FORWARD)
   combo_game.set("aim_locked_id",target_body.get_instance_id())
   var selected:Dictionary=combo_game.call("_aim_candidate",13.0)
   check(not selected.is_empty() and selected.get("body")==target_body,"real aiming selects the visible hostile")
   if not selected.is_empty():
    combo_game.set("v398_ranks",{"magic_focus":1,"magic_mastery":1})
    var cost:float=float(combo_game.call("_v385_spell_mana_cost","fire"))
    check(is_equal_approx(cost,25.0),"two magic talents discount Eclipse Flame to 25 mana")
    combo_game.set("v385_mana",cost-0.5)
    combo_game.set("ability_cooldown",0.0)
    var hp_before:float=float(target["hp"])
    combo_game.call("_spell","fire")
    check(is_equal_approx(float(combo_game.get("v385_mana")),cost-0.5),"valid target with low mana spends nothing")
    check(is_zero_approx(float(combo_game.get("ability_cooldown"))),"valid target with low mana starts no cooldown")
    check(is_equal_approx(float(target["hp"]),hp_before),"valid target with low mana takes no damage")
    combo_game.set("v385_mana",cost)
    combo_game.call("_spell","fire")
    check(is_zero_approx(float(combo_game.get("v385_mana"))),"successful aimed cast charges exactly one discounted mana cost")
    check(float(combo_game.get("ability_cooldown"))>0.0,"successful aimed cast starts cooldown")
    check(float(target["hp"])<hp_before,"successful aimed cast damages the selected hostile")
    check(str(combo_game.get("v406_damage_context")).is_empty(),"magic context resets after an aimed hit")
    var hp_after:float=float(target["hp"])
    combo_game.set("v385_mana",cost)
    combo_game.call("_spell","fire")
    check(is_equal_approx(float(combo_game.get("v385_mana")),cost),"cooldown prevents a second mana deduction")
    check(is_equal_approx(float(target["hp"]),hp_after),"cooldown prevents a second hit")
    combo_game.set("ability_cooldown",0.0)
    for blocked_state in ["menu_open","death_active","wait_open","wait_transition_active","mounted"]:
     combo_game.set(blocked_state,true)
     combo_game.call("_spell","fire")
     check(is_equal_approx(float(combo_game.get("v385_mana")),cost) and is_zero_approx(float(combo_game.get("ability_cooldown"))) and is_equal_approx(float(target["hp"]),hp_after),"aimed flame blocked during "+blocked_state)
     combo_game.set(blocked_state,false)
  combo_game.queue_free()
 quit(1 if failures>0 else 0)
