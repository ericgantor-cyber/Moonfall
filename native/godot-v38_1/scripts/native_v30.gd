extends "res://scripts/native_v29.gd"
## v30: distribute non-critical idle AI updates; active encounters run every tick.
## Retains v29 gameplay math, Godot collision physics and schema-2 checkpoints.
const V30_AI_BUDGET:=preload("res://scripts/systems/ai_budget.gd")
const V30_LOD_INTERVAL:float=0.45
var v30_idle_phases:Array[int]=[0,0,0,0]
var v30_last_ai_usec:int=0
var v30_worst_ai_usec:int=0
var v30_perf_samples:int=0
var v30_perf_total_usec:int=0
var v30_perf_clock:float=0.0

func _release_label()->String:
 return "v30"

func _update_beasts(delta:float)->void:
 # Important: ecology stays on its original update schedule.
 _ecology_update(delta)
 v28_frame+=1
 v29_frame_updates=0
 v29_frame_skips=0
 if not is_instance_valid(player):return
 var player_at:Vector3=player.global_position
 var now:float=map_time
 var current_beasts:Array=zone_beast_lists[region_index]
 # Spread idle ticks by stable encounter slot, not the usually-zero boss_id.
 # This avoids a synchronized 4-frame CPU spike on passive wildlife.
 var began:int=Time.get_ticks_usec()
 for index in range(current_beasts.size()):
  var beast:Dictionary=current_beasts[index]
  if not bool(beast.get("alive",false)):continue
  var body:CharacterBody3D=beast.get("body")
  if not is_instance_valid(body):continue
  var at:Vector3=body.global_position
  var home:Vector3=beast.get("home",at)
  var activity:Dictionary=beast.get("ecology_target",{})
  var idle:bool=V30_AI_BUDGET.may_throttle(
   at.distance_squared_to(player_at),body.velocity.length_squared(),
   at.distance_squared_to(home),bool(beast.get("hostile",false)),
   bool(beast.get("boss",false)),int(beast.get("dungeon_site",-1)),
   bool(beast.get("engaged",false)),now<float(beast.get("alert_until",0.0)),
   now<float(beast.get("stagger_until",0.0)),
   float(beast.get("windup",0.0))>0.0 or float(beast.get("elite_lunge_windup",0.0))>0.0,
   not activity.is_empty() and now<float(beast.get("hunting_until",0.0)),
   now<float(beast.get("scared_until",0.0)),now<float(beast.get("retreat_until",0.0)))
  if idle and not V30_AI_BUDGET.should_tick(v28_frame,index):
   beast["v28_sleep_dt"]=minf(0.25,float(beast.get("v28_sleep_dt",0.0))+delta)
   v28_idle_skips+=1
   v29_frame_skips+=1
   continue
  if idle:v30_idle_phases[posmod(index,4)]+=1
  var asleep:float=float(beast.get("v28_sleep_dt",0.0))
  if asleep>0.0:beast["v28_sleep_dt"]=0.0
  # Combat and fleeing actors are always updated immediately.
  _update_beast_step(beast,delta+asleep)
  v28_ai_live_steps+=1
  v29_frame_updates+=1
 var elapsed:int=Time.get_ticks_usec()-began
 v30_last_ai_usec=elapsed
 v30_worst_ai_usec=maxi(v30_worst_ai_usec,elapsed)
 v30_perf_samples+=1
 v30_perf_total_usec+=elapsed
 # Keep inherited profiling consumers working.
 if posmod(v28_frame,SAMPLE_EVERY)==0:
  v29_ai_samples+=1
  v29_ai_total_usec+=elapsed
  v29_ai_peak_usec=maxi(v29_ai_peak_usec,elapsed)

func v30_diagnostics()->Dictionary:
 return {"sampled_frames":v30_perf_samples,
  "average_ai_usec":float(v30_perf_total_usec)/float(maxi(1,v30_perf_samples)),
  "worst_ai_usec":v30_worst_ai_usec,
  "idle_updates_by_phase":v30_idle_phases.duplicate(),
  "idle_skips":v28_idle_skips,"active_updates":v28_ai_live_steps}

func _unhandled_input(event:InputEvent)->void:
 # Keyboard 0 or pressing the controller right stick restores a safe camera scale.
 if hero_playing and not menu_open:
  if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_0:
   _v24_set_zoom(19.0)
   get_viewport().set_input_as_handled()
   return
  if event is InputEventJoypadButton and event.pressed and event.button_index==JOY_BUTTON_RIGHT_STICK:
   _v24_set_zoom(19.0)
   get_viewport().set_input_as_handled()
   return
 super._unhandled_input(event)

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v30_idle_phases=[0,0,0,0]
 v30_last_ai_usec=0
 v30_worst_ai_usec=0
 v30_perf_samples=0
 v30_perf_total_usec=0
