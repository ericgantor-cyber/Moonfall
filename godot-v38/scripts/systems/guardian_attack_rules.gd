extends RefCounted
## Pure guardian attack selection: same damage/timing, fewer duplicated calculations.
static func pattern_count(phase:int)->int:
 return mini(4,clampi(phase,1,3)+1)
static func pattern_index(pattern:int,phase:int)->int:
 return posmod(pattern,pattern_count(phase))
static func next_pattern(pattern:int,phase:int)->int:
 return (pattern_index(pattern,phase)+1)%pattern_count(phase)

# Phase upgrades can introduce unfamiliar attacks without repeating the previous
# pattern immediately. Stay deterministic for replay and save compatibility.
static func next_pattern_after_phase_change(previous_pattern:int,new_phase:int)->int:
 var count:int=pattern_count(new_phase)
 # Normalize first: extreme saved attack IDs must not overflow addition.
 var previous_slot:int=posmod(previous_pattern,count)
 var next:int=(previous_slot+1)%count
 return next
# Phase transitions unlock additional attacks while preserving the existing
# deterministic rotation and the player's learned dodge timings.
static func unlocked_patterns(phase:int)->Array[int]:
 var result:Array[int]=[]
 for index in range(pattern_count(phase)):
  result.append(index)
 return result

static func pattern_unlocked(pattern:int,phase:int)->bool:
 return pattern>=0 and pattern<pattern_count(phase)

static func radius(pattern:int)->float:
 match posmod(pattern,4):
  0:return 2.8
  1:return 3.8
  2:return 2.2
  _:return 3.1
static func windup(phase:int,base:float)->float:
 if not is_finite(base):return 0.75
 return clampf(base-0.25*float(clampi(phase,1,3)-1),0.75,8.0)

# Treat malformed/old save data as safe, bounded combat geometry.
static func safe_phase(phase:int)->int:
 return clampi(phase,1,3)

static func safe_radius(value:float)->float:
 if not is_finite(value):return 2.8
 return clampf(value,0.5,4.0)

static func warning_hit(marked:Vector3,hero:Vector3,radius_value:float)->bool:
 if not is_finite(marked.x) or not is_finite(marked.z) or not is_finite(hero.x) or not is_finite(hero.z):return false
 var radius_safe:float=safe_radius(radius_value)
 var offset:Vector2=Vector2(marked.x-hero.x,marked.z-hero.z)
 if not is_finite(offset.x) or not is_finite(offset.y):return false
 var distance_sq:float=offset.length_squared()
 return is_finite(distance_sq) and distance_sq<radius_safe*radius_safe

static func boss_damage(phase:int)->float:
 return 19.0+7.0*float(safe_phase(phase))
