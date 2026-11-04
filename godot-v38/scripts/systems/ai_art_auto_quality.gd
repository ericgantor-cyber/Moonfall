extends RefCounted
## Conservative automatic detail governor for optional imported 4K models.
## This changes visual quality only, never collisions, difficulty or movement.
const ECONOMY:int=0
const BALANCED:int=1
const HIGH:int=2
const MIN_QUALITY:int=ECONOMY
const MAX_QUALITY:int=HIGH
const SLOW_FRAME_MS:float=27.0
const FAST_FRAME_MS:float=17.5
const SLOW_HOLD_SECONDS:float=3.0
const FAST_HOLD_SECONDS:float=16.0
const MIN_LOAD_INTERVAL_MS:int=900
const ANIMAL_ENTER_RADIUS_SQ:float=22.0*22.0
const ANIMAL_EXIT_RADIUS_SQ:float=28.0*28.0
const ROOM_ENTER_RADIUS_SQ:float=30.0*30.0
const ROOM_EXIT_RADIUS_SQ:float=36.0*36.0

static func quality_name(tier:int)->String:
 match clampi(tier,MIN_QUALITY,MAX_QUALITY):
  ECONOMY:return "Eco"
  BALANCED:return "Balanced"
 return "High"

static func animal_limit(tier:int)->int:
 match clampi(tier,MIN_QUALITY,MAX_QUALITY):
  ECONOMY:return 0
  BALANCED:return 1
 return 3

static func chamber_limit(tier:int)->int:
 return 1 if tier>=HIGH else 0

static func next_tier(tier:int,slow_seconds:float,fast_seconds:float)->int:
 var bounded:int=clampi(tier,MIN_QUALITY,MAX_QUALITY)
 if slow_seconds>=SLOW_HOLD_SECONDS:return maxi(MIN_QUALITY,bounded-1)
 if fast_seconds>=FAST_HOLD_SECONDS:return mini(MAX_QUALITY,bounded+1)
 return bounded

static func should_degrade(average_ms:float)->bool:
 return is_finite(average_ms) and average_ms>=SLOW_FRAME_MS

static func should_upgrade(average_ms:float)->bool:
 return is_finite(average_ms) and average_ms<=FAST_FRAME_MS
