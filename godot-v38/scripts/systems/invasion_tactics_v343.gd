extends RefCounted
## Three HP-driven phases, deterministic and bounded. No survival stopwatch.
const MAX_MARKERS:int=2
const EXPOSE_AFTER_DODGES:int=2
const EXPOSED_HIT_MULTIPLIER:float=1.55
const CLEAN_SEAL_BONUS:int=1

static func commander_phase(hp:float,max_hp:float)->int:
 if max_hp<=0.0:return 1
 var fraction:float=clampf(hp/max_hp,0.0,1.0)
 if fraction<=0.33:return 3
 if fraction<=0.66:return 2
 return 1

static func phase_title(phase:int)->String:
 return "AWAKENING" if phase==1 else "ENRAGED" if phase==2 else "ECLIPSE FURY"

static func phase_color(site:int,phase:int)->Color:
 var base:Color=Color("75cfff") if site==0 else Color("ffad51") if site==1 else Color("c47eff")
 return base.lerp(Color("fff5fa"),float(clampi(phase,1,3)-1)*0.28)

static func target_positions(site:int,player_at:Vector3,pattern:int,phase:int)->Array[Vector3]:
 var pivot:Vector3=Vector3(player_at.x,0.14,player_at.z)
 var turn:float=1.0 if pattern%2==0 else -1.0
 if site==0:
  # Lightning cross: second mark appears only when furious.
  if phase>=3:return [pivot+Vector3(turn*2.2,0.0,0.0),pivot+Vector3(0.0,0.0,turn*2.2)]
  return [pivot+Vector3(turn*1.3,0.0,0.0)]
 if site==1:
  # Split fire lanes alternate direction with phase.
  if phase>=2:return [pivot+Vector3(0.0,0.0,-2.6*turn),pivot+Vector3(0.0,0.0,2.6*turn)]
  return [pivot+Vector3(-2.1,0.0,0.0),pivot+Vector3(2.1,0.0,0.0)]
 # Void echoes: a second mark appears only during Eclipse Fury.
 if phase>=3:return [pivot+Vector3(-2.9*turn,0.0,0.0),pivot+Vector3(2.9*turn,0.0,0.0)]
 return [pivot+Vector3(0.0,0.0,turn*1.1)]

static func marker_count(site:int,phase:int)->int:
 return clampi((2 if site==1 or phase>=3 else 1),1,MAX_MARKERS)

static func exposed_damage(normal:float,exposed:bool)->float:
 return normal*EXPOSED_HIT_MULTIPLIER if exposed else normal

static func perfect_seal(dodges:int,real_hits:int)->bool:
 return dodges>=EXPOSE_AFTER_DODGES and real_hits==0
