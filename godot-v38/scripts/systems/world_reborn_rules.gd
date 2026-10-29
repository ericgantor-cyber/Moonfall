extends RefCounted
## Bounded, deterministic visual streaming and reputation rules for v32.0.
const CLOSE_DISTANCE_SQ:float=44.0*44.0
const FAR_DISTANCE_SQ:float=60.0*60.0
const WARDEN_REACH_SQ:float=3.5*3.5

static func visible_at(distance_sq:float,was_visible:bool,low_quality:bool)->bool:
 if low_quality:return false
 return distance_sq<(FAR_DISTANCE_SQ if was_visible else CLOSE_DISTANCE_SQ)

static func standing(contracts:Array,rift_count:int)->String:
 var score:int=contracts.size()+maxi(0,rift_count)*2
 if score>=10:return "Champion of the Six Realms"
 if score>=6:return "Moonward Protector"
 if score>=3:return "Trusted Wayfarer"
 return "Unknown Traveler"

static func contract_available(zone:int,complete:Array,active:int)->bool:
 return zone>=0 and zone<6 and zone not in complete and active<0
