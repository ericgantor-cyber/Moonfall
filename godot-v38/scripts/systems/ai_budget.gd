extends RefCounted
## v28: only throttle settled, distant, inactive wildlife. This code is node-free.
const FAR_IDLE_DISTANCE_SQ:float=26.0*26.0
const IDLE_INTERVAL:int=4

static func may_throttle(distance_sq:float, velocity_sq:float, home_distance_sq:float, hostile:bool, boss:bool, dungeon_site:int, engaged:bool, alerted:bool, staggered:bool, windup:bool, hunting:bool, frightened:bool, retreating:bool) -> bool:
 if distance_sq<FAR_IDLE_DISTANCE_SQ or velocity_sq>0.0025 or home_distance_sq>1.0:return false
 if boss or dungeon_site>=0 or engaged or alerted or staggered or windup or hunting or frightened or retreating:return false
 return true

static func should_tick(frame:int,slot:int) -> bool:
 return posmod(frame+slot,IDLE_INTERVAL)==0
