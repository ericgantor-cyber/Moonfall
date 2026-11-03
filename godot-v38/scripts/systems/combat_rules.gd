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

static func elite_missed(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float) -> bool:
 if not is_finite(strike_pos.x) or not is_finite(strike_pos.z):return true
 if not is_finite(hero_pos.x) or not is_finite(hero_pos.z):return true
 # Corrupt dodge clocks must never be interpreted as permission to hit.
 if not is_finite(dodge_remaining) or dodge_remaining>0.0:return true
 # Compare squared planar distance: same reach, no per-hit square root.
 var planar_offset:=Vector2(strike_pos.x-hero_pos.x,strike_pos.z-hero_pos.z)
 return planar_offset.length_squared()>=1.9*1.9


# Only credit a timed dodge when the attack would otherwise land.
static func elite_dodge_credit(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float,clear_sight:bool=true)->bool:
 if not clear_sight or not is_finite(dodge_remaining) or dodge_remaining<=0.0:return false
 if not is_finite(strike_pos.x) or not is_finite(strike_pos.z) or not is_finite(hero_pos.x) or not is_finite(hero_pos.z):return false
 var offset:=Vector2(strike_pos.x-hero_pos.x,strike_pos.z-hero_pos.z)
 var distance_sq:float=offset.length_squared()
 return is_finite(distance_sq) and distance_sq<1.9*1.9

static func guardian_attack_cooldown(phase:int) -> float:
 return maxf(1.6,3.0-0.45*float(clampi(phase,1,10)-1))
