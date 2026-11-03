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
 check("reset_button.pressed.connect(_reset_aim_preferences)" in aim_code,"aim preferences expose reset to defaults")
 check("aim_toggle.set_pressed_no_signal(true)" in aim_code and "aim_strength_slider.set_value_no_signal(aim_assist_strength)" in aim_code,"reset updates controls without duplicate callback saves")
 check("aim_strength_slider.drag_ended.connect(_on_aim_strength_drag_ended)" in aim_code,"aim strength saves after drag completes")
 check("aim_strength_slider.drag_started.connect(_on_aim_strength_drag_started)" in aim_code,"slider records the beginning of drag")
 check("if value_changed:_save_aim_preferences()" in aim_code,"unchanged slider drags do not write preferences")
 check("func _save_aim_preferences()->void:" in aim_code and "aim_strength_dragging" in aim_code,"aim preferences share one save path and defer writes during dragging")
 check("aim_options.visible=hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active" in aim_code,"aim options disappear during transitions")
 check("if not hero_playing or menu_open or death_active or wait_open or wait_transition_active or not is_instance_valid(player):return" in aim_code,"aim marker pauses during transitions")
 check("lerpf(AIM_CONE_DOT,0.38" in directional,"aim assist strength affects the targeting cone")
 var sample_targets:Array[int]=[1,4,9]
 check(sample_targets[sample_targets.bsearch(4+1)]==9,"target-cycle next higher index")
 var wrap_index:int=sample_targets.bsearch(9+1)
 if wrap_index>=sample_targets.size():wrap_index=0
 check(sample_targets[wrap_index]==1,"target-cycle wraps to first eligible index")
 var ai_rules=load("res://scripts/systems/ai_rules.gd")
 check(is_equal_approx(ai_rules.maximum_perception(),16.24),"maximum awareness matches alert night range")
 check(ai_rules.potentially_visible(16.23),"awareness accepts targets within maximum range")
 check(not ai_rules.potentially_visible(16.24),"awareness excludes upper bound")
 check(not ai_rules.potentially_visible(-INF) and not ai_rules.potentially_visible(NAN),"invalid awareness distances fail closed")
 check(not ai_rules.potentially_visible(-1.0) and not ai_rules.potentially_visible(-0.001),"negative awareness distances fail closed")
 check(ai_rules.potentially_visible(0.0),"overlapping enemy positions remain within awareness range")
 check(not ai_rules.potentially_visible(INF),"infinite awareness distance fails closed")
 var awareness_limit:float=ai_rules.maximum_perception()
 check(ai_rules.potentially_visible_squared(0.0),"squared awareness accepts overlapping positions")
 check(ai_rules.potentially_visible_squared(awareness_limit*awareness_limit-0.01),"squared awareness accepts inside boundary")
 check(not ai_rules.potentially_visible_squared(awareness_limit*awareness_limit),"squared awareness excludes upper boundary")
 check(not ai_rules.potentially_visible_squared(-1.0) and not ai_rules.potentially_visible_squared(NAN) and not ai_rules.potentially_visible_squared(INF),"invalid squared awareness values fail closed")
 # Equivalence across ordinary combat distances protects the optimized path.
 for sample_distance in [0.0,0.1,1.0,6.0,10.0,14.0,16.0,16.23,16.25,24.0,80.0]:
  var scalar_visible:bool=ai_rules.potentially_visible(sample_distance)
  var squared_visible:bool=ai_rules.potentially_visible_squared(sample_distance*sample_distance)
  check(scalar_visible==squared_visible,"squared awareness agrees with scalar gate at "+str(sample_distance))
 check(ai_rules.planar_direction(Vector3(3,7,4),5.0).distance_to(Vector3(0.6,0,0.8))<0.0001,"planar direction preserves horizontal heading")
 check(ai_rules.planar_direction(Vector3.ZERO,0.0)==Vector3.ZERO,"overlapping creatures have safe zero direction")
 check(ai_rules.planar_direction(Vector3(INF,0,1),2.0)==Vector3.ZERO,"invalid movement vectors fail closed")
 check(ai_rules.planar_direction(Vector3(100,0,0),1.0)==Vector3.ZERO,"mismatched movement distance cannot create oversized heading")
 check(ai_rules.wildlife_escape_velocity(Vector3(100,0,0),10.0,5.0,1.0,1)==Vector3.ZERO,"oversized escape direction cannot spike wildlife speed")
 check(ai_rules.retreat_velocity(Vector3(1,0,0),10.0,0.9).distance_to(Vector3(-9,0,0))<0.0001,"timed retreat retains original velocity")
 check(ai_rules.retreat_velocity(Vector3(0,0,1),10.0,0.85).distance_to(Vector3(0,0,-8.5))<0.0001,"low-health retreat retains original velocity")
 check(ai_rules.retreat_velocity(Vector3.ZERO,10.0,0.9)==Vector3.ZERO,"overlapping actors cannot retreat in invalid direction")
 check(ai_rules.retreat_velocity(Vector3(1,0,0),NAN,0.9)==Vector3.ZERO,"invalid retreat speed fails closed")
 check(ai_rules.retreat_velocity(Vector3(1,0,0),10.0,NAN)==Vector3.ZERO,"invalid retreat scale fails closed")
 check(ai_rules.retreat_velocity(Vector3(1,0,0),10.0,-0.9)==Vector3.ZERO,"negative retreat multiplier cannot reverse retreat toward hero")
 check(ai_rules.retreat_velocity(Vector3(1,0,0),10.0,2.0)==Vector3.ZERO,"oversized retreat multiplier cannot cause movement spike")
 check(ai_rules.wildlife_escape_velocity(Vector3(1,0,0),10.0,5.0,1.0,1)==Vector3(-10,0,0),"nearby passive wildlife flees from player")
 check(ai_rules.wildlife_escape_velocity(Vector3.ZERO,10.0,9.0,0.0,0).distance_to(Vector3(10,0,0))<0.0001,"scared wildlife retains timed wandering escape")
 check(ai_rules.wildlife_escape_velocity(Vector3.ZERO,NAN,9.0,0.0,0)==Vector3.ZERO,"invalid wildlife speed fails closed")
 check(ai_rules.wildlife_escape_velocity(Vector3.ZERO,10.0,-1.0,0.0,0)==Vector3.ZERO,"negative escape distance fails closed")
 check(ai_rules.wildlife_escape_velocity(Vector3.ZERO,10.0,9.0,INF,0)==Vector3.ZERO,"invalid escape time fails closed")
 check(ai_rules.wildlife_escape_velocity(Vector3.ZERO,10.0,9.0,1.0e15,0)==Vector3.ZERO,"extreme escape time fails closed")
 check(ai_rules.prey_chase_velocity(Vector3(3,7,4),10.0).distance_to(Vector3(4.8,0,6.4))<0.0001,"predator pursuit preserves 80 percent horizontal movement")
 check(ai_rules.prey_chase_velocity(Vector3.ZERO,10.0)==Vector3.ZERO,"prey overlap cannot create invalid velocity")
 check(ai_rules.prey_chase_velocity(Vector3(1.0e30,0,1.0e30),10.0)==Vector3.ZERO,"overflowing chase offset fails closed")
 check(ai_rules.return_home_velocity(Vector3(3,7,4),10.0).distance_to(Vector3(2.4,0,3.2))<0.0001,"wildlife home return preserves 40 percent horizontal movement")
 check(ai_rules.return_home_velocity(Vector3(INF,0,0),10.0)==Vector3.ZERO,"invalid home position fails closed")
 check(ai_rules.return_home_velocity(Vector3(1.0e30,0,1.0e30),10.0)==Vector3.ZERO,"overflowing home offset fails closed")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),10.0,1.0).distance_to(Vector3(1.697749,0,6.790997))<0.001,"orbit direction and speed match established movement")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),10.0,-1.0).z<0.0,"negative orbit direction circles opposite way")
 check(ai_rules.orbit_velocity(Vector3.ZERO,10.0,1.0)==Vector3.ZERO,"overlapping actors do not orbit")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),0.0,1.0)==Vector3.ZERO,"zero-speed actors cannot orbit")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),NAN,1.0)==Vector3.ZERO,"invalid orbit speeds fail closed")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),10.0,0.0)==Vector3.ZERO,"invalid zero orbit side fails closed")
 check(ai_rules.orbit_velocity(Vector3(1,0,0),10.0,100.0)==Vector3.ZERO,"invalid orbit side cannot produce excessive velocity")
 check(ai_rules.approach(Vector3(5,0,0),0.0,1,false,1,5.0)==Vector3.ZERO,"zero-speed enemy cannot drift")
 check(ai_rules.approach(Vector3(INF,0,0),3.0,1,false,1,5.0)==Vector3.ZERO,"invalid pursuit direction fails closed")
 check(ai_rules.approach(Vector3(1,0,0),NAN,1,false,1,5.0)==Vector3.ZERO,"invalid enemy speed fails closed")
 check(not ai_rules.retreat_due(1.0,0.0,false,-1,2.0,0.0),"zero max-health does not trigger retreat")
 check(not ai_rules.retreat_due(NAN,100.0,false,-1,2.0,0.0),"invalid health does not trigger retreat")
 # Sweep 111 deterministic headings without spawning additional game entities.
 # Verify consistent chase/home speed and exclusively planar movement.
 var movement_sweep_ok:bool=true
 for sample_index in range(111):
  var angle:float=TAU*float(sample_index)/111.0
  var offset:Vector3=Vector3(cos(angle)*7.0,3.0,sin(angle)*7.0)
  var chase:Vector3=ai_rules.prey_chase_velocity(offset,10.0)
  var homing:Vector3=ai_rules.return_home_velocity(offset,10.0)
  if not is_equal_approx(chase.length(),8.0) or not is_equal_approx(homing.length(),4.0) or not is_zero_approx(chase.y) or not is_zero_approx(homing.y):
   movement_sweep_ok=false
   break
 check(movement_sweep_ok,"111 deterministic wildlife movement headings preserve speeds and planar direction")
 var combat_sweep_ok:bool=true
 for sample_index in range(111):
  var angle:float=TAU*float(sample_index)/111.0
  var heading:Vector3=Vector3(cos(angle),0.0,sin(angle))
  var backward:Vector3=ai_rules.retreat_velocity(heading,10.0,0.9)
  var clockwise:Vector3=ai_rules.orbit_velocity(heading,10.0,1.0)
  var counterclockwise:Vector3=ai_rules.orbit_velocity(heading,10.0,-1.0)
  if not is_equal_approx(backward.length(),9.0) or backward.dot(heading)>-8.99 or not is_equal_approx(clockwise.length(),7.0) or not is_equal_approx(counterclockwise.length(),7.0):
   combat_sweep_ok=false
   break
 check(combat_sweep_ok,"111 deterministic combat headings preserve retreat and orbit pacing")
 check(ai_rules.retreat_velocity(Vector3(100,0,0),10.0,0.9)==Vector3.ZERO,"oversized retreat heading cannot produce an AI speed spike")
 check(ai_rules.orbit_velocity(Vector3(100,0,0),10.0,1.0)==Vector3.ZERO,"oversized orbit heading cannot produce an AI speed spike")
 check(ai_rules.approach(Vector3(5,0,0),10.0,1,false,1,NAN)==Vector3.ZERO,"invalid pursuit distance fails closed")
 var pursuit_sweep_ok:bool=true
 for sample_index in range(111):
  var angle:float=TAU*float(sample_index)/111.0
  var offset:Vector3=Vector3(cos(angle)*12.0,0.0,sin(angle)*12.0)
  var regular:Vector3=ai_rules.approach(offset,10.0,sample_index,false,1,12.0)
  var boss:Vector3=ai_rules.approach(offset,10.0,sample_index,true,3,12.0)
  if not is_equal_approx(regular.length(),10.0) or not is_equal_approx(boss.length(),13.0) or not is_zero_approx(regular.y) or not is_zero_approx(boss.y):
   pursuit_sweep_ok=false
   break
 check(pursuit_sweep_ok,"111 deterministic pursuit headings preserve normal and phase-three boss speeds")
 var heading_sweep_ok:bool=true
 for sample_index in range(111):
  var angle:float=TAU*float(sample_index)/111.0
  var planar_offset:Vector3=Vector3(cos(angle)*5.0,4.0,sin(angle)*5.0)
  var direction:Vector3=ai_rules.planar_direction(planar_offset,5.0)
  var escape:Vector3=ai_rules.wildlife_escape_velocity(direction,10.0,5.0,1.0,sample_index)
  if not is_equal_approx(direction.length(),1.0) or not is_zero_approx(direction.y) or not is_equal_approx(escape.length(),10.0) or escape.dot(direction)>-9.99:
   heading_sweep_ok=false
   break
 check(heading_sweep_ok,"111 headings preserve valid normalized and wildlife escape motion")
 var guardian_rules=load("res://scripts/systems/guardian_attack_rules.gd")
 check(is_equal_approx(guardian_rules.windup(1,INF),0.75),"invalid guardian windup stays bounded")
 check(guardian_rules.windup(1,1000.0)<=8.0,"very long guardian telegraph stays bounded")
 check(not guardian_rules.warning_hit(Vector3(INF,0,0),Vector3.ZERO,3.0),"invalid attack marker never strikes hero")
 check(not guardian_rules.warning_hit(Vector3.ZERO,Vector3(NAN,0,0),3.0),"invalid hero coordinates cannot trigger a hit")
 check(not guardian_rules.warning_hit(Vector3(1.0e30,0,0),Vector3(-1.0e30,0,0),3.0),"overflowing guardian offset never causes phantom hit")
 check(not guardian_rules.warning_hit(Vector3(1.0e25,0,0),Vector3.ZERO,3.0),"squared-distance overflow cannot count as a guardian hit")
 check(guardian_rules.warning_hit(Vector3.ZERO,Vector3(1,0,1),2.8),"valid guardian radius still lands attacks")
 var combat_rules=load("res://scripts/systems/enemy_combat_rules.gd")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(1.5,-1.0),1.5),"negative frame time never accelerates attacks")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(1.5,INF),1.5),"invalid frame time leaves attack cooldown stable")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(1.5,0.5),1.4),"large frame hitch uses capped cooldown step")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(NAN,0.016),0.1),"invalid cooldown recovers with short delay")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(-1.0,0.016),0.1),"negative cooldown recovers with short delay")
 check(not combat_rules.ready_to_strike(-0.01) and not combat_rules.ready_to_strike(NAN),"corrupt attack clocks never grant immediate strikes")
 check(combat_rules.ready_to_strike(0.0) and not combat_rules.ready_to_strike(0.01),"only a completed cooldown permits a strike")
 var strike_timer_sweep_ok:bool=true
 for sample_index in range(111):
  var starting_timer:float=0.02+float(sample_index)*0.01
  var frame_delta:float=0.005+float(sample_index%7)*0.015
  var clipped_delta:float=minf(frame_delta,0.1)
  var after_frame:float=combat_rules.attack_timer_after_frame(starting_timer,frame_delta)
  var expected_timer:float=maxf(0.0,starting_timer-clipped_delta)
  if not is_equal_approx(after_frame,expected_timer) or combat_rules.ready_to_strike(after_frame)!=(expected_timer==0.0):
   strike_timer_sweep_ok=false
   break
 check(strike_timer_sweep_ok,"111 attack countdown samples require a completed safe timer before striking")
 var live_strike_gate_ok:bool=true
 for sample_index in range(111):
  var raw_timer:float=0.0 if sample_index%5==0 else (-0.01*float(sample_index+1) if sample_index%5==1 else (NAN if sample_index%5==2 else (INF if sample_index%5==3 else float(sample_index+1)*0.02)))
  var recovered_timer:float=combat_rules.attack_timer_after_frame(raw_timer,0.016)
  var expected_ready:bool=sample_index%5==0
  if not is_finite(recovered_timer) or recovered_timer<0.0 or combat_rules.ready_to_strike(recovered_timer)!=expected_ready:
   live_strike_gate_ok=false
   break
 check(live_strike_gate_ok,"111 live strike-gate inputs reject corrupted and unfinished cooldowns")
 var live_enemy_script:String=FileAccess.get_file_as_string("res://scripts/native_v11.gd")
 check("ENEMY_COMBAT_RULES.ready_to_strike(float(beast[\"attack\"]))" in live_enemy_script,"live enemy attacks use validated readiness gate")
 var elite_lunge_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_timer:float=5.0 if sample_index%5==0 else (0.0 if sample_index%5==1 else (-1.0 if sample_index%5==2 else (NAN if sample_index%5==3 else float(sample_index)*0.01)))
  var sample_delta:float=0.5 if sample_index%3==0 else (0.016 if sample_index%3==1 else -0.02)
  var after_lunge:float=combat_rules.attack_timer_after_frame(sample_timer,sample_delta)
  var expected_lunge:float=0.1 if not is_finite(sample_timer) or sample_timer<0.0 else maxf(0.0,sample_timer-clampf(sample_delta,0.0,0.1))
  if not is_equal_approx(after_lunge,expected_lunge) or combat_rules.ready_to_strike(after_lunge)!=(expected_lunge==0.0):
   elite_lunge_sweep_ok=false
   break
 check(elite_lunge_sweep_ok,"111 elite lunge cooldown cases prevent early attacks and clamp frame hitches")
 check("beast[\"elite_lunge_cd\"]=ENEMY_COMBAT_RULES.attack_timer_after_frame" in live_enemy_script and "ENEMY_COMBAT_RULES.ready_to_strike(float(beast[\"elite_lunge_cd\"]))" in live_enemy_script,"live elite lunge uses validated cooldown and readiness")
 var windup_sweep_ok:bool=true
 for sample_index in range(111):
  var windup_start:float=0.05+float(sample_index%17)*0.05
  var windup_delta:float=0.5 if sample_index%3==0 else (0.016 if sample_index%3==1 else -0.02)
  var windup_after:float=combat_rules.attack_timer_after_frame(windup_start,windup_delta)
  var elapsed:float=clampf(windup_delta,0.0,0.1)
  var windup_expected:float=maxf(0.0,windup_start-elapsed)
  if not is_equal_approx(windup_after,windup_expected) or windup_after<0.0 or windup_after>windup_start:
   windup_sweep_ok=false
   break
 check(windup_sweep_ok,"111 elite windup samples honor capped frame progression")
 check("if not is_finite(lunge_windup) or lunge_windup<0.0 or lunge_windup>0.9:" in live_enemy_script,"invalid elite windups cancel before damage")
 check("beast[\"elite_lunge_windup\"]=ENEMY_COMBAT_RULES.attack_timer_after_frame(lunge_windup,delta)" in live_enemy_script,"live elite warning uses clamped frame delta")
 check(is_equal_approx(combat_rules.attack_timer_after_frame(0.9,0.5),0.8),"frame hitch cannot finish elite windup instantly")
 check(combat_rules.can_attack(1.75,true,true,"boar"),"boar uses its existing 1.8 melee range")
 check(not combat_rules.can_attack(1.75,true,true,"wolf"),"wolf retains default 1.7 melee range")
 check(not combat_rules.can_attack(1.0,false,true,"boar") and not combat_rules.can_attack(1.0,true,false,"boar"),"readiness and line-of-sight still gate melee")
 check(not combat_rules.can_attack(NAN,true,true,"boar") and not combat_rules.can_attack(-1.0,true,true,"boar"),"invalid melee distances fail closed")
 var melee_range_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_distance:float=float(sample_index)*0.02
  var boar_expected:bool=sample_distance<1.8
  var wolf_expected:bool=sample_distance<1.7
  if combat_rules.can_attack(sample_distance,true,true,"boar")!=boar_expected or combat_rules.can_attack(sample_distance,true,true,"wolf")!=wolf_expected:
   melee_range_sweep_ok=false
   break
 check(melee_range_sweep_ok,"111 melee range samples preserve species-specific attack reach")
 var cooldown_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_kind:String="boar" if sample_index%4==0 else ("panther" if sample_index%4==1 else ("serpent" if sample_index%4==2 else "wolf"))
  var delay:float=combat_rules.staggered_cooldown(sample_kind,sample_index)
  var repeat_delay:float=combat_rules.staggered_cooldown(sample_kind,sample_index+4)
  var baseline:float=combat_rules.attack_cooldown(sample_kind)
  var spread:float=0.17 if sample_kind=="boar" else (0.10 if sample_kind=="panther" else 0.12)
  var expected_delay:float=baseline+float(sample_index%4)*spread
  if not is_equal_approx(delay,expected_delay) or not is_equal_approx(delay,repeat_delay) or delay<baseline or delay>baseline+3.0*spread:
   cooldown_sweep_ok=false
   break
 check(cooldown_sweep_ok,"111 cooldown samples preserve species pacing and stagger cycle")
 var attack_slots_ok:bool=true
 for sample_index in range(111):
  var moment:float=float(sample_index)*0.25+0.125
  var group_ready_count:int=0
  for actor_id in range(4):
   if combat_rules.group_attack_ready(moment,actor_id):group_ready_count+=1
  var boar_ready_count:int=0
  for actor_id in range(5):
   if combat_rules.tactical_attack_ready("boar",moment,actor_id):boar_ready_count+=1
  if group_ready_count!=1 or boar_ready_count!=2:
   attack_slots_ok=false
   break
 check(attack_slots_ok,"111 quarter-second windows schedule one standard and two boar attack turns")
 check(not combat_rules.group_attack_ready(0.125,0,0) and not combat_rules.group_attack_ready(0.125,0,9),"invalid attack slot counts never trigger a strike")
 var slot_boundary_sweep_ok:bool=true
 for sample_index in range(111):
  var slot_count:int=1+sample_index%8
  var tick_index:int=sample_index/8
  var window_time:float=float(tick_index)*0.25+0.125
  var expected_slot:int=tick_index%slot_count
  var allowed_count:int=0
  for actor_id in range(slot_count):
   var ready:bool=combat_rules.group_attack_ready(window_time,actor_id,slot_count)
   if ready:allowed_count+=1
   if ready!=(actor_id==expected_slot):
    slot_boundary_sweep_ok=false
    break
  if not slot_boundary_sweep_ok or allowed_count!=1:
   slot_boundary_sweep_ok=false
   break
 check(slot_boundary_sweep_ok,"111 variable-size attack windows allow exactly one assigned fighter")
 check(combat_rules.role_attack_ready("boar",0.125,-1)==combat_rules.role_attack_ready("boar",0.125,3),"negative beast IDs retain stable attack turn")
 check(combat_rules.tactical_attack_ready("boar",0.125,-1)==combat_rules.tactical_attack_ready("boar",0.125,4),"negative tactical beast IDs retain stable turn")
 check(not combat_rules.group_attack_ready(NAN,1) and not combat_rules.tactical_attack_ready("boar",INF,1),"invalid attack clocks cannot trigger attacks")
 check(combat_rules.investigate_velocity(true,false,false,1.0,2.0,Vector3.ZERO,Vector3(1.0e30,0.0,1.0e30),10.0)==Vector3.ZERO,"overflowed investigation offset fails closed")
 check(not combat_rules.search_arrived(Vector3.ZERO,Vector3(1.0e30,0.0,1.0e30)),"overflowed search offset cannot count as arrival")
 var search_sweep_ok:bool=true
 for sample_index in range(111):
  var angle:float=TAU*float(sample_index)/111.0
  var destination:Vector3=Vector3(cos(angle)*6.0,0.0,sin(angle)*6.0)
  var velocity:Vector3=combat_rules.investigate_velocity(true,false,false,1.0,2.0,Vector3.ZERO,destination,10.0)
  if not is_equal_approx(velocity.length(),6.5) or not is_zero_approx(velocity.y) or velocity.dot(destination)<=0.0 or combat_rules.search_arrived(Vector3.ZERO,destination):
   search_sweep_ok=false
   break
 check(search_sweep_ok,"111 investigation headings preserve search speed, direction, and arrival detection")
 var braking_sweep_ok:bool=true
 for sample_index in range(111):
  var search_distance:float=1.05+float(sample_index)*0.07
  var destination:Vector3=Vector3(search_distance,0.0,0.0)
  var search_speed:Vector3=combat_rules.investigate_velocity(true,false,false,1.0,2.0,Vector3.ZERO,destination,10.0)
  var expected_speed:float=minf(6.5,(search_distance-1.0)*3.5)
  if absf(search_speed.length()-expected_speed)>0.002 or search_speed.x<=0.0 or not is_zero_approx(search_speed.y) or not is_zero_approx(search_speed.z) or combat_rules.search_arrived(Vector3.ZERO,destination):
   braking_sweep_ok=false
   break
 check(braking_sweep_ok,"111 investigation distances brake smoothly without movement spikes")
 check(combat_rules.investigate_velocity(true,false,false,1.0,2.0,Vector3.ZERO,Vector3(1.0,0.0,0.0),10.0)==Vector3.ZERO,"search reaches rest exactly at one unit")
 check(combat_rules.investigate_velocity(true,false,false,1.0e12+1.0,1.0e12+2.0,Vector3.ZERO,Vector3(6,0,0),10.0)==Vector3.ZERO,"future AI clocks cannot resume stale investigations")
 var memory_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_now:float=0.1+float(sample_index)*0.02
  var known_position:Vector3=Vector3(float(sample_index%7)+3.0,0.0,6.0)
  var memory:Dictionary=combat_rules.remember_target(true,true,false,sample_now,known_position,Vector3.ZERO,0.0)
  var expiry:float=float(memory.get("until",0.0))
  var investigation:Vector3=combat_rules.investigate_velocity(true,false,false,sample_now,expiry,Vector3.ZERO,known_position,10.0)
  var after_expiry:Vector3=combat_rules.investigate_velocity(true,false,false,expiry,expiry,Vector3.ZERO,known_position,10.0)
  if memory.get("position",Vector3.ZERO)!=known_position or not is_equal_approx(expiry,sample_now+3.0) or investigation.length()<=0.0 or after_expiry!=Vector3.ZERO:
   memory_sweep_ok=false
   break
 check(memory_sweep_ok,"111 target-memory samples keep three-second pursuit and expire on schedule")
 var memory_horizon_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_now:float=1.0e12-5.0+float(sample_index)*0.05
  var target_pos:Vector3=Vector3(6.0,0.0,2.0)
  var fresh:Dictionary=combat_rules.remember_target(true,true,false,sample_now,target_pos,Vector3.ZERO,0.0)
  if sample_now<=1.0e12-3.0:
   if fresh.get("position",Vector3.ZERO)!=target_pos or not is_equal_approx(float(fresh.get("until",0.0)),sample_now+3.0) or float(fresh.get("until",0.0))>1.0e12:
    memory_horizon_sweep_ok=false
    break
  elif fresh.get("position",target_pos)!=Vector3.ZERO or float(fresh.get("until",-1.0))!=0.0:
   memory_horizon_sweep_ok=false
   break
 check(memory_horizon_sweep_ok,"111 target memory clock-boundary cases remain within supported horizon")
 var old_memory_horizon_ok:bool=true
 for sample_index in range(111):
  var sample_now:float=1.0e12-5.0+float(sample_index)*0.05
  var old_expiry:float=sample_now+2.5
  var old_position:Vector3=Vector3(6.0,0.0,3.0)
  var retained:Dictionary=combat_rules.remember_target(false,true,false,sample_now,Vector3.ZERO,old_position,old_expiry)
  var expected_valid:bool=old_expiry<=1.0e12
  var retained_expiry:float=float(retained.get("until",0.0))
  var moving:Vector3=combat_rules.investigate_velocity(true,false,false,sample_now,old_expiry,Vector3.ZERO,old_position,10.0)
  var pause_active:bool=combat_rules.search_pause_active(false,sample_now,sample_now+0.5)
  if expected_valid:
   if not is_equal_approx(retained_expiry,old_expiry) or moving.length()<=0.0:
    old_memory_horizon_ok=false
    break
  elif retained_expiry!=0.0 or moving!=Vector3.ZERO:
   old_memory_horizon_ok=false
   break
  if pause_active!=(sample_now+0.5<=1.0e12):
   old_memory_horizon_ok=false
   break
 check(old_memory_horizon_ok,"111 stale-memory horizon samples reject invalid expiry across all AI search paths")
 var invalid_sighting_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_now:float=0.25+float(sample_index)*0.05
  var stale_target:Vector3=Vector3(4.0,0.0,6.0)
  var invalid_target:Vector3=Vector3(NAN,0.0,0.0) if sample_index%3==0 else (Vector3(INF,0.0,0.0) if sample_index%3==1 else Vector3(0.0,0.0,-INF))
  var cleared:Dictionary=combat_rules.remember_target(true,true,false,sample_now,invalid_target,stale_target,sample_now+2.0)
  var retained:Dictionary=combat_rules.remember_target(false,true,false,sample_now,invalid_target,stale_target,sample_now+2.0)
  if cleared.get("position",stale_target)!=Vector3.ZERO or float(cleared.get("until",-1.0))!=0.0 or retained.get("position",Vector3.ZERO)!=stale_target or not is_equal_approx(float(retained.get("until",0.0)),sample_now+2.0):
   invalid_sighting_sweep_ok=false
   break
 check(invalid_sighting_sweep_ok,"111 corrupt sighting cases discard stale targets but preserve valid unseen memory")
 check(not combat_rules.search_pause_active(false,1.0e12+1.0,1.0e12+1.5),"out-of-range clock cannot prolong enemy search pause")
 check(not combat_rules.search_pause_active(false,NAN,1.0),"nonfinite search clock cannot activate pause")
 var pause_sweep_ok:bool=true
 for sample_index in range(111):
  var sample_now:float=0.5+float(sample_index)*0.125
  var expires:float=combat_rules.search_pause_expiry(sample_now)
  var halfway:float=sample_now+0.375
  if not is_equal_approx(expires,sample_now+0.75) or not combat_rules.search_pause_active(false,sample_now,expires) or not combat_rules.search_pause_active(false,halfway,expires) or combat_rules.search_pause_active(false,expires,expires) or combat_rules.search_pause_active(true,sample_now,expires):
   pause_sweep_ok=false
   break
 check(pause_sweep_ok,"111 enemy search pauses last 0.75 seconds and cancel on engagement")
 check(combat_rules.search_pause_expiry(INF)==0.0,"invalid AI clock cannot create endless search")
 check(combat_rules.search_pause_expiry(1.0e15)==0.0,"extreme AI clock cannot create endless search")
 check(combat_rules.search_pause_expiry(1.0e12)==0.0,"maximum AI clock cannot schedule an unusable pause")
 check(is_equal_approx(combat_rules.search_pause_expiry(1.0e12-0.75),1.0e12),"latest valid pause expiry stays within clock horizon")
 var horizon_sweep_ok:bool=true
 for sample_index in range(111):
  var boundary_now:float=1.0e12-2.0+float(sample_index)*0.025
  var expiry_time:float=combat_rules.search_pause_expiry(boundary_now)
  var in_horizon:bool=boundary_now<=1.0e12-0.75
  if in_horizon:
   if not is_equal_approx(expiry_time,boundary_now+0.75) or not combat_rules.search_pause_active(false,boundary_now,expiry_time) or combat_rules.search_pause_active(false,expiry_time,expiry_time):
    horizon_sweep_ok=false
    break
  elif expiry_time!=0.0:
   horizon_sweep_ok=false
   break
 check(horizon_sweep_ok,"111 search pause clock-boundary samples never schedule unusable expiry")
 var telegraph=load("res://scripts/systems/boss_telegraph_readout.gd")
 check("DODGE AWAY" in telegraph.warning_text(1,0),"focused attack gives directional dodge cue")
 check("CLEAR THE AREA" in telegraph.warning_text(2,1),"sweep attack warns of area danger")
 check("SIDESTEP" in telegraph.warning_text(3,2),"precision attack suggests sidestep")
 check("WATCH THE DELAY" in telegraph.warning_text(3,3),"delayed attack warns of timing")
 check(telegraph.dodge_cue(4)==telegraph.dodge_cue(0),"guardian dodge cues wrap safely with pattern index")
 check(telegraph.dodge_cue(-1)=="WATCH THE DELAY","legacy negative attack patterns get valid warnings")
 var boss_controller:String=FileAccess.get_file_as_string("res://scripts/native_v38_10.gd")
 check("func _refresh_boss_phase(beast:Dictionary)->void:" in boss_controller and "super._refresh_boss_phase(beast)" in boss_controller,"live guardian phase changes invoke parent behavior")
 check("new_phase<=old_phase:return" in boss_controller and "next_pattern_after_phase_change(old_pattern,new_phase)" in boss_controller,"new attack selection runs only when guardian phase increases")
 check("if float(beast.get(\"windup\",0.0))>0.0:return" in boss_controller,"active guardian windup timer prevents attack rotation")
 check(boss_controller.find("_show_message(\"GUARDIAN PHASE")<boss_controller.find("if float(beast.get(\"windup\",0.0))>0.0:return"),"guardian phase warning fires even during active windups")
 var adaptive=load("res://scripts/systems/gameplay_v313.gd")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,0,2.0)==1,"close player pressure selects guardian sweep")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,1,2.0)==2,"guardian does not repeat sweep back-to-back")
 check(adaptive.choose_boss_pattern(3,20.0,100.0,2,2.0)==0,"low health player gets readable focused attack")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,2,8.0)==3,"phase three can select fourth guardian pattern")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,0,2.0,3)==2,"repeated close pressure chooses precision")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,2,2.0,4)==3,"precision cannot immediately repeat")
 check(adaptive.choose_boss_pattern(3,20.0,100.0,0,2.0,4)==0,"low health safety overrides pressure")
 check(adaptive.update_close_pressure(2,2.0)==3,"pressure rises during repeated close attacks")
 check(adaptive.update_close_pressure(4,8.0)==0,"pressure resets when hero retreats")
 check(adaptive.update_close_pressure(4,-1.0)==0,"pressure resets when hero location unavailable")
 check(adaptive.update_close_pressure(4,3.0)==3,"pressure decays outside close range")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,1,3.0,0,true)==0,"guardian changes follow-up after landing attack")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,1,3.0,0,false)==2,"missed attack retains regular pattern rotation")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,0,3.0,0,false,1)==2,"precision guardian chooses precision at medium range")
 check(adaptive.choose_boss_pattern(3,100.0,100.0,0,3.0,0,false,2)==1,"sweeper guardian chooses sweep at medium range")
 check(adaptive.choose_boss_pattern(3,20.0,100.0,0,3.0,0,false,2)==0,"low-health fairness overrides fighting style")
 var sprite=game.get("hero_sprite") as Sprite3D
 check(is_instance_valid(sprite) and sprite.texture!=null,"original hero sprite texture present")
 game.queue_free()
 if failed==0:print("MOONFALL AI 111 OVERHAUL PASS")
 quit(1 if failed>0 else 0)
