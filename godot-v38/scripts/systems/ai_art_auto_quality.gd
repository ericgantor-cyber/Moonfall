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

# Detail budgets scale with the number of visible, eligible models.
# High has NO fixed 3-creature / 1-room ceiling; performance adapts over time.
# Eco remains available automatically if the system cannot sustain High.
static func detail_budget(tier:int,eligible_count:int)->int:
 var count:int=maxi(0,eligible_count)
 match clampi(tier,MIN_QUALITY,MAX_QUALITY):
  ECONOMY:return 0
  BALANCED:return ceili(float(count)*0.5)
 return count

static func animal_limit(tier:int,eligible_count:int=0)->int:
 return detail_budget(tier,eligible_count)

static func chamber_limit(tier:int,eligible_count:int=0)->int:
 return detail_budget(tier,eligible_count)

static func next_tier(tier:int,slow_seconds:float,fast_seconds:float)->int:
 var bounded:int=clampi(tier,MIN_QUALITY,MAX_QUALITY)
 if slow_seconds>=SLOW_HOLD_SECONDS:return maxi(MIN_QUALITY,bounded-1)
 if fast_seconds>=FAST_HOLD_SECONDS:return mini(MAX_QUALITY,bounded+1)
 return bounded

static func should_degrade(average_ms:float)->bool:
 return is_finite(average_ms) and average_ms>=SLOW_FRAME_MS

static func should_upgrade(average_ms:float)->bool:
 return is_finite(average_ms) and average_ms<=FAST_FRAME_MS

# Throttle state POLLING on distant imported rigs, never the AnimationPlayer itself.
# Combat-critical state changes are always applied on the next 80-ms update.
# Phase spreads costly distant actors across ticks without arbitrary actor limits.
static func animation_poll_stride(tier:int,distance_sq:float)->int:
 if not is_finite(distance_sq) or distance_sq<0.0:return 1
 if distance_sq<=16.0*16.0:return 1
 var far:bool=distance_sq>32.0*32.0
 match clampi(tier,MIN_QUALITY,MAX_QUALITY):
  ECONOMY:return 4 if far else 2
  BALANCED:return 3 if far else 2
 return 2 if far else 1

static func should_poll_animation(tier:int,distance_sq:float,tick:int,phase:int,requested:String,previous:String)->bool:
 # Always deliver the first bind and EVERY state change immediately. Otherwise,
 # unchanged distant idle/move loops may be polled less frequently.
 if requested!=previous:return true
 if requested in ["attack","hurt","death","telegraph","spawn","special_1","special_2"]:return true
 var stride:int=animation_poll_stride(tier,distance_sq)
 return posmod(tick+phase,stride)==0
