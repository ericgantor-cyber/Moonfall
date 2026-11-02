extends RefCounted
## Pure, deterministic Moonfall v31.3 encounter rules. No asset or save mutations.
const COMBO_WINDOW:float=1.25
const MAX_COMBO:int=3
const SURGE_SECONDS:float=28.0
const SURGE_INTERVAL:float=165.0
const TRAP_WARNING:float=1.15
const TRAP_COOLDOWN:float=6.0

static func next_combo(now:float,expires:float,chain:int)->int:
 return mini(MAX_COMBO,chain+1) if now<=expires else 1

static func combo_multiplier(chain:int)->float:
 return 1.0+0.08*float(clampi(chain,1,MAX_COMBO)-1)

static func hurt_retreat(hp:float,max_hp:float,is_boss:bool,alert:bool,cooldown_ready:bool)->bool:
 return alert and not is_boss and cooldown_ready and max_hp>0.0 and hp/max_hp<0.26

# Record proximity only when selecting a new attack; forget pressure if
# the hero is distant or unavailable. No allocations or per-frame scans.
static func update_close_pressure(previous:int,distance:float)->int:
 if not is_finite(distance) or distance<0.0:return 0
 if distance>=6.0:return 0
 return clampi(previous+(1 if distance<2.5 else -1),0,4)

static func choose_boss_pattern(phase:int,hero_health:float,hero_max:float,previous:int,distance:float=-1.0,close_pressure:int=0,last_landed:bool=false,style:int=0)->int:
 var count:int=mini(4,clampi(phase,1,3)+1)
 var next:int=posmod(previous+1,count)
 # Decisions happen before the attack telegraph, never while an attack is live.
 # Low-health players face the readable focused strike rather than extra area pressure.
 if is_finite(hero_health) and is_finite(hero_max) and hero_max>0.0 and hero_health/hero_max<0.35:
  return 0
 # A precision specialist follows a missed attack with a delayed strike in
 # phase three, varying the timing without shortening any dodge window.
 if phase>=3 and style==1 and not last_landed and posmod(previous,count)==2:
  return 3
 # Guardian combat styles use the same fair, telegraphed attack set.
 # Style 1 favors precision at medium range; style 2 favors wider sweeps.
 if phase>=3 and style==1 and is_finite(distance) and distance>=2.5 and distance<=5.0 and posmod(previous,count)!=2:
  return 2
 if phase>=2 and style==2 and is_finite(distance) and distance>=2.5 and distance<=5.0 and posmod(previous,count)!=1:
  return 1
 # Sustained close-range pressure unlocks a precision response in phase three.
 # Use a small capped per-guardian counter, not a per-frame search.
 if phase>=3 and close_pressure>=3 and posmod(previous,count)!=2:
  return 2
 # After a successful hit, avoid relentless wide-sweep pressure; favor
 # a slower-to-read focused follow-up instead of increasing damage.
 if last_landed and phase>=2 and posmod(previous,count)!=0:
  return 0
 # Close pressure favors a wide sweep, but don't repeat it back-to-back.
 if phase>=2 and is_finite(distance) and distance>=0.0 and distance<2.5 and posmod(previous,count)!=1:
  return 1
 return next

static func surge_zone(elapsed:float,zones:int)->int:
 if zones<=0 or elapsed<0.0:return -1
 return int(floor(elapsed/SURGE_INTERVAL))%zones

static func surge_active(elapsed:float)->bool:
 return elapsed>=SURGE_INTERVAL and fposmod(elapsed,SURGE_INTERVAL)<SURGE_SECONDS

static func elite_material(zone:int,variant:String,is_boss:bool)->String:
 if is_boss or variant=="normal":return ""
 return "Moon Sigil" if zone%2==0 else "Dusk Cinder"
