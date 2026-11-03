extends RefCounted
## Lightweight, deterministic role and attack pacing rules for existing fauna.
static func role_for(kind:String)->String:
 match kind:
  "panther":return "ambusher"
  "wolf":return "ambusher"
  "serpent":return "ambusher"
  "boar":return "bruiser"
  _:return "bruiser"

static func attack_cooldown(kind:String)->float:
 match kind:
  "panther":return 1.3
  "wolf":return 1.5
  "serpent":return 1.6
  "boar":return 1.9
  _:return 1.5

# Stable timing offsets reduce synchronized swarm strikes without scanning peers.
# Bosses retain their independent telegraphed combat cadence.
static func staggered_cooldown(kind:String,beast_id:int)->float:
 return attack_cooldown(kind)+float(posmod(beast_id,4))*(0.17 if kind=="boar" else (0.10 if kind=="panther" else 0.12))

# Fixed per-enemy attack slots make a pack less likely to strike at once.
# This only delays a ready attack; it never skips line-of-sight or dodge checks.
static func group_attack_ready(now:float,beast_id:int,slots:int=4)->bool:
 if not is_finite(now) or now<0.0 or now>1.0e12:return false
 var count:int=clampi(slots,1,8)
 # Reduce the clock before integer conversion to avoid overflow in long saves.
 var cycle:float=float(count)*0.25
 var phase:float=fposmod(now,cycle)
 var slot:int=mini(count-1,int(floor(phase*4.0)))
 return slot==posmod(beast_id,count)

# Ambushers wait for their assigned quarter-second turn; bruisers get
# two slots to keep heavier melee enemies threatening but predictable.
static func role_attack_ready(kind:String,now:float,beast_id:int)->bool:
 if group_attack_ready(now,beast_id):return true
 return role_for(kind)=="bruiser" and group_attack_ready(now,beast_id+1)

static func preferred_spacing(kind:String)->float:
 return 2.9 if kind=="panther" else (2.0 if kind=="serpent" else (2.6 if kind=="wolf" else (2.4 if role_for(kind)=="ambusher" else 1.7)))

static func wants_flank(kind:String,distance:float,has_sight:bool)->bool:
 return has_sight and is_finite(distance) and distance>preferred_spacing(kind) and distance<(8.2 if kind=="panther" else (6.0 if kind=="serpent" else 6.5)) and role_for(kind)=="ambusher"

# Attack turns use the same phase clock for every creature. Adjusting
# slots by role prevents the heavier bruisers from dominating mixed packs.
static func attack_window_count(kind:String)->int:
 return 5 if kind=="serpent" else (5 if kind=="boar" else (4 if role_for(kind)=="ambusher" else 6))

static func tactical_attack_ready(kind:String,now:float,beast_id:int)->bool:
 if not is_finite(now) or now<0.0 or now>1.0e12:return false
 var slots:int=attack_window_count(kind)
 if group_attack_ready(now,beast_id,slots):return true
 return role_for(kind)=="bruiser" and group_attack_ready(now,beast_id+1,slots)

static func attack_timer_after_frame(current:float,delta:float)->float:
 if not is_finite(current):return 0.1
 var elapsed:float=clampf(delta,0.0,0.1) if is_finite(delta) else 0.0
 return maxf(0.0,clampf(current,0.0,30.0)-elapsed)

static func should_circle(kind:String,intent:String,distance:float)->bool:
 return intent=="strafe" and wants_flank(kind,distance,true) and distance<(6.2 if kind=="panther" else (5.0 if kind=="serpent" else 5.5))

## Melee-only creatures must not kite as ranged characters.
static func melee_engage_range(_kind:String)->float:
 return 1.8 if _kind=="boar" else 1.7
static func can_attack(distance:float,ready:bool,has_sight:bool)->bool:
 return ready and has_sight and is_finite(distance) and distance>=0.0 and distance<melee_engage_range("")
static func orbit_direction(beast_id:int)->float:
 return 1.0 if posmod(beast_id,2)==0 else -1.0

static func ready_to_strike(timer:float)->bool:
 return is_finite(timer) and timer<=0.0

# Memory is per creature: no scans, pathfinders, or extra nodes per frame.
static func remember_target(seen:bool,hostile:bool,boss:bool,now:float,hero_pos:Vector3,old_pos:Vector3,old_until:float)->Dictionary:
 if not hostile or boss or not is_finite(now) or now<0.0 or now>1.0e12:
  return {"position":old_pos,"until":0.0}
 if seen and is_finite(hero_pos.x) and is_finite(hero_pos.y) and is_finite(hero_pos.z):
  return {"position":hero_pos,"until":now+3.0}
 # Expired, corrupt or abnormally long memories must not extend a chase.
 var valid_old:bool=is_finite(old_pos.x) and is_finite(old_pos.y) and is_finite(old_pos.z)
 var valid_expiry:bool=is_finite(old_until) and old_until>now and old_until<=now+3.0
 return {"position":old_pos if valid_old else Vector3.ZERO,"until":old_until if valid_old and valid_expiry else 0.0}

static func investigate_velocity(hostile:bool,engaged:bool,boss:bool,now:float,until:float,body_pos:Vector3,last_pos:Vector3,speed:float)->Vector3:
 if not hostile or engaged or boss or not is_finite(now) or now<0.0 or not is_finite(until) or now>=until or until>now+3.0:
  return Vector3.ZERO
 var offset:Vector3=last_pos-body_pos
 offset.y=0.0
 var distance_sq:float=offset.length_squared()
 if not is_finite(offset.x) or not is_finite(offset.z) or not is_finite(distance_sq) or distance_sq<1.0:
  return Vector3.ZERO
 # Slow as the search point approaches so actors do not oscillate over it.
 var max_speed:float=minf(clampf(speed,0.0,18.0)*0.65,(sqrt(distance_sq)-1.0)*3.5) if is_finite(speed) and speed>0.0 else 0.0
 return offset.normalized()*maxf(0.0,max_speed)

static func search_arrived(body_pos:Vector3,last_pos:Vector3)->bool:
 var offset:Vector3=last_pos-body_pos
 offset.y=0.0
 var distance_sq:float=offset.length_squared()
 return is_finite(offset.x) and is_finite(offset.z) and is_finite(distance_sq) and distance_sq<=1.0

# Brief look-around at the last known location; sighting a hero cancels it.
static func search_pause_active(engaged:bool, now:float, pause_until:float)->bool:
 return not engaged and is_finite(now) and now>=0.0 and is_finite(pause_until) and pause_until>now and pause_until<=now+0.8

static func search_pause_expiry(now:float)->float:
 return now+0.75 if is_finite(now) and now>=0.0 and now<=1.0e12 else 0.0
