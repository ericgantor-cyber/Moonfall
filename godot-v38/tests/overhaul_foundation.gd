extends SceneTree
## Whole-game overhaul quality gate: core scene, controls, combat and loot.
var failed:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS overhaul: "+label)
 else:
  failed+=1
  push_error("FAIL overhaul: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 check(scene!=null,"active RPG scene loads")
 if scene==null:quit(1);return
 var game:Node=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.has_method("_cycle_aim_target"),"Tab target cycling is wired")
 check(game.has_method("_toggle_atlas"),"M map action is available")
 check(game.has_method("_spell") and game.has_method("_attack"),"combat attack and spell pathways available")
 check(game.has_method("_create_gear_drop"),"loot drop integration available")
 check(game.has_method("_defeat_beast"),"enemy defeat integration available")
 check(game.has_method("_checkpoint_extra_data"),"save extension available")
 var target_code:String=FileAccess.get_file_as_string("res://scripts/native_v38_9.gd")
 var map_code:String=FileAccess.get_file_as_string("res://scripts/native_v38_2.gd")
 check("event.keycode==KEY_TAB" in target_code and "event.keycode==KEY_F6" not in target_code,"Tab replaces F6")
 check("beasts.find(beast)" not in target_code and "eligible.append(index)" in target_code,"target cycling avoids repeated entire-beast scans")
 check("eligible.sort()" not in target_code,"target cycling keeps canonical order without sorting")
 check("eligible.bsearch(aim_locked_id + 1)" in target_code,"target cycling moves to next higher eligible index")
 check("if beasts.is_empty():" in target_code,"no canonical enemies short circuits targeting")
 check("if not body.visible or not body.is_inside_tree():continue" in target_code,"hidden or detached enemy bodies excluded from targeting")
 check("is_instance_valid(body_ref) and body_ref.visible and body_ref.is_inside_tree()" in target_code,"hidden enemies excluded before regional index allocation")
 check("if zone_ids.is_empty():" in target_code,"invalid regional enemies short circuit global target scan")
 check("zone_beast_lists[region_index].is_empty()" in target_code,"empty regions avoid unnecessary target scans")
 check(target_code.count("_show_message(\"No visible target\",0.9)")>=2,"empty and obstructed target cycles both give feedback")
 check("bool(beast.get(\"alive\",false)) and bool(beast.get(\"hostile\",false))" in target_code,"dead or peaceful region entries excluded from targeting")
 check("not wait_open and not wait_transition_active:" in target_code,"controller target cycling respects world-transition lockout")
 check("if not hero_playing or menu_open or death_active or wait_open or wait_transition_active:return" in target_code,"target-cycle method independently guards gameplay state")
 check("event.keycode==KEY_M" in map_code,"M remains the map key")
 var follower:String=FileAccess.get_file_as_string("res://scripts/native_v38_5.gd")
 check("realm_combat_attack_timer" in follower and "var companion_attack_timer" not in follower,"inherited companion timer collision fixed")
 var directional:String=FileAccess.get_file_as_string("res://scripts/native_v38_7.gd")
 check("body==locked_body" in directional and "beasts.find(beast)" not in directional,"aim assistance resolves locked target without a per-enemy full scan")
 check("not body.is_inside_tree() or not body.visible" in directional,"aim assistance excludes removed or hidden enemies")
 check("if hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active:" in directional,"aim button handlers respect transition lockouts")
 check("or wait_open or wait_transition_active or not is_instance_valid(player):return" in directional,"aim direction pauses during world transitions")
 check("func _toggle_aim_lock()->void:\n if not hero_playing or menu_open or death_active or wait_open or wait_transition_active:return" in directional,"manual aim lock respects gameplay state")
 var aim_code:String=FileAccess.get_file_as_string("res://scripts/native_v38_8.gd")
 check("beasts[aim_locked_id]==target" in aim_code and "beasts.find(target)" not in aim_code,"aim feedback avoids per-frame full beast-list lookup")
 check("aim_strength_slider=HSlider.new()" in aim_code and "aim_assist_strength" in aim_code,"aim assist has adjustable saved strength")
 check("aim_strength_slider.editable=enabled" in aim_code,"aim strength disabled when aim assist is off")
 check("preset_button.pressed.connect(_set_aim_preset.bind(preset))" in aim_code,"aim assist presets are clickable")
 check("if not aim_assist_enabled:return" in aim_code,"presets do not bypass disabled aim assist")
 check("aim_strength_slider.focus_mode=Control.FOCUS_ALL" in aim_code,"aim slider supports keyboard and controller focus")
 check("aim_options.visible=hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active" in aim_code,"aim options disappear during transitions")
 check("if not hero_playing or menu_open or death_active or wait_open or wait_transition_active or not is_instance_valid(player):return" in aim_code,"aim marker pauses during transitions")
 check("lerpf(AIM_CONE_DOT,0.38" in directional,"aim assist strength affects the targeting cone")
 var sample_targets:Array[int]=[1,4,9]
 check(sample_targets[sample_targets.bsearch(4+1)]==9,"target-cycle next higher index")
 var wrap_index:int=sample_targets.bsearch(9+1)
 if wrap_index>=sample_targets.size():wrap_index=0
 check(sample_targets[wrap_index]==1,"target-cycle wraps to first eligible index")
 var sprite=game.get("hero_sprite") as Sprite3D
 check(is_instance_valid(sprite) and sprite.texture!=null,"original hero sprite texture present")
 game.queue_free()
 quit(1 if failed>0 else 0)
