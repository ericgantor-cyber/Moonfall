extends RefCounted
## Pure v38 UI and gameplay-safety rules. No new save fields, currencies or modes.
const UPDATE_PERIOD:float=0.25
const LOW_AIR:float=25.0
const CRITICAL_AIR:float=10.0
const LOW_HEALTH:float=0.35

static func air_urgency(air:float,underwater:bool)->int:
 if not underwater:return 0
 if air<=CRITICAL_AIR:return 2
 if air<=LOW_AIR:return 1
 return 0

static func threat_status(air:float,underwater:bool,threat:bool,hp:float,max_hp:float)->String:
 if air_urgency(air,underwater)==2:return "CRITICAL AIR • ASCEND NOW"
 if air_urgency(air,underwater)==1:return "LOW AIR • RETURN TO SURFACE"
 if threat:return "COMBAT • WATCH ATTACK CUES"
 if max_hp>0.0 and hp/max_hp<=LOW_HEALTH:return "LOW HEALTH • HEAL WHEN SAFE"
 if underwater:return "EXPLORING UNDERWATER"
 return "EXPLORING"

static func safe_label(region:String,hp:float,max_hp:float,oxygen:float,underwater:bool,depth:float,threat:bool)->String:
 var hp_percent:int=0 if max_hp<=0.0 else clampi(roundi(100.0*hp/max_hp),0,100)
 var result:String="REGION: "+region+"\nHP: "+str(hp_percent)+"%"
 if underwater:result+="  •  AIR: "+str(clampi(ceili(oxygen),0,100))+"%  •  DEPTH: "+str(snappedf(depth,0.1))
 return result+"\n"+threat_status(oxygen,underwater,threat,hp,max_hp)
