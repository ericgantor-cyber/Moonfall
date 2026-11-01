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
  "panther":return 1.25
  "wolf":return 1.35
  "serpent":return 1.65
  "boar":return 1.75
  _:return 1.5

static func attack_timer_after_frame(current:float,delta:float)->float:
 return maxf(0.0,clampf(current,0.0,30.0)-clampf(delta if is_finite(delta) else 0.0,0.0,0.1)) if is_finite(current) else 0.0

static func should_circle(kind:String,intent:String,distance:float)->bool:
 return is_finite(distance) and intent=="strafe" and kind in ["wolf","panther","serpent"] and distance>1.7 and distance<6.0

## Melee-only creatures must not kite as ranged characters.
static func melee_engage_range(_kind:String)->float:
 return 1.7
static func can_attack(distance:float,ready:bool,has_sight:bool)->bool:
 return ready and has_sight and is_finite(distance) and distance>=0.0 and distance<melee_engage_range("")
static func orbit_direction(beast_id:int)->float:
 return 1.0 if posmod(beast_id,2)==0 else -1.0

static func ready_to_strike(timer:float)->bool:
 return is_finite(timer) and timer<=0.0

# Memory is per creature: no scans, pathfinders, or extra nodes per frame.
static func remember_target(seen:bool,hostile:bool,boss:bool,now:float,hero_pos:Vector3,old_pos:Vector3,old_until:float)->Dictionary:
 if not hostile or boss or not is_finite(now):
  return {"position":old_pos,"until":0.0}
 if seen and is_finite(hero_pos.x) and is_finite(hero_pos.y) and is_finite(hero_pos.z):
  return {"position":hero_pos,"until":now+3.0}
 return {"position":old_pos,"until":old_until}

static func investigate_velocity(hostile:bool,engaged:bool,boss:bool,now:float,until:float,body_pos:Vector3,last_pos:Vector3,speed:float)->Vector3:
 if not hostile or engaged or boss or not is_finite(now) or now>=until:
  return Vector3.ZERO
 var offset:Vector3=last_pos-body_pos
 offset.y=0.0
 if offset.length_squared()<1.0 or not is_finite(offset.x) or not is_finite(offset.z):
  return Vector3.ZERO
 # Slow as the search point approaches so actors do not oscillate over it.
 var max_speed:float=minf(clampf(speed,0.0,20.0)*0.65,(sqrt(offset.length_squared())-1.0)*4.0) if is_finite(speed) else 0.0
 return offset.normalized()*maxf(0.0,max_speed)

static func search_arrived(body_pos:Vector3,last_pos:Vector3)->bool:
 var offset:Vector3=last_pos-body_pos
 offset.y=0.0
 return is_finite(offset.x) and is_finite(offset.z) and offset.length_squared()<1.0
