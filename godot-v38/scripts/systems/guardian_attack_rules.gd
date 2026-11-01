extends RefCounted
## Pure guardian attack selection: same damage/timing, fewer duplicated calculations.
static func pattern_count(phase:int)->int:
 return mini(4,clampi(phase,1,3)+1)
static func pattern_index(pattern:int,phase:int)->int:
 return posmod(pattern,pattern_count(phase))
static func next_pattern(pattern:int,phase:int)->int:
 return (pattern_index(pattern,phase)+1)%pattern_count(phase)
static func radius(pattern:int)->float:
 match posmod(pattern,4):
  0:return 2.8
  1:return 3.8
  2:return 2.2
  _:return 3.1
static func windup(phase:int,base:float)->float:
 return maxf(0.75,base-0.25*float(clampi(phase,1,3)-1))

# Treat malformed/old save data as safe, bounded combat geometry.
static func safe_phase(phase:int)->int:
 return clampi(phase,1,3)

static func safe_radius(value:float)->float:
 if not is_finite(value):return 2.8
 return clampf(value,0.5,4.0)

static func warning_hit(marked:Vector3,hero:Vector3,radius_value:float)->bool:
 var radius_safe:float=safe_radius(radius_value)
 return Vector2(marked.x-hero.x,marked.z-hero.z).length_squared()<radius_safe*radius_safe

static func boss_damage(phase:int)->float:
 return 19.0+7.0*float(safe_phase(phase))
