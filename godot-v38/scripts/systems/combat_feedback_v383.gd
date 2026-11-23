extends RefCounted
## Pure, bounded enemy hit-recoil rules; no new nodes, allocations, or gameplay damage changes.
const REACTION_SECONDS:float=0.16
const NORMAL_ANGLE:float=9.0
const CRITICAL_ANGLE:float=14.0
const ATTACK_SWING_SECONDS:float=0.22
const ATTACK_SWING_ANGLE:float=13.0

# Ordinary melee foes telegraph before contact. Elite lunges and guardian
# windups retain their separate, existing schedulers and timing.
const MELEE_TELL_SECONDS:float=0.42
const MELEE_TELL_ANGLE:float=12.0
const MELEE_STRIKE_RANGE:float=1.7
const MELEE_CANCEL_COOLDOWN:float=0.35

static func melee_tell_degrees(time_remaining:float)->float:
 # Visual anticipation starts neutral and pulls back as the strike approaches.
 # Existing actor meshes only: no new animation nodes or texture alterations.
 var fraction:float=clampf(time_remaining/MELEE_TELL_SECONDS,0.0,1.0)
 return -MELEE_TELL_ANGLE*(1.0-fraction)

static func melee_strike_allowed(engaged:bool,distance:float,staggered:bool,dodging:bool,has_sight:bool)->bool:
 # Recheck at impact: leaving range, breaking sight, dodging or staggering
 # the attacker must never leave behind a guaranteed delayed hit.
 return engaged and distance<MELEE_STRIKE_RANGE and not staggered and not dodging and has_sight

static func attack_followthrough_degrees(time_remaining:float)->float:
 # Short post-strike arc: zero at the start and end, peaking mid-animation.
 var fraction:float=clampf(time_remaining/ATTACK_SWING_SECONDS,0.0,1.0)
 return ATTACK_SWING_ANGLE*sin(PI*fraction)


static func recoil_degrees(time_remaining:float,critical:bool)->float:
 var fraction:float=clampf(time_remaining/REACTION_SECONDS,0.0,1.0)
 return (CRITICAL_ANGLE if critical else NORMAL_ANGLE)*fraction

## Animate the existing boss warning disk's opacity, not its radius or damage area.
static func boss_telegraph_transparency(elapsed:float,phase:int)->float:
 var speed:float=2.0+float(clampi(phase,1,3)-1)*0.65
 var wave:float=0.5+0.5*sin(maxf(0.0,elapsed)*TAU*speed)
 return 0.05+0.24*wave

## Avoid forcing Godot to rebuild unchanged CylinderMesh geometry every frame.
static func warning_radius_changed(current_radius:float,target_radius:float)->bool:
 return not is_equal_approx(current_radius,target_radius)
