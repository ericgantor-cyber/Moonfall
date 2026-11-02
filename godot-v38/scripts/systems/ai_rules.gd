extends RefCounted
## Stateless creature decision math; Godot nodes remain owned by the world.
static func perception(is_alert:bool,weather:String,is_night:bool) -> float:
 var radius:=14.0 if is_alert else 10.0
 if weather=="Mist":radius*=0.78
 if is_night:radius*=1.16
 return radius

static func approach(diff:Vector3,speed:float,spawn_id:int,is_boss:bool,boss_phase:int,distance:float) -> Vector3:
 if not is_finite(diff.x) or not is_finite(diff.y) or not is_finite(diff.z) or not is_finite(speed) or speed<=0.0:return Vector3.ZERO
 if diff.length_squared()<0.00001:return Vector3.ZERO
 var advance:=diff.normalized()
 if not is_boss and is_finite(distance) and distance>=0.0 and distance<8.0:
  var side:=1.0 if spawn_id%2==0 else -1.0
  # Three stable approach lanes break up crowded packs without path scans.
  var flank_weight:float=0.25+0.14*float(posmod(spawn_id,3))
  # At melee distance the sideways push fades, so wolves still close in
  # instead of endlessly circling the hero without a chance to strike.
  flank_weight*=clampf((distance-1.4)/3.0,0.0,1.0)
  advance=(advance+Vector3(-advance.z,0.0,advance.x)*side*flank_weight).normalized()
 if is_boss and boss_phase>=2:speed*=1.0+0.15*float(clampi(boss_phase,1,3)-1)
 return advance*speed

static func retreat_due(hp:float,max_health:float,is_boss:bool,dungeon_site:int,now:float,cooldown_until:float) -> bool:
 return is_finite(hp) and is_finite(max_health) and max_health>0.0 and is_finite(now) and is_finite(cooldown_until) and hp>0.0 and hp<max_health*0.28 and not is_boss and dungeon_site<0 and now>=cooldown_until
