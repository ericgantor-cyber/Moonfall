extends "res://scripts/native_v32_4.gd"
## v32.5: bounded flanking blended into existing terrain steering.
## Does not override attack logic, change collision or modify character art.
const V325_FLANK_RANGE_MIN:float=3.0
const V325_FLANK_RANGE_MAX:float=11.0
const V325_FLANK_STRENGTH:float=0.23
var v325_flank_steps:int=0
var v325_flank_skips:int=0

func _release_label()->String:return "v32.5"

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var before:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.size()<=before:return
 var creature:Dictionary=beasts[-1]
 var actor:CharacterBody3D=creature.get("body")
 if is_instance_valid(actor):actor.set_meta("v325_creature",creature)

func _terrain_grid_steer(body:CharacterBody3D,desired:Vector3)->Vector3:
 # The parent navigation hook remains authoritative, including its wall avoidance.
 if not hero_playing or not is_instance_valid(player) or not body.has_meta("v325_creature"):
  return super._terrain_grid_steer(body,desired)
 var beast:Dictionary=body.get_meta("v325_creature")
 if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)) or bool(beast.get("boss",false)) or not bool(beast.get("engaged",false)) or map_time<float(beast.get("stagger_until",0.0)) or float(beast.get("windup",0.0))>0.0 or float(beast.get("elite_lunge_windup",0.0))>0.0 or map_time<float(beast.get("retreat_until",0.0)):
  v325_flank_skips+=1
  return super._terrain_grid_steer(body,desired)
 var toward:Vector3=player.global_position-body.global_position
 toward.y=0.0
 var distance:float=toward.length()
 if distance<V325_FLANK_RANGE_MIN or distance>V325_FLANK_RANGE_MAX or desired.length_squared()<0.04:
  return super._terrain_grid_steer(body,desired)
 # Stable, per-enemy direction: prevents all mobs choosing the same side.
 var side:float=float(beast.get("v329_side",1 if int(beast.get("boss_id",0))%2==0 else -1))
 var perpendicular:Vector3=Vector3(-toward.z,0.0,toward.x).normalized()*side
 var speed:float=desired.length()
 var mixed:Vector3=(desired.normalized()*(1.0-V325_FLANK_STRENGTH)+perpendicular*V325_FLANK_STRENGTH).normalized()*speed
 v325_flank_steps+=1
 return super._terrain_grid_steer(body,mixed)

func _perf_report()->Dictionary:
 var stats:Dictionary=super._perf_report()
 stats["version"]="v32.5"
 stats["flank_steering_steps"]=v325_flank_steps
 stats["flank_excluded_steps"]=v325_flank_skips
 return stats
