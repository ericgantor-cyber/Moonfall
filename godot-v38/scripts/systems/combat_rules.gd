extends RefCounted
## Isolated combat arithmetic; callers still own RNG, health, effects and rewards.
static func outgoing_damage(base:float,critical:bool,wounded:bool,execute_rank:int,fury:float,power_modifier:float) -> float:
 var damage:=base*(1.6 if critical else 1.0)
 if wounded:damage*=1.0+0.1*float(execute_rank)
 damage*=1.0+fury
 damage*=power_modifier
 return damage

static func elite_missed(strike_pos:Vector3,hero_pos:Vector3,dodge_remaining:float) -> bool:
 return Vector2(strike_pos.x-hero_pos.x,strike_pos.z-hero_pos.z).length()>=1.9 or dodge_remaining>0.0

static func guardian_attack_cooldown(phase:int) -> float:
 return maxf(1.6,3.0-0.45*float(phase-1))
