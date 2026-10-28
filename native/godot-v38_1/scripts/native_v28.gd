extends "res://scripts/native_v27.gd"
## v28: preserve per-frame combat and ecology, throttle only motionless distant idle wildlife.
const AI_BUDGET:=preload("res://scripts/systems/ai_budget.gd")
var v28_frame:int=0
var v28_idle_skips:int=0
var v28_ai_live_steps:int=0

func _release_label() -> String:
 return "v28"

func _v28_is_safe_to_throttle(beast:Dictionary) -> bool:
 if not bool(beast.get("alive",false)):return false
 var body:CharacterBody3D=beast.get("body")
 if not is_instance_valid(body) or not is_instance_valid(player):return false
 var now:float=map_time
 var home:Vector3=beast.get("home",body.global_position)
 var activity:Dictionary=beast.get("ecology_target",{})
 return AI_BUDGET.may_throttle(
  body.global_position.distance_squared_to(player.global_position),
  body.velocity.length_squared(),
  body.global_position.distance_squared_to(home),
  bool(beast.get("hostile",false)),
  bool(beast.get("boss",false)),
  int(beast.get("dungeon_site",-1)),
  bool(beast.get("engaged",false)),
  now<float(beast.get("alert_until",0.0)),
  now<float(beast.get("stagger_until",0.0)),
  float(beast.get("windup",0.0))>0.0 or float(beast.get("elite_lunge_windup",0.0))>0.0,
  (not activity.is_empty() and now<float(beast.get("hunting_until",0.0))),
  now<float(beast.get("scared_until",0.0)),
  now<float(beast.get("retreat_until",0.0)))

func _update_beasts(delta:float) -> void:
 _ecology_update(delta)
 v28_frame+=1
 for beast in zone_beast_lists[region_index]:
  if _v28_is_safe_to_throttle(beast) and not AI_BUDGET.should_tick(v28_frame,int(beast.get("boss_id",0))):
   beast["v28_sleep_dt"]=minf(0.25,float(beast.get("v28_sleep_dt",0.0))+delta)
   v28_idle_skips+=1
   continue
  var accumulated:float=float(beast.get("v28_sleep_dt",0.0))
  if accumulated>0.0:beast["v28_sleep_dt"]=0.0
  # Sleeping actors are stationary; their timers still advance by total elapsed time.
  _update_beast_step(beast,delta+accumulated)
  v28_ai_live_steps+=1

func _reset_new_adventure() -> void:
 super._reset_new_adventure()
 v28_frame=0
 v28_idle_skips=0
 v28_ai_live_steps=0

func _despawn_native_beast(beast:Dictionary,delay:float) -> void:
 beast.erase("v28_sleep_dt")
 super._despawn_native_beast(beast,delay)

func _reset_native_beast(beast:Dictionary) -> void:
 super._reset_native_beast(beast)
 beast.erase("v28_sleep_dt")
