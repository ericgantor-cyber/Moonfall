extends RefCounted
## Pure, frame-rate-independent underwater breathing rules. Not a survival clock.
const MAX_OXYGEN:float=100.0
const DRAIN_PER_SECOND:float=4.0
const REFILL_PER_SECOND:float=25.0
const LOW_OXYGEN:float=25.0
const DROWN_TICK_SECONDS:float=1.25
const DROWN_DAMAGE_FRACTION:float=0.08
const DIVE_MOVE_MULTIPLIER:float=0.85

static func advance(current:float,delta:float,underwater:bool)->float:
 var dt:float=clampf(delta,0.0,0.25)
 var change:float=(-DRAIN_PER_SECOND if underwater else REFILL_PER_SECOND)*dt
 return clampf(current+change,0.0,MAX_OXYGEN)

static func can_dive(in_lake:bool,playing:bool,paused:bool,mounted:bool,dead:bool)->bool:
 return in_lake and playing and not paused and not mounted and not dead

static func is_low(oxygen:float)->bool:
 return oxygen<=LOW_OXYGEN

static func damage_tick(max_health:float)->float:
 return maxf(5.0,max_health*DROWN_DAMAGE_FRACTION)
