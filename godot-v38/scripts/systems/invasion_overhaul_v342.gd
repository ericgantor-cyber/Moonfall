extends RefCounted
## v34.2 elemental invasion balance, constant bounded costs, no timers for survival.
const MAX_WARNING_MARKERS:int=2
const DODGE_RESONANCE:int=18

static func marker_count(site:int)->int:
 return 2 if site==1 else 1

static func marker_radius(site:int)->float:
 return 2.4 if site==0 else 2.5 if site==1 else 3.3

static func inside_warning(site:int,player_at:Vector3,mark_at:Vector3)->bool:
 var dx:float=player_at.x-mark_at.x
 var dz:float=player_at.z-mark_at.z
 var radius:float=marker_radius(site)
 return dx*dx+dz*dz<=radius*radius

static func target_positions(site:int,player_at:Vector3,pattern:int)->Array[Vector3]:
 var at:Vector3=Vector3(player_at.x,0.14,player_at.z)
 if site==0:
  return [at+Vector3((1.4 if pattern%2==0 else -1.4),0,0)]
 if site==1:
  return [at+Vector3(-2.1,0,0),at+Vector3(2.1,0,0)]
 return [at+Vector3(0,0,(1.1 if pattern%2==0 else -1.1))]

static func warning_color(site:int)->Color:
 return Color("6edbff") if site==0 else Color("ff772c") if site==1 else Color("c354f9")

static func warning_seconds(site:int)->float:
 return 1.35 if site==0 else 1.55 if site==1 else 1.85

static func rest_seconds(site:int)->float:
 return 5.0 if site==0 else 5.5 if site==1 else 6.0

static func strike_damage(site:int)->float:
 return 25.0 if site==0 else 30.0 if site==1 else 34.0

static func bonus_material(site:int)->String:
 return "Moon Sigil" if site==0 else "Dusk Cinder" if site==1 else "Elder Rootstone"

static func health_scale(site:int,wave:int)->float:
 return 1.05+0.05*float(clampi(site,0,2))+0.04*float(clampi(wave,1,3)-1)

static func lunge_cooldown(site:int)->float:
 return 1.7 if site==0 else 2.2 if site==1 else 2.7

static func commander_title(site:int)->String:
 return "TEMPEST WARDEN" if site==0 else "CINDER COLOSSUS" if site==1 else "THE HOLLOW ORACLE"

static func wave_notice(site:int,wave:int)->String:
 var faction:String="STORM" if site==0 else "FIRE" if site==1 else "VOID"
 return faction+" INVASION: "+("COMMANDER EMERGES" if wave==3 else "WAVE "+str(wave)+" / 2")

static func warning_message(site:int)->String:
 return "LIGHTNING MARK! DODGE THE CIRCLE!" if site==0 else "FIRE SPLIT! ESCAPE THE TWO MARKS!" if site==1 else "VOID PULSE! EVADE THE VIOLET RING!"
