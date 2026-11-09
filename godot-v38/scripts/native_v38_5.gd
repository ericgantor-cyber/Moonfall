extends "res://scripts/native_v38_4.gd"
## Companion attacks use existing beast defeat and loot pipelines; no duplicate loot or modes.
const COMPANION_RANGE:float=10.0
const COMPANION_STRIKE_RANGE:float=2.8
const COMPANION_ATTACK_INTERVAL:float=1.35
const COMPANION_GUARD_RADIUS:float=5.5
const COMPANION_LEASH_RADIUS:float=12.0
const COMPANION_LOW_HEALTH:float=0.35
var realm_companion_attack_timer:float=0.0
const COMPANION_SCAN_INTERVAL:float=0.20
var companion_scan_timer:float=0.0
var companion_cached_target:Dictionary={}

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing or death_active or menu_open or not is_instance_valid(player):
  companion_cached_target.clear()
  companion_scan_timer=0.0
  return
 if not is_instance_valid(realm_follower) or not realm_follower.visible:
  companion_cached_target.clear()
  companion_scan_timer=0.0
  return
 realm_companion_attack_timer=maxf(0.0,realm_companion_attack_timer-delta)
 if region_index<0 or region_index>=zone_beast_lists.size():
  companion_cached_target.clear()
  return
 companion_scan_timer=maxf(0.0,companion_scan_timer-delta)
 if companion_scan_timer<=0.0 or not _companion_target_valid(companion_cached_target):
  companion_scan_timer=COMPANION_SCAN_INTERVAL
  companion_cached_target=_companion_find_target()
 var chosen:Dictionary=companion_cached_target
 if chosen.is_empty():return
 var enemy_body:CharacterBody3D=chosen.get("body") as CharacterBody3D
 if not is_instance_valid(enemy_body):return
 var distance:float=realm_follower.global_position.distance_squared_to(enemy_body.global_position)
 # Movement stays smooth between scans; attack speed remains cooldown-limited.
 if distance>COMPANION_STRIKE_RANGE*COMPANION_STRIKE_RANGE:
  var destination:Vector3=enemy_body.global_position+Vector3(0.0,1.0,0.0)
  realm_follower.global_position=realm_follower.global_position.move_toward(destination,FOLLOW_SPEED*minf(delta,0.1))
  return
 if realm_companion_attack_timer>0.0:return
 if not _companion_target_valid(chosen):
  companion_cached_target.clear()
  return
 realm_companion_attack_timer=COMPANION_ATTACK_INTERVAL
 var loyalty:int=REALM_ETHICS.companion_loyalty(REALM_ETHICS.normalize(realm_ethics),realm_companion_ethic)
 var damage:float=clampf(6.0+hero_level*0.65+loyalty*0.05,6.0,38.0)
 chosen["hp"]=float(chosen["hp"])-damage
 chosen["alert_until"]=map_time+5.0
 _show_floating_damage(chosen,int(round(damage)),false)
 if float(chosen["hp"])<=0.0:
  _defeat_beast(chosen,true)
  companion_cached_target.clear()

func _companion_target_valid(beast:Dictionary)->bool:
 if beast.is_empty() or not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):return false
 if float(beast.get("alert_until",0.0))<=map_time:return false
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 if not is_instance_valid(body) or not is_instance_valid(player) or not is_instance_valid(realm_follower):return false
 if realm_follower.global_position.distance_squared_to(body.global_position)>=COMPANION_RANGE*COMPANION_RANGE:return false
 if player.global_position.distance_squared_to(body.global_position)>COMPANION_LEASH_RADIUS*COMPANION_LEASH_RADIUS:return false
 return _has_clear_sight(player,body)

func _companion_find_target()->Dictionary:
 var chosen:Dictionary={}
 var low_health:bool=max_hp>0.0 and hp/max_hp<=COMPANION_LOW_HEALTH
 var best_priority:float=INF
 for beast in zone_beast_lists[region_index]:
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body") as CharacterBody3D
  if not is_instance_valid(body):continue
  var distance:float=realm_follower.global_position.distance_squared_to(body.global_position)
  if distance>=COMPANION_RANGE*COMPANION_RANGE:continue
  var hero_distance:float=player.global_position.distance_squared_to(body.global_position)
  if hero_distance>COMPANION_LEASH_RADIUS*COMPANION_LEASH_RADIUS:continue
  if float(beast.get("alert_until",0.0))<=map_time:continue
  if not _has_clear_sight(player,body):continue
  var guarding:bool=hero_distance<=COMPANION_GUARD_RADIUS*COMPANION_GUARD_RADIUS
  var priority:float=hero_distance*2.0+distance if low_health else distance
  if low_health and guarding:priority-=1000.0
  if priority>=best_priority:continue
  best_priority=priority
  chosen=beast
 return chosen
