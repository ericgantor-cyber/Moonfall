extends RefCounted
## Pure targeting and damage rules for the realm companion.
static func target_priority(hero_distance_squared:float, follower_distance_squared:float, low_health:bool, guard_radius:float)->float:
 if not is_finite(hero_distance_squared) or not is_finite(follower_distance_squared):return 1.0e12
 var hd:float=maxf(0.0,hero_distance_squared)
 var fd:float=maxf(0.0,follower_distance_squared)
 var guard:float=maxf(0.0,guard_radius) if is_finite(guard_radius) else 0.0
 if low_health and hd<=guard*guard:
  return hd*2.0+fd-1000.0
 return hd*2.0+fd if low_health else fd

static func strike_damage(level:int, loyalty:int, boss:bool)->float:
 var normal:float=clampf(6.0+maxi(level,1)*0.65+clampi(loyalty,0,100)*0.05,6.0,38.0)
 return normal*0.55 if boss else normal

## Never target dead, passive, invisible, unreachable or corrupt-position enemies.
static func can_engage(alive:bool,hostile:bool,alerted_until:float,now:float,hero_distance_squared:float,companion_distance_squared:float,leash:float,sight:bool,range_limit:float)->bool:
 if not alive or not hostile or not sight:return false
 if not is_finite(now) or not is_finite(alerted_until) or alerted_until<=now:return false
 if not is_finite(hero_distance_squared) or not is_finite(companion_distance_squared):return false
 if not is_finite(leash) or not is_finite(range_limit):return false
 if leash<=0.0 or range_limit<=0.0:return false
 return hero_distance_squared>=0.0 and companion_distance_squared>=0.0 and hero_distance_squared<=leash*leash and companion_distance_squared<range_limit*range_limit

static func cooldown_elapsed(current:float,delta:float)->float:
 if not is_finite(current):return 0.1
 var elapsed:float=clampf(delta,0.0,0.1) if is_finite(delta) else 0.0
 return maxf(0.0,current-elapsed)

static func pursuit_step(speed:float,delta:float)->float:
 if not is_finite(speed) or not is_finite(delta):return 0.0
 return clampf(speed,0.0,30.0)*clampf(delta,0.0,0.1)
