extends RefCounted
## Moonfall v30.4: isolated inventory/crafting/loot rules. No scene or save references.
const BAG_START_CAPACITY := 100
const BAG_MAX_CAPACITY := 500
const BAG_UPGRADE_STEP := 50
const VAULT_LIMIT := 1000
const MATERIAL_STACK_SIZE := 999
const BAG_PAGE_SIZE := 20
const MATERIAL_LIMIT := 99999

static func valid_capacity(value:int)->int:
 # Migration-safe: reject corrupted sizes, but do not shrink upgraded bags.
 return clampi(value,BAG_START_CAPACITY,BAG_MAX_CAPACITY)

static func upgrade_cost(capacity:int)->int:
 if capacity>=BAG_MAX_CAPACITY:return 0
 return 100 + maxi(0,(valid_capacity(capacity)-BAG_START_CAPACITY)/BAG_UPGRADE_STEP)*150

static func can_store_gear(bag_count:int,capacity:int=BAG_START_CAPACITY)->bool:
 return bag_count < valid_capacity(capacity)

static func material_stack_counts(quantity:int)->Array[int]:
 var result:Array[int]=[]
 var remaining:=clampi(quantity,0,MATERIAL_LIMIT)
 while remaining>0:
  var amount:=mini(remaining,MATERIAL_STACK_SIZE)
  result.append(amount)
  remaining-=amount
 return result

static func transfer_gear(source:Array,destination:Array,index:int,capacity:int)->bool:
 if index<0 or index>=source.size() or destination.size()>=capacity:return false
 var item:Variant=source[index]
 if not (item is Dictionary):return false
 destination.append(item.duplicate(true))
 source.remove_at(index)
 return true

static func matching_gear_indices(items:Array,search:String,filter_mode:String,sort_mode:String)->Array[int]:
 var indices:Array[int]=[]
 var needle:=search.strip_edges().to_lower().substr(0,80)
 for i in range(items.size()):
  var item:Variant=items[i]
  if not (item is Dictionary):continue
  var name:=str(item.get("name",""))
  var rarity:=str(item.get("rarity","Common"))
  if filter_mode!="All" and rarity!=filter_mode:continue
  if needle!="" and not (needle in (name+" "+rarity+" "+str(item.get("affix",""))).to_lower()):continue
  indices.append(i)
 if sort_mode=="Power":
  indices.sort_custom(func(a:int,b:int)->bool:
   if int(items[a].get("power",0))!=int(items[b].get("power",0)):
    return int(items[a].get("power",0))>int(items[b].get("power",0))
   return a<b)
 elif sort_mode=="Rarity":
  var weight:Dictionary={"Common":0,"Uncommon":1,"Rare":2,"Crafted":3,"Epic":4,"Legendary":5,"Mythic":6}
  indices.sort_custom(func(a:int,b:int)->bool:
   var wa:int=int(weight.get(str(items[a].get("rarity","Common")),0))
   var wb:int=int(weight.get(str(items[b].get("rarity","Common")),0))
   if wa!=wb:return wa>wb
   return a<b)
 elif sort_mode=="Name":
  indices.sort_custom(func(a:int,b:int)->bool:
   var name_a:=str(items[a].get("name","")).to_lower()
   var name_b:=str(items[b].get("name","")).to_lower()
   if name_a!=name_b:return name_a<name_b
   return a<b)
 return indices

static func page_count(count:int,page_size:int=BAG_PAGE_SIZE)->int:
 return maxi(1,ceili(float(count)/float(maxi(page_size,1))))

static func add_material(materials:Dictionary,name:String,amount:int)->void:
 materials[name]=clampi(int(materials.get(name,0))+amount,0,MATERIAL_LIMIT)

static func consume_one(materials:Dictionary,name:String)->bool:
 if int(materials.get(name,0))<1:return false
 materials[name]=int(materials[name])-1
 return true

static func craft_issue(recipe:Dictionary,materials:Dictionary,bag_count:int,capacity:int=BAG_START_CAPACITY)->String:
 var costs:Dictionary=recipe.get("cost",{})
 for key in costs.keys():
  if int(materials.get(str(key),0))<int(costs[key]):return "Missing "+str(key)+" for "+str(recipe.get("name","recipe"))
 if int(recipe.get("power",0))>0 and not can_store_gear(bag_count,capacity):return "Bag full."
 return ""

static func craft_commit(recipe:Dictionary,materials:Dictionary)->Dictionary:
 # Caller validates with craft_issue first; each recipe has a fixed, known cost.
 var costs:Dictionary=recipe.get("cost",{})
 for key in costs.keys():
  materials[str(key)]=int(materials.get(str(key),0))-int(costs[key])
 if int(recipe.get("power",0))>0:
  return {"gear":{"name":recipe["result"],"rarity":"Crafted","power":recipe["power"]},"material":""}
 return {"gear":{},"material":str(recipe["result"])}

static func equip_copy(items:Array,index:int)->Dictionary:
 if index<0 or index>=items.size():return {}
 var item:Variant=items[index]
 return item.duplicate(true) if item is Dictionary else {}

static func equipment_power(equipped:Dictionary)->int:
 return clampi(int(equipped.get("power",0)),0,250)

static func first_discardable_drop(drops:Array)->int:
 for i in range(drops.size()):
  var drop:Variant=drops[i]
  if drop is Dictionary:
   var data:Variant=drop.get("data",{})
   if data is Dictionary and str(data.get("rarity","Common")) in ["Common","Uncommon"]:return i
 return -1
