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
