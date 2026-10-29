extends "res://scripts/native_v32_8.gd"
## v32.9: bounded pack-side coordination. Reuses v32.5 steering and existing
## creature metadata, collision and terrain navigation. No save format changes.
const V329_SIDE_REFRESH:float=0.75
const V329_NEIGHBOR_RADIUS_SQ:float=49.0
const V329_MAX_NEIGHBORS:int=5
const V329_MAX_CANDIDATES:int=32
var v329_side_refreshes:int=0
var v329_neighbor_checks:int=0

func _release_label()->String:return "v32.9"

func _terrain_grid_steer(body:CharacterBody3D,desired:Vector3)->Vector3:
 if not is_instance_valid(body) or not body.has_meta("v325_creature") or not is_instance_valid(player):
  return super._terrain_grid_steer(body,desired)
 var creature:Dictionary=body.get_meta("v325_creature")
 if not hero_playing or not bool(creature.get("alive",false)) or not bool(creature.get("hostile",false)) or bool(creature.get("boss",false)) or not bool(creature.get("engaged",false)):
  return super._terrain_grid_steer(body,desired)
 if map_time>=float(creature.get("v329_refresh_at",0.0)):
  creature["v329_refresh_at"]=map_time+V329_SIDE_REFRESH
  var base_side:int=1 if int(creature.get("boss_id",0))%2==0 else -1
  var left_count:int=0
  var right_count:int=0
  var observed:int=0
  var inspected:int=0
  var zone:int=int(creature.get("zone",-1))
  if zone>=0 and zone<zone_beast_lists.size():
   for neighbor in zone_beast_lists[zone]:
    if inspected>=V329_MAX_CANDIDATES:break
    inspected+=1
    if neighbor==creature or not bool(neighbor.get("alive",false)) or not bool(neighbor.get("engaged",false)) or bool(neighbor.get("boss",false)):continue
    var actor:CharacterBody3D=neighbor.get("body")
    if not is_instance_valid(actor) or body.global_position.distance_squared_to(actor.global_position)>V329_NEIGHBOR_RADIUS_SQ:continue
    if int(neighbor.get("v329_side",1 if int(neighbor.get("boss_id",0))%2==0 else -1))>0:left_count+=1
    else:right_count+=1
    observed+=1
    if observed>=V329_MAX_NEIGHBORS:break
  v329_neighbor_checks+=inspected
  creature["v329_side"]=-1 if left_count>right_count else 1 if right_count>left_count else base_side
  v329_side_refreshes+=1
 # v32.5 remains authoritative for windups, retreat, terrain and collision.
 return super._terrain_grid_steer(body,desired)

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.9"
 report["pack_side_refreshes"]=v329_side_refreshes
 report["pack_neighbor_checks"]=v329_neighbor_checks
 return report
