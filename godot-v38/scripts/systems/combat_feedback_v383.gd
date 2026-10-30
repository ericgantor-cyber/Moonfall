extends RefCounted
## Pure, bounded enemy hit-recoil rules; no new nodes, allocations, or gameplay damage changes.
const REACTION_SECONDS:float=0.16
const NORMAL_ANGLE:float=9.0
const CRITICAL_ANGLE:float=14.0
const ATTACK_SWING_SECONDS:float=0.22
const ATTACK_SWING_ANGLE:float=13.0

static func attack_followthrough_degrees(time_remaining:float)->float:
 # Short post-strike arc: zero at the start and end, peaking mid-animation.
 var fraction:float=clampf(time_remaining/ATTACK_SWING_SECONDS,0.0,1.0)
 return ATTACK_SWING_ANGLE*sin(PI*fraction)


static func recoil_degrees(time_remaining:float,critical:bool)->float:
 var fraction:float=clampf(time_remaining/REACTION_SECONDS,0.0,1.0)
 return (CRITICAL_ANGLE if critical else NORMAL_ANGLE)*fraction
