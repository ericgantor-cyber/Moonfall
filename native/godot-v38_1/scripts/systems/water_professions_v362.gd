extends RefCounted
## v36.2 finite resource nodes and bounded, persistent crafting. No new currency.
const JOBS:=["Fishing","Herbalism","Mining","Cooking","Alchemy","Smithing"]
const RECIPES:=[
 {"name":"Moonfish Stew","job":"Cooking","cost":{"Moonwater Fish":2,"herb":1},"result":"Moonfish Stew","power":0},
 {"name":"Moonwater Tonic","job":"Alchemy","cost":{"River Reed":2,"herb":2},"result":"Moonwater Tonic","power":0},
 {"name":"Moonsteel Ingot","job":"Smithing","cost":{"Ore Shard":3,"Dusk Cinder":1},"result":"Moonsteel Ingot","power":0},
 {"name":"Moonsteel Edge","job":"Smithing","cost":{"Moonsteel Ingot":2,"Moon Sigil":1},"result":"Moonsteel Edge","power":38}
]
static func can_gather(site_id:int,today:int,harvested_days:Dictionary)->bool:
 return site_id>=0 and site_id<18 and today>0 and int(harvested_days.get(str(site_id),-1))!=today
static func harvest_kind(slot:int)->String:
 match slot:
  0:return "Fishing"
  1:return "Herbalism"
  2:return "Mining"
 return ""
static func gather_yield(kind:String,skill:int,roll:int)->Dictionary:
 var bonus:int=1 if skill>=75 else 0
 match kind:
  "Fishing":return {"Moonwater Fish":1+abs(roll)%2+bonus}
  "Herbalism":return {"River Reed":1+abs(roll)%2+bonus,"herb":1}
  "Mining":return {"Ore Shard":2+abs(roll)%2+bonus}
 return {}
static func recipe_issue(recipe:Dictionary,materials:Dictionary,inventory_count:int,bag_capacity:int)->String:
 for ingredient in (recipe["cost"] as Dictionary).keys():
  if int(materials.get(ingredient,0))<int(recipe["cost"][ingredient]):return "Missing "+str(ingredient)
 if int(recipe.get("power",0))>0 and inventory_count>=bag_capacity:return "Equipment bag full"
 return ""
static func consume_recipe(recipe:Dictionary,materials:Dictionary)->void:
 ## Only call after recipe_issue() returns an empty string.
 for ingredient in (recipe["cost"] as Dictionary).keys():
  materials[ingredient]=int(materials.get(ingredient,0))-int(recipe["cost"][ingredient])
static func initial_water_offset(zone:int)->Vector2:
 ## All features off paved roads and within the world bounds.
 var x:float=18.0 if zone%2==0 else -18.0
 var z:float=14.0 if zone<3 else -14.0
 return Vector2(x,z)
