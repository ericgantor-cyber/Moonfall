extends RefCounted
## Isolated combat arithmetic; callers still own RNG, health, effects and rewards.
static func outgoing_damage(base:float,critical:bool,wounded:bool,execute_rank:int,fury:float,power_modifier:float) -> float:
 # Invalid saved buffs must not inject NaN, Infinity or negative damage into combat.
 if not is_finite(base) or base<=0.0 or not is_finite(fury) or not is_finite(power_modifier):return 0.0
 if fury<0.0 or power_modifier<=0.0 or execute_rank<0:return 0.0
 # Bound unreasonable corrupted modifiers without changing normal attack calculations.
 if base>1.0e8 or fury>100.0 or power_modifier>100.0 or execute_rank>1000:return 0.0
 var damage:float=base*(1.6 if critical else 1.0)
 if wounded:damage*=1.0+0.1*float(execute_rank)
 damage*=1.0+fury
 damage*=power_modifier
 return damage if is_finite(damage) and damage>=0.0 else 0.0

static func elite_missed(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float) -> bool:
 # Invalid strike coordinates cannot legitimately hit; corrupt dodge timers cannot grant immunity.
 if not is_finite(strike_pos.x) or not is_finite(strike_pos.z) or not is_finite(hero_pos.x) or not is_finite(hero_pos.z):return true
 var offset_x:float=strike_pos.x-hero_pos.x
 var offset_z:float=strike_pos.z-hero_pos.z
 if not is_finite(offset_x) or not is_finite(offset_z):return true
 var d_sq:float=offset_x*offset_x+offset_z*offset_z
 if not is_finite(d_sq):return true
 return d_sq>=1.9*1.9 or (is_finite(dodge_remaining) and dodge_remaining>0.0)

static func guardian_attack_cooldown(phase:int) -> float:
 return maxf(1.6,3.0-0.45*float(phase-1))
