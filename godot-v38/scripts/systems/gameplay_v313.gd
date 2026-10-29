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

static func choose_boss_pattern(phase:int,hero_health:float,hero_max:float,previous:int)->int:
 if phase<=1:return 0
 if hero_max>0.0 and hero_health/hero_max<0.35:return 0 # a dodgeable single-target strike, never unavoidable damage
 return (previous+1)%mini(phase,3)

static func surge_zone(elapsed:float,zones:int)->int:
 if zones<=0 or elapsed<0.0:return -1
 return int(floor(elapsed/SURGE_INTERVAL))%zones

static func surge_active(elapsed:float)->bool:
 return elapsed>=SURGE_INTERVAL and fposmod(elapsed,SURGE_INTERVAL)<SURGE_SECONDS

static func elite_material(zone:int,variant:String,is_boss:bool)->String:
 if is_boss or variant=="normal":return ""
 return "Moon Sigil" if zone%2==0 else "Dusk Cinder"
