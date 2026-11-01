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
 return maxf(0.0,current-clampf(delta,0.0,0.1))

static func should_circle(kind:String,intent:String,distance:float)->bool:
 return intent=="strafe" and kind in ["wolf","panther","serpent"] and distance>1.7 and distance<6.0

## Melee-only creatures must not kite as ranged characters.
static func melee_engage_range(_kind:String)->float:
 return 1.7
static func can_attack(distance:float,ready:bool,has_sight:bool)->bool:
 return ready and has_sight and distance>=0.0 and distance<1.7
static func orbit_direction(beast_id:int)->float:
 return 1.0 if posmod(beast_id,2)==0 else -1.0
