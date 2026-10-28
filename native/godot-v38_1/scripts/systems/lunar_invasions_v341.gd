extends RefCounted
## Deterministic, event-driven invasion balance. No survival time mechanic.
const REGIONS:Array[int]=[1,3,5]
const PRESSURE_TRIGGER:int=4
const SOLDIER_SLOTS:int=3
const WAVES:int=3
const ACTIVE_INVADERS_MAX:int=3

static func add_pressure(old:int,eligible:bool)->int:
 return clampi(old+(1 if eligible else 0),0,PRESSURE_TRIGGER)

static func wave_size(wave:int)->int:
 return 2 if wave==1 else SOLDIER_SLOTS if wave==2 else 1 if wave==3 else 0

static func wave_health(wave:int,boss:bool)->float:
 return 3.1 if boss else 1.15+0.35*float(clampi(wave,1,2))

static func event_name(site:int)->String:
 match site:
  0:return "STORMBREACH"
  1:return "EMBERFALL"
  2:return "VOIDTIDE"
 return "LUNAR INVASION"

static func commander_name(site:int)->String:
 match site:
  0:return "STORMBREACH COMMANDER"
  1:return "EMBERFALL COMMANDER"
  2:return "VOIDTIDE COMMANDER"
 return "INVASION COMMANDER"

static func invasion_anchor(rift_anchor:Vector3,site:int)->Vector3:
 # Confined to existing playable land, away from the persistent dungeon marker.
 return rift_anchor+(Vector3(-13,0,-15) if site==2 else Vector3(15,0,-14))
