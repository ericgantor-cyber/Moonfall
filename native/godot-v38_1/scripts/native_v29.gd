extends "res://scripts/native_v28.gd"
## v29: reduce repeated AI queries, distribute idle work, and sample live update cost.
## This does not modify combat damage, enemy reach, or the saved-game schema.
const SAMPLE_EVERY:int=8
var v29_ai_samples:int=0
var v29_ai_total_usec:int=0
var v29_ai_peak_usec:int=0
var v29_frame_updates:int=0
var v29_frame_skips:int=0

func _release_label() -> String:
 return "v29"

func _update_beasts(delta:float) -> void:
 # Profiling only one frame in eight minimizes overhead in the real game.
 var profiling:bool=(v28_frame%SAMPLE_EVERY)==0
 var started:int=Time.get_ticks_usec() if profiling else 0
 _ecology_update(delta)
 v28_frame+=1
 v29_frame_updates=0
 v29_frame_skips=0
 if not is_instance_valid(player):return
 var player_at:Vector3=player.global_position
 var now:float=map_time
 var current_beasts:Array=zone_beast_lists[region_index]
 for beast in current_beasts:
  # Do not spend movement/animation work on already despawned actors.
  if not bool(beast.get("alive",false)):continue
  var body:CharacterBody3D=beast.get("body")
  if not is_instance_valid(body):continue
  # Use the same safety predicates as v28, but cache the frame's player position.
  var at:Vector3=body.global_position
  var home:Vector3=beast.get("home",at)
  var activity:Dictionary=beast.get("ecology_target",{})
  var idle:bool=AI_BUDGET.may_throttle(
   at.distance_squared_to(player_at),body.velocity.length_squared(),
   at.distance_squared_to(home),bool(beast.get("hostile",false)),
   bool(beast.get("boss",false)),int(beast.get("dungeon_site",-1)),
   bool(beast.get("engaged",false)),now<float(beast.get("alert_until",0.0)),
   now<float(beast.get("stagger_until",0.0)),
   float(beast.get("windup",0.0))>0.0 or float(beast.get("elite_lunge_windup",0.0))>0.0,
   not activity.is_empty() and now<float(beast.get("hunting_until",0.0)),
   now<float(beast.get("scared_until",0.0)),now<float(beast.get("retreat_until",0.0)))
  if idle and not AI_BUDGET.should_tick(v28_frame,int(beast.get("boss_id",0))):
   beast["v28_sleep_dt"]=minf(0.25,float(beast.get("v28_sleep_dt",0.0))+delta)
   v28_idle_skips+=1
   v29_frame_skips+=1
   continue
  var asleep:float=float(beast.get("v28_sleep_dt",0.0))
  if asleep>0.0:beast["v28_sleep_dt"]=0.0
  _update_beast_step(beast,delta+asleep)
  v28_ai_live_steps+=1
  v29_frame_updates+=1
 if profiling:
  var elapsed:int=Time.get_ticks_usec()-started
  v29_ai_samples+=1
  v29_ai_total_usec+=elapsed
  v29_ai_peak_usec=maxi(v29_ai_peak_usec,elapsed)

func _combat_threat() -> bool:
 # Same exact radius and line-of-sight logic, skipping sqrt for most actors.
 if not is_instance_valid(player):return false
 var player_at:Vector3=player.global_position
 var radius_sq:float=BEAST_THREAT_RANGE*BEAST_THREAT_RANGE
 for enemy in zone_beast_lists[region_index]:
  if not bool(enemy.get("alive",false)) or not bool(enemy.get("hostile",false)):continue
  var body:CharacterBody3D=enemy.get("body")
  if not is_instance_valid(body):continue
  if body.global_position.distance_squared_to(player_at)>radius_sq:continue
  if _has_clear_sight(body,player):return true
 return false

func _reset_new_adventure() -> void:
 super._reset_new_adventure()
 v29_ai_samples=0
 v29_ai_total_usec=0
 v29_ai_peak_usec=0
 v29_frame_updates=0
 v29_frame_skips=0

func v29_ai_profile() -> Dictionary:
 return {"sampled_frames":v29_ai_samples,"average_usec":float(v29_ai_total_usec)/maxi(1,v29_ai_samples),"peak_usec":v29_ai_peak_usec,"live_steps":v28_ai_live_steps,"idle_skips":v28_idle_skips}
