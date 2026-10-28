extends RefCounted
## Stateless creature decision math; Godot nodes remain owned by the world.
static func perception(is_alert:bool,weather:String,is_night:bool) -> float:
 var radius:=14.0 if is_alert else 10.0
 if weather=="Mist":radius*=0.78
 if is_night:radius*=1.16
 return radius

static func approach(diff:Vector3,speed:float,spawn_id:int,is_boss:bool,boss_phase:int,distance:float) -> Vector3:
 if diff.length_squared()<0.00001:return Vector3.ZERO
 var advance:=diff.normalized()
 if not is_boss and distance<8.0:
  var side:=1.0 if spawn_id%2==0 else -1.0
  advance=(advance+Vector3(-advance.z,0.0,advance.x)*side*0.42).normalized()
 if is_boss and boss_phase>=2:speed*=1.0+0.15*float(boss_phase-1)
 return advance*speed

static func retreat_due(hp:float,max_health:float,is_boss:bool,dungeon_site:int,now:float,cooldown_until:float) -> bool:
 return hp>0.0 and hp<max_health*0.28 and not is_boss and dungeon_site<0 and now>=cooldown_until
