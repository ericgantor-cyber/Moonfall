extends RefCounted
## Pure, bounded enemy hit-recoil rules; no new nodes, allocations, or gameplay damage changes.
const REACTION_SECONDS:float=0.16
const NORMAL_ANGLE:float=9.0
const CRITICAL_ANGLE:float=14.0
const ATTACK_SWING_SECONDS:float=0.22
const ATTACK_SWING_ANGLE:float=13.0

static func attack_followthrough_degrees(time_remaining:float)->float:
 # Short post-strike arc: zero at the start and end, peaking mid-animation.
 if not is_finite(time_remaining):return 0.0
 var fraction:float=clampf(time_remaining/ATTACK_SWING_SECONDS,0.0,1.0)
 return ATTACK_SWING_ANGLE*sin(PI*fraction)


static func recoil_degrees(time_remaining:float,critical:bool)->float:
 if not is_finite(time_remaining):return 0.0
 var fraction:float=clampf(time_remaining/REACTION_SECONDS,0.0,1.0)
 return (CRITICAL_ANGLE if critical else NORMAL_ANGLE)*fraction

## Animate the existing boss warning disk's opacity, not its radius or damage area.
static func boss_telegraph_transparency(elapsed:float,phase:int)->float:
 # Corrupt clocks must not write NaN alpha to the existing warning mesh.
 if not is_finite(elapsed):return 0.17
 var speed:float=2.0+float(clampi(phase,1,3)-1)*0.65
 var wave:float=0.5+0.5*sin(maxf(0.0,elapsed)*TAU*speed)
 return 0.05+0.24*wave

## Avoid forcing Godot to rebuild unchanged CylinderMesh geometry every frame.
static func warning_radius_changed(current_radius:float,target_radius:float)->bool:
 # Never rebuild a cylinder mesh using an invalid radius.
 if not is_finite(target_radius) or target_radius<=0.0:return false
 if not is_finite(current_radius):return true
 return not is_equal_approx(current_radius,target_radius)
