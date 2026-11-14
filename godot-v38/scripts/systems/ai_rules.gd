extends RefCounted
## Stateless creature decision math; Godot nodes remain owned by the world.
static func perception(is_alert:bool,weather:String,is_night:bool) -> float:
 var radius:=14.0 if is_alert else 10.0
 if weather=="Mist":radius*=0.78
 if is_night:radius*=1.16
 return radius

static func approach(diff:Vector3,speed:float,spawn_id:int,is_boss:bool,boss_phase:int,distance:float) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(distance) or distance<0.0:return Vector3.ZERO
 if not is_finite(diff.x) or not is_finite(diff.y) or not is_finite(diff.z):return Vector3.ZERO
 # Avoid overflow in squared length and speed scaling before normalization.
 if absf(diff.x)>1.0e12 or absf(diff.y)>1.0e12 or absf(diff.z)>1.0e12:return Vector3.ZERO
 if speed>1.0e6:return Vector3.ZERO
 var length_sq:float=diff.length_squared()
 if not is_finite(length_sq) or length_sq<0.00001:return Vector3.ZERO
 var advance:=diff.normalized()
 if not is_boss and distance<8.0:
  var side:=1.0 if spawn_id%2==0 else -1.0
  # Near contact, orbit instead of pushing through the hero every frame.
  # Keep a small forward component so enemies still pursue moving targets.
  var lateral_weight:=1.8 if distance<2.0 else 0.42
  advance=(advance+Vector3(-advance.z,0.0,advance.x)*side*lateral_weight).normalized()
 if is_boss and boss_phase>=2:speed*=1.0+0.15*float(mini(boss_phase-1,12))
 return advance*speed

static func retreat_due(hp:float,max_health:float,is_boss:bool,dungeon_site:int,now:float,cooldown_until:float) -> bool:
 # Invalid health or world clocks must never trigger corrupted retreat behavior.
 if not is_finite(hp) or not is_finite(max_health) or not is_finite(now) or not is_finite(cooldown_until):return false
 if max_health<=0.0 or hp<=0.0 or hp>max_health:return false
 if now<0.0 or cooldown_until<0.0:return false
 return hp<max_health*0.28 and not is_boss and dungeon_site<0 and now>=cooldown_until
