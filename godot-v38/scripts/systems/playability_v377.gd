extends RefCounted
## v37.7: reusable, read-only game-state rules. No timers, saves or textures.
const MIN_SUBMERGED_DEPTH:float=0.32
const CRITICAL_OXYGEN:float=25.0
const CRITICAL_HEALTH_FRACTION:float=0.35
const SAFE_SHORE_X:float=6.7
const SAFE_SHORE_Z:float=5.4

static func rescue_available(playing:bool,dead:bool,mounted:bool,depth:float,zone:int,threat:bool)->bool:
 return playing and not dead and not mounted and not threat and zone>=0 and depth>MIN_SUBMERGED_DEPTH

static func shoreline(regions:Array,zone:int,offset:Vector2,surface_y:float)->Vector3:
 if zone<0 or zone>=regions.size():return Vector3.ZERO
 var center:Vector2=regions[zone]["at"]
 return Vector3(center.x+offset.x+SAFE_SHORE_X,surface_y,center.y+offset.y+SAFE_SHORE_Z)

static func urgent_tip(depth:float,air:float,health:float,max_health:float,threat:bool)->String:
 if depth>MIN_SUBMERGED_DEPTH and air<=CRITICAL_OXYGEN:
  return "LOW AIR: ascend with Space / controller A. Open System for emergency surfacing when out of combat."
 if depth>MIN_SUBMERGED_DEPTH:
  return "UNDERWATER: C/right-stick dives; Space/controller A ascends. Watch oxygen and explore the grotto caves."
 if health<max_health*CRITICAL_HEALTH_FRACTION and max_health>0.0:
  return "LOW HEALTH: heal with food, draughts or a sanctuary before the next battle."
 if threat:return "COMBAT: prioritize attack warnings, dodge timing and a safe escape route."
 return "EXPLORE: follow your Atlas, gather resources, and seal rifts to purify the region."

static func format_percent(amount:float,max_amount:float)->String:
 if max_amount<=0.0:return "0%"
 return str(clampi(roundi(amount/max_amount*100.0),0,100))+"%"
