extends RefCounted
## Stage 2 rift layout and reward rules. No save-format change.
const ROOM_OFFSETS:Array[Vector3]=[Vector3(-11.0,0,-3.0),Vector3(11.0,0,-3.0),Vector3(0,0,11.0)]
const CHAMBER_FLOOR_RADIUS:float=6.2
const CHAMBER_WALL_RADIUS:float=6.48
const CHAMBER_WALL_SEGMENTS:int=20
const REGIONAL_DUNGEON_COUNT:int=5
const REGIONAL_DUNGEON_RING_RADIUS:float=43.0

# Five separate dungeon entry positions in each region, reachable from open terrain.
# Positioning alone does not add links between independent dungeons.
static func regional_dungeon_position(centre:Vector2,index:int)->Vector3:
 var angle:float=-PI*0.5+TAU*float(posmod(index,REGIONAL_DUNGEON_COUNT))/float(REGIONAL_DUNGEON_COUNT)
 return Vector3(centre.x+cos(angle)*REGIONAL_DUNGEON_RING_RADIUS,0.0,centre.y+sin(angle)*REGIONAL_DUNGEON_RING_RADIUS)
# Region dungeons are distinct from original story rifts. Each dungeon has
# fourteen physical chambers, extending radially outward from its own entrance.
# No dungeon-to-dungeon doorway, corridor or direct passage is generated.
const REGIONAL_CHAMBERS_PER_DUNGEON:int=14
const REGIONAL_ROOM_OFFSETS:Array[Vector2]=[
 Vector2(0,0),Vector2(-11,14),Vector2(11,14),Vector2(0,28),
 Vector2(-14,28),Vector2(-28,28),Vector2(-28,42),
 Vector2(-14,42),Vector2(0,42),Vector2(14,42),
 Vector2(28,42),Vector2(28,56),Vector2(14,56),Vector2(0,56)
]
static func regional_chamber_position(centre:Vector2,dungeon_id:int,stage:int)->Vector3:
 var entrance:Vector3=regional_dungeon_position(centre,dungeon_id)
 if stage<1 or stage>REGIONAL_CHAMBERS_PER_DUNGEON:return entrance
 var outward:Vector2=Vector2(entrance.x-centre.x,entrance.z-centre.y).normalized()
 var across:Vector2=Vector2(outward.y,-outward.x)
 var local:Vector2=REGIONAL_ROOM_OFFSETS[stage-1]
 return entrance+Vector3(across.x*local.x+outward.x*local.y,0,across.y*local.x+outward.y*local.y)

# Four readable, low-poly exploration landmarks; no collidable obstructions.
const REGIONAL_LANDMARK_NAMES:Array[String]=["Moon Seal","Stone Archive","Eclipse Altar","Raven Beacon"]
static func regional_landmark_type(dungeon_id:int,stage:int)->int:
 if stage<2 or stage>REGIONAL_CHAMBERS_PER_DUNGEON:return -1
 return posmod(dungeon_id*3+stage,REGIONAL_LANDMARK_NAMES.size())

static func regional_landmark_name(dungeon_id:int,stage:int)->String:
 var marker_type:int=regional_landmark_type(dungeon_id,stage)
 return REGIONAL_LANDMARK_NAMES[marker_type] if marker_type>=0 else ""

static func regional_chamber_title(stage:int)->String:
 if stage==1:return "Dungeon Threshold"
 if stage==2:return "Outer Sanctum"
 if stage==3:return "Guardian Crossing"
 if stage>=4 and stage<=14:return annex_title(stage)
 return "Unknown Chamber"

# Physical exploration annexes, stages 4–14, branch from the existing Guardian Heart.
# Each step is 14m, exceeding the 12.4m diameter of enlarged floors.
const EXPLORATION_OFFSETS:Array[Vector3]=[
 Vector3(0,0,25),Vector3(-14,0,25),Vector3(-28,0,25),
 Vector3(-28,0,39),Vector3(-14,0,39),Vector3(0,0,39),
 Vector3(14,0,39),Vector3(28,0,39),Vector3(28,0,53),
 Vector3(14,0,53),Vector3(0,0,53)
]
const ANNEX_TITLES:Array[String]=[
 "Moonlit Archive","Cinder Reliquary","Raven Watch","Lost Observatory",
 "Whispering Vault","Twilight Garden","Ember Crucible","Fallen Library",
 "Astral Gallery","Hollow Shrine","Eclipse Threshold"
]
const ANNEX_SIGILS:Array[String]=["archive","embers","watch","stars","echo","garden","forge","scroll","astral","shrine","eclipse"]
const ROOM_NAMES:Array[String]=["Outer Sanctum","Moonbound Passage","Guardian Heart"]
const STAGE_LOOT_NAMES:Array[String]=["Ravenwood Vow","Ashen Moon Vow","Bloodmoon Vow"]

static func room_position(anchor:Vector3,stage:int)->Vector3:
 return anchor+ROOM_OFFSETS[clampi(stage,1,3)-1]

# Keep the actual walkway approaches open when placing solid chamber walls.
# Each room has two connected neighbours; no new phantom exits.
static func wall_segment_open(stage:int,angle:float)->bool:
 if stage<1 or stage>3 or not is_finite(angle):return true
 var here:Vector3=ROOM_OFFSETS[stage-1]
 for other_stage in range(1,4):
  if other_stage==stage:continue
  var offset:Vector3=ROOM_OFFSETS[other_stage-1]-here
  var doorway_angle:float=atan2(offset.z,offset.x)
  if absf(wrapf(angle-doorway_angle,-PI,PI))<0.70:return true
 return false

static func exploration_position(anchor:Vector3,stage:int)->Vector3:
 if stage<4 or stage>14:return anchor
 return anchor+EXPLORATION_OFFSETS[stage-4]

static func annex_title(stage:int)->String:
 if stage<4 or stage>14:return "Unknown Chamber"
 return ANNEX_TITLES[stage-4]

static func annex_sigil(stage:int)->String:
 if stage<4 or stage>14:return ""
 return ANNEX_SIGILS[stage-4]

static func annex_tint(stage:int)->Color:
 if stage<4 or stage>14:return Color(0.5,0.5,0.6)
 var hue:float=fmod(float(stage-4)*0.137,1.0)
 return Color.from_hsv(hue,0.38,0.85)

static func exploration_previous(anchor:Vector3,stage:int)->Vector3:
 if stage<=4:return room_position(anchor,3)
 return exploration_position(anchor,stage-1)

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
