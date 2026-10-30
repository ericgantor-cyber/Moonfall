extends RefCounted
## v37: deterministic world interactions, no new currency, mode, or survival clock.
const REGION_COUNT:int=6
const WILDLIFE_PER_REGION:int=2
const MAX_WILDLIFE:int=12
const SWIM_RADIUS_SQUARED:float=9.25
const SWIM_SPEED_MULTIPLIER:float=0.62
const WILDLIFE_SLOT_START:int=18
const CONTRACT_SLOT_START:int=24
const SAVED_SLOT_LIMIT:int=36
const TICK_INTERVAL:float=0.25
const RECIPES:=[
 {"name":"Riverguard Salve","job":"Alchemy","cost":{"River Reed":2,"herb":2,"Moonwater Fish":1},"result":"Riverguard Salve","power":0},
 {"name":"Aquifer Moonsteel Charm","job":"Smithing","cost":{"Moonsteel Ingot":1,"Ore Shard":3,"Moon Sigil":1},"result":"Aquifer Moonsteel Charm","power":49},
 {"name":"Marshside Provisions","job":"Cooking","cost":{"Moonwater Fish":2,"herb":2},"result":"Marshside Provisions","power":0}
]
static func swim_zone(x:float,z:float,regions:Array,y:float=0.85)->int:
 for zone in range(mini(REGION_COUNT,regions.size())):
  var at:Vector2=regions[zone]["at"]
  var sx:float=18.0 if zone%2==0 else -18.0
  var sz:float=14.0 if zone<3 else -14.0
  var dx:float=x-at.x-sx
  var dz:float=z-at.y-sz
  if dx*dx+dz*dz<=SWIM_RADIUS_SQUARED:return zone
  # A submerged side tunnel links to one handcrafted grotto in each lake.
  # Its mouth starts inside the original swim radius; above water it stays closed.
  if y< -0.24 and dx>=1.5 and dx<=6.65 and absf(dz)<=1.35:return zone
 return -1
static func wildlife_available(zone:int,day:int,sites:Dictionary)->bool:
 return zone>=0 and zone<REGION_COUNT and day>0 and int(sites.get(str(WILDLIFE_SLOT_START+zone),-1))!=day
static func contract_complete(zone:int,sites:Dictionary)->bool:
 return zone>=0 and zone<REGION_COUNT and sites.has(str(CONTRACT_SLOT_START+zone))
static func contract_cost(zone:int)->Dictionary:
 match zone%3:
  0:return {"Moonwater Fish":2,"River Reed":2}
  1:return {"Ore Shard":3,"herb":2}
  _:return {"River Reed":3,"Ore Shard":2}
static func missing(cost:Dictionary,inventory:Dictionary)->String:
 for item in cost.keys():
  if int(inventory.get(item,0))<int(cost[item]):return str(item)
 return ""
static func pay(cost:Dictionary,inventory:Dictionary)->void:
 for item in cost.keys():inventory[item]=int(inventory.get(item,0))-int(cost[item])
static func wildlife_offset(zone:int,slot:int,clock:float,fright:float)->Vector3:
 var theta:float=clock*(0.31+0.04*float(zone%3))+float(slot)*PI+float(zone)*0.37
 var wander:float=1.9+minf(2.2,maxf(0.0,fright))
 return Vector3(cos(theta)*wander,0.0,sin(theta)*wander)
static func purified(zone:int,rifts:Array,active_site:int,sites:Array)->bool:
 if zone<0 or zone>=REGION_COUNT:return false
 for rift in rifts:
  if int(rift.get("zone",-1))==zone and not bool(rift.get("cleared",false)):return false
 if active_site>=0 and active_site<sites.size():
  if int(sites[active_site].get("zone",-1))==zone:return false
 return true

static func ward_damage(raw:float,defended:bool)->float:
 return maxf(0.0,raw)*(0.95 if defended else 1.0)
