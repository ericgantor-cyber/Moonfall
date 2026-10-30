extends RefCounted
## Pure, bounded balance rules. Not a survival-time difficulty ramp.
const MAX_RESONANCE:int=100
const HIT_GAIN:int=8
const KILL_GAIN:int=12
const BOSS_GAIN:int=18
const PERFECT_DODGE_GAIN:int=22
const FINISHER_TARGET_CAP:int=4
const FINISHER_RANGE_SQ:float=6.8*6.8

static func gain(current:int,amount:int)->int:
 return clampi(current+maxi(0,amount),0,MAX_RESONANCE)

static func cleared_boon_count(cleared_count:int)->int:
 return clampi(cleared_count,0,3)

static func damage_boon(cleared_count:int)->float:
 return 1.0+float(cleared_boon_count(cleared_count))*0.025

static func guard_boon(cleared_count:int)->float:
 return 1.0-float(cleared_boon_count(cleared_count))*0.025

static func rift_surge_damage(stage:int,region:int)->float:
 return 7.0+float(clampi(stage,1,3))*3.0+float(clampi(region,0,5))*1.0

static func surge_rest(stage:int)->float:
 return 6.5 if stage<=1 else 6.0 if stage==2 else 5.6

static func finisher_power(level:int,cleared_count:int,is_nyra:bool)->float:
 return (23.0+float(clampi(level,1,200))*0.72+float(cleared_boon_count(cleared_count))*4.0)*(1.12 if is_nyra else 1.0)

static func safe_surge_position(anchor:Vector3,target:Vector3,stage:int)->Vector3:
 # Keep danger inside the existing corrupted rift, not under another world object.
 var displacement:Vector3=target-anchor
 displacement.y=0.0
 if stage<=1:
  displacement=Vector3(5.0,0.0,3.0)
 elif displacement.length_squared()>8.0*8.0:
  displacement=displacement.normalized()*8.0
 return anchor+displacement+Vector3(0.0,0.10,0.0)
