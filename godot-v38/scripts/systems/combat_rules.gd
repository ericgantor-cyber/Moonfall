extends RefCounted
## Isolated combat arithmetic; callers own random rolls, hit effects and rewards.
## Preserve normal damage scaling while rejecting broken values from saves or mods.
static func outgoing_damage(base:float,critical:bool,wounded:bool,execute_rank:int,fury:float,power_modifier:float) -> float:
 if not is_finite(base) or base<=0.0:return 0.0
 var safe_base:float=minf(base,1000000.0)
 var rank:int=clampi(execute_rank,0,100)
 var safe_fury:float=clampf(fury,0.0,10.0) if is_finite(fury) else 0.0
 var safe_power:float=clampf(power_modifier,0.0,100.0) if is_finite(power_modifier) else 1.0
 var damage:float=safe_base*(1.6 if critical else 1.0)
 if wounded:damage*=1.0+0.1*float(rank)
 damage*=1.0+safe_fury
 damage*=safe_power
 return minf(damage,1000000000.0)

# Live dodge windows are 0.5 or 0.55 seconds. Invalid state cannot extend immunity.
static func dodge_timer_after_frame(current:float,delta:float)->float:
 if not is_finite(current) or current<0.0 or current>0.55:return 0.0
 if not is_finite(delta) or delta<=0.0:return current
 return maxf(0.0,current-delta)

const ELITE_HIT_RADIUS_SQUARED:float=1.9*1.9

# Shared ground-plane reach check for impact and dodge statistics.
static func elite_in_hit_radius(strike_pos:Vector3,hero_pos:Vector3)->bool:
 if not is_finite(strike_pos.x) or not is_finite(strike_pos.y) or not is_finite(strike_pos.z):return false
 if not is_finite(hero_pos.x) or not is_finite(hero_pos.y) or not is_finite(hero_pos.z):return false
 var planar_offset:=Vector2(strike_pos.x-hero_pos.x,strike_pos.z-hero_pos.z)
 var distance_sq:float=planar_offset.length_squared()
 return is_finite(distance_sq) and distance_sq<ELITE_HIT_RADIUS_SQUARED

static func elite_missed(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float) -> bool:
 # Corrupt dodge clocks must never be interpreted as permission to hit.
 if not is_finite(dodge_remaining) or dodge_remaining>0.0:return true
 return not elite_in_hit_radius(strike_pos,hero_pos)

# Only credit a timed dodge when the attack would otherwise land.
static func elite_dodge_credit(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float,clear_sight:bool=false)->bool:
 # Live dodge actions last at most 0.55 seconds; corrupt long clocks earn no credit.
 return clear_sight and is_finite(dodge_remaining) and dodge_remaining>0.0 and dodge_remaining<=0.55 and elite_in_hit_radius(strike_pos,hero_pos)

static func guardian_attack_cooldown(phase:int) -> float:
 return maxf(1.6,3.0-0.45*float(clampi(phase,1,10)-1))
