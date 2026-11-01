extends RefCounted
## Pure targeting and damage rules for the realm companion.
static func target_priority(hero_distance_squared:float, follower_distance_squared:float, low_health:bool, guard_radius:float)->float:
 if low_health and hero_distance_squared<=guard_radius*guard_radius:
  return hero_distance_squared*2.0+follower_distance_squared-1000.0
 return hero_distance_squared*2.0+follower_distance_squared if low_health else follower_distance_squared

static func strike_damage(level:int, loyalty:int, boss:bool)->float:
 var normal:float=clampf(6.0+maxi(level,1)*0.65+clampi(loyalty,0,100)*0.05,6.0,38.0)
 return normal*0.55 if boss else normal

# Pure deterministic targeting guards used by companion AI.
static func can_engage(alive:bool,hostile:bool,alerted_until:float,now:float,hero_distance_squared:float,companion_distance_squared:float,leash:float,sight:bool,range_limit:float)->bool:
 return alive and hostile and sight and alerted_until>now and hero_distance_squared<=leash*leash and companion_distance_squared<range_limit*range_limit

static func cooldown_elapsed(current:float,delta:float)->float:
 return maxf(0.0,current-clampf(delta,0.0,0.1))

static func pursuit_step(speed:float,delta:float)->float:
 return maxf(0.0,speed)*clampf(delta,0.0,0.1)
