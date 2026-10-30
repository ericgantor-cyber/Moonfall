extends RefCounted
## Pure, bounded enemy hit-recoil rules; no new nodes, allocations, or gameplay damage changes.
const REACTION_SECONDS:float=0.16
const NORMAL_ANGLE:float=9.0
const CRITICAL_ANGLE:float=14.0

static func recoil_degrees(time_remaining:float,critical:bool)->float:
 var fraction:float=clampf(time_remaining/REACTION_SECONDS,0.0,1.0)
 return (CRITICAL_ANGLE if critical else NORMAL_ANGLE)*fraction
