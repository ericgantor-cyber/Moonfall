extends RefCounted
## Renderer-independent melee timing helpers inspired by fast, readable action combat.
## Shared rules for live combat rhythm; original Z/Nyra artwork unchanged.
const MAX_CHAIN:int = 5
const CHAIN_WINDOW:float = 0.48
const PERFECT_DODGE_WINDOW:float = 0.16
const PARRY_WINDOW:float = 0.13

static func next_chain(current:int, seconds_since_hit:float) -> int:
 if not is_finite(seconds_since_hit) or seconds_since_hit < 0.0:
  return 1
 if seconds_since_hit > CHAIN_WINDOW or current < 1 or current >= MAX_CHAIN:
  return 1
 return current + 1

static func chain_damage_multiplier(chain:int, heavy_finisher:bool = false) -> float:
 var stage:int = clampi(chain, 1, MAX_CHAIN)
 var multiplier:float = 1.0 + 0.12 * float(stage - 1)
 return multiplier * (1.65 if heavy_finisher and stage == MAX_CHAIN else 1.0)

static func perfect_dodge(dodge_elapsed:float, attack_connected:bool) -> bool:
 return attack_connected and is_finite(dodge_elapsed) and dodge_elapsed >= 0.0 and dodge_elapsed <= PERFECT_DODGE_WINDOW

static func successful_parry(parry_elapsed:float, incoming_parryable:bool) -> bool:
 return incoming_parryable and is_finite(parry_elapsed) and parry_elapsed >= 0.0 and parry_elapsed <= PARRY_WINDOW

static func posture_after_hit(current:float, maximum:float, impact:float, was_parry:bool = false) -> float:
 if not is_finite(current) or not is_finite(maximum) or not is_finite(impact) or maximum <= 0.0:
  return 0.0
 var added:float = maxf(impact, 0.0) * (2.0 if was_parry else 1.0)
 return clampf(current + added, 0.0, maximum)

static func posture_broken(current:float, maximum:float) -> bool:
 return is_finite(current) and is_finite(maximum) and maximum > 0.0 and current >= maximum

static func safe_stamina_cost(base_cost:float, combo_stage:int, heavy:bool = false) -> float:
 if not is_finite(base_cost) or base_cost < 0.0:
  return 0.0
 return minf(1000.0, base_cost * (1.7 if heavy else 1.0) * (1.0 + 0.05 * float(clampi(combo_stage, 1, MAX_CHAIN) - 1)))

## Recover guard pressure lazily between landed hits, without per-frame enemy scans.
static func recovered_posture(current:float, elapsed:float, grace:float = 2.0, recovery_per_second:float = 15.0) -> float:
 if not is_finite(current) or not is_finite(elapsed) or not is_finite(grace) or not is_finite(recovery_per_second):
  return 0.0
 if current <= 0.0:return 0.0
 if elapsed <= grace or elapsed < 0.0:return current
 return maxf(0.0,current - (elapsed - maxf(grace,0.0))*maxf(recovery_per_second,0.0))
