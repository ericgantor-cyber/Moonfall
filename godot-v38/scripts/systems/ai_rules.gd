extends RefCounted
## Stateless creature decision math; Godot nodes remain owned by the world.
static func perception(is_alert:bool,weather:String,is_night:bool) -> float:
 var radius:=14.0 if is_alert else 10.0
 if weather=="Mist":radius*=0.78
 if is_night:radius*=1.16
 return radius

# Conservative upper bound for every weather/day/alert combination.
# Use the same coefficients as perception() to prevent awareness drift.
static func maximum_perception() -> float:
 return perception(true,"Clear",true)

# Cheap distance gate before any time-of-day or line-of-sight work.
static func potentially_visible(distance:float) -> bool:
 return is_finite(distance) and distance>=0.0 and distance<maximum_perception()

# Squared-distance variant for callers that already have a displacement vector.
# Avoids a square root when deciding whether to evaluate weather or line of sight.
static func potentially_visible_squared(distance_squared:float) -> bool:
 var max_radius:float=maximum_perception()
 return is_finite(distance_squared) and distance_squared>=0.0 and distance_squared<max_radius*max_radius

# Shared horizontal direction for retreat, orbiting and wildlife escape.
# Return zero for coincident or invalid positions to avoid invalid velocities.
static func planar_direction(diff:Vector3,distance:float) -> Vector3:
 if not is_finite(distance) or distance<=0.00001:return Vector3.ZERO
 if not is_finite(diff.x) or not is_finite(diff.z):return Vector3.ZERO
 var heading:Vector3=Vector3(diff.x/distance,0.0,diff.z/distance)
 if not is_finite(heading.x) or not is_finite(heading.z):return Vector3.ZERO
 return heading

# Retreat uses a pre-normalized horizontal direction from the shared rules.
# Multipliers 0.90 (timed retreat) and 0.85 (low-health retreat) preserve pacing.
static func retreat_velocity(player_direction:Vector3,speed:float,multiplier:float) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(multiplier) or multiplier<=0.0 or multiplier>1.0:return Vector3.ZERO
 if not is_finite(player_direction.x) or not is_finite(player_direction.z):return Vector3.ZERO
 return -player_direction*speed*multiplier

# Deterministic combat orbit. The tangent and approach direction are
# perpendicular unit vectors, so the 1:0.25 blend has fixed length.
static func orbit_velocity(player_direction:Vector3,speed:float,orbit_side:float) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(orbit_side) or absf(orbit_side)!=1.0:return Vector3.ZERO
 if not is_finite(player_direction.x) or not is_finite(player_direction.z):return Vector3.ZERO
 var flank:Vector3=Vector3(-player_direction.z,0.0,player_direction.x)
 return (flank*orbit_side+player_direction*0.25)*(speed*0.7/sqrt(1.0625))

# Wildlife movement remains purely planar. Frightened animals can wander away
# after the hero exits immediate fleeing range, using their stable spawn ID.
static func wildlife_escape_velocity(player_direction:Vector3,speed:float,distance:float,now:float,spawn_id:int) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(distance) or distance<0.0 or not is_finite(now) or absf(now)>1.0e12:return Vector3.ZERO
 if not is_finite(player_direction.x) or not is_finite(player_direction.z):return Vector3.ZERO
 var escape:Vector3=-player_direction
 if distance>8.0:escape=Vector3(cos(now+float(spawn_id)),0.0,sin(now+float(spawn_id)))
 return escape*speed

# Keep wildlife hunting and home-return speeds identical to the old world loop.
static func prey_chase_velocity(offset:Vector3,speed:float) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(offset.x) or not is_finite(offset.z):return Vector3.ZERO
 var planar:Vector3=Vector3(offset.x,0.0,offset.z)
 var length_sq:float=planar.length_squared()
 if not is_finite(length_sq) or length_sq<0.00001:return Vector3.ZERO
 return planar.normalized()*speed*0.8

static func return_home_velocity(offset:Vector3,speed:float) -> Vector3:
 if not is_finite(speed) or speed<=0.0 or not is_finite(offset.x) or not is_finite(offset.z):return Vector3.ZERO
 var planar:Vector3=Vector3(offset.x,0.0,offset.z)
 var length_sq:float=planar.length_squared()
 if not is_finite(length_sq) or length_sq<0.00001:return Vector3.ZERO
 return planar.normalized()*speed*0.4

static func approach(diff:Vector3,speed:float,spawn_id:int,is_boss:bool,boss_phase:int,distance:float,role:String="ambusher") -> Vector3:
 if not is_finite(diff.x) or not is_finite(diff.y) or not is_finite(diff.z) or not is_finite(speed) or speed<=0.0:return Vector3.ZERO
 var planar:Vector3=Vector3(diff.x,0.0,diff.z)
 var length_sq:float=planar.length_squared()
 if not is_finite(length_sq) or length_sq<0.00001:return Vector3.ZERO
 var advance:Vector3=planar.normalized()
 if not is_boss and role!="passive" and is_finite(distance) and distance>=0.0 and distance<8.0:
  var side:float=1.0 if posmod(spawn_id,2)==0 else -1.0
  # Ambushers use broad flanks, bruisers close more directly.
  var role_weight:float=1.0 if role=="ambusher" else 0.35
  var flank_weight:float=(0.25+0.14*float(posmod(spawn_id,3)))*role_weight
  flank_weight*=clampf((distance-1.4)/3.0,0.0,1.0)
  advance=(advance+Vector3(-advance.z,0.0,advance.x)*side*flank_weight).normalized()
 if is_boss and boss_phase>=2:speed*=1.0+0.15*float(clampi(boss_phase,1,3)-1)
 return advance*minf(speed,20.0)

static func retreat_due(hp:float,max_health:float,is_boss:bool,dungeon_site:int,now:float,cooldown_until:float,role:String="ambusher") -> bool:
 if not is_finite(hp) or not is_finite(max_health) or max_health<=0.0:return false
 if not is_finite(now) or not is_finite(cooldown_until) or hp<=0.0:return false
 if is_boss or dungeon_site>=0 or now<cooldown_until:return false
 var fraction:float=hp/max_health
 return is_finite(fraction) and fraction<(0.34 if role=="ambusher" else 0.20)
