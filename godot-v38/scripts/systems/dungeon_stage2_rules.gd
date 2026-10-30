extends RefCounted
## Stage 2 rift layout and reward rules. No save-format change.
const ROOM_OFFSETS:Array[Vector3]=[Vector3(-11.0,0,-3.0),Vector3(11.0,0,-3.0),Vector3(0,0,11.0)]
const ROOM_NAMES:Array[String]=["Outer Sanctum","Moonbound Passage","Guardian Heart"]
const STAGE_LOOT_NAMES:Array[String]=["Ravenwood Vow","Ashen Moon Vow","Bloodmoon Vow"]

static func room_position(anchor:Vector3,stage:int)->Vector3:
 return anchor+ROOM_OFFSETS[clampi(stage,1,3)-1]

static func enemy_position(anchor:Vector3,stage:int,slot:int)->Vector3:
 var center:Vector3=room_position(anchor,stage)
 var angle:float=float(posmod(slot,3))*TAU/3.0
 return center+Vector3(cos(angle)*2.6,0.0,sin(angle)*2.6)

static func chamber_title(stage:int)->String:
 return ROOM_NAMES[clampi(stage,1,3)-1]

static func room_visible(distance_squared:float,graphics_low:bool)->bool:
 return not graphics_low and distance_squared<75.0*75.0

static func final_champion_health(normal_hp:float,stage:int,slot:int)->float:
 return normal_hp*1.65 if stage==3 and slot==2 else normal_hp

static func named_relic(site:int,zone:int,affix:String)->Dictionary:
 var idx:int=clampi(site,0,STAGE_LOOT_NAMES.size()-1)
 return {"name":STAGE_LOOT_NAMES[idx]+" Signet","rarity":"Legendary","power":mini(250,48+zone*12),"affix":affix,"affix_value":0.11+0.015*float(idx)}
