extends RefCounted
## v36.3: Deterministic, bounded living-world rules. No new save schema, currency,
## survival countdown, or construction gameplay.
const WATER_ZONE_COUNT:int=6
const FLORA_PER_ZONE:int=5
const FISH_PER_ZONE:int=2
const UPDATE_SECONDS:float=0.25
const MAX_WARD_SECONDS:float=45.0
const RECIPES:=[
 {"name":"Witchwood Trail Rations","job":"Cooking","cost":{"Moonwater Fish":1,"River Reed":1,"herb":1},"result":"Witchwood Trail Rations","power":0},
 {"name":"Moonbloom Ward Tonic","job":"Alchemy","cost":{"River Reed":2,"herb":2,"Elder Rootstone":1},"result":"Moonbloom Ward Tonic","power":0},
 {"name":"Tideforged Blade","job":"Smithing","cost":{"Ore Shard":4,"Moonsteel Ingot":1},"result":"Tideforged Blade","power":46},
]
static func is_tainted(zone:int,rifts:Array,active_invasion_site:int,invasion_sites:Array)->bool:
 if zone<0 or zone>=WATER_ZONE_COUNT:return false
 for entry in rifts:
  if int(entry.get("zone",-1))==zone and not bool(entry.get("cleared",false)):
   return true
 if active_invasion_site>=0 and active_invasion_site<invasion_sites.size():
  return int(invasion_sites[active_invasion_site].get("zone",-1))==zone
 return false
static func flora_response(distance_squared:float,is_tainted_region:bool)->float:
 ## 0=rest, 1=gentle proximity response; taint suppresses bioluminescence.
 return clampf((6.5-distance_squared**0.5)/4.5,0.0,1.0)*(0.35 if is_tainted_region else 1.0)
static func fish_escape_radius(distance_squared:float)->float:
 return 3.1 if distance_squared<16.0 else 1.9
static func ward_damage(raw_damage:float,ward_seconds:float,inside_corruption:bool)->float:
 return maxf(0.0,raw_damage)*(0.8 if ward_seconds>0.0 and inside_corruption else 1.0)
static func recipe_issue(recipe:Dictionary,materials:Dictionary,inventory_count:int,capacity:int)->String:
 for item in (recipe["cost"] as Dictionary).keys():
  if int(materials.get(item,0))<int(recipe["cost"][item]):return "Missing "+str(item)
 if int(recipe["power"])>0 and inventory_count>=capacity:return "Equipment bag full"
 return ""
static func spend(recipe:Dictionary,materials:Dictionary)->void:
 ## Call only after recipe_issue returns empty. No partial deduction.
 for item in (recipe["cost"] as Dictionary).keys():
  materials[item]=int(materials.get(item,0))-int(recipe["cost"][item])
