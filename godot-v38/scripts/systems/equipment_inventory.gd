extends RefCounted
## Data-driven loot, inventory, crafting and equipment state.
## No artwork copies, runtime singleton, or per-frame inventory work.
const CATALOG = preload("res://scripts/systems/equipment_catalog.gd")
const RNG = preload("res://scripts/systems/equipment_rng.gd")
const SLOTS:Array[String] = ["weapon","offhand","helmet","chest","gloves","legs","boots","ring","amulet","cloak"]

static func new_state()->Dictionary:
 var equipped:Dictionary={}
 for slot in SLOTS:equipped[slot]=""
 return {"items":{},"equipped":equipped,"materials":{},"gold":0,"loot_rolls":[],"loot_pity":0}

static func slot_for(item:Dictionary)->String:
 match str(item.get("category","")):
  "weapon":return "weapon"
  "offhand":return "offhand"
  "armor":
   var family:String=str(item.get("family","")).to_lower()
   for slot in ["helmet","chest","gloves","legs","boots"]:
    if family.ends_with(" "+slot):return slot
  "accessory":return str(item.get("family","")).to_lower()
 return ""

static func add_item(state:Dictionary,item_id:String,count:int=1)->bool:
 if count<=0 or CATALOG.get_item(item_id).is_empty():return false
 var items:Dictionary=state.get("items",{})
 items[item_id]=mini(9999,maxi(0,int(items.get(item_id,0)))+count)
 state["items"]=items
 return true

static func equip(state:Dictionary,item_id:String,hero_level:int)->bool:
 var item:Dictionary=CATALOG.get_item(item_id)
 if item.is_empty() or hero_level<int(item.get("level_required",1)):return false
 if int(state.get("items",{}).get(item_id,0))<1:return false
 var slot:String=slot_for(item)
 if slot=="" or not SLOTS.has(slot):return false
 var slots:Dictionary=state.get("equipped",{})
 slots[slot]=item_id
 state["equipped"]=slots
 return true

static func add_material(state:Dictionary,material:String,count:int)->bool:
 if material=="" or count<=0:return false
 var materials:Dictionary=state.get("materials",{})
 materials[material]=mini(999999,maxi(0,int(materials.get(material,0)))+count)
 state["materials"]=materials
 return true

static func craft(state:Dictionary,item_id:String,hero_level:int)->bool:
 var item:Dictionary=CATALOG.get_item(item_id)
 if item.is_empty() or hero_level<int(item["level_required"]):return false
 var material:String=str(item["name"]).split(" ")[0]
 var cost:int=2+int(item["tier"])*3
 var materials:Dictionary=state.get("materials",{})
 if int(materials.get(material,0))<cost:return false
 materials[material]=int(materials[material])-cost
 state["materials"]=materials
 return add_item(state,item_id)

static func loot_enemy(state:Dictionary,hero_level:int,seed:int)->Dictionary:
 var item:Dictionary=CATALOG.loot_for_level(hero_level,seed)
 if not item.is_empty():
  add_item(state,str(item["id"]))
  var rarity:Dictionary=RNG.roll(item,seed+int(state.get("loot_pity",0))*9973,0,int(state.get("loot_pity",0)))
  state["loot_pity"]=RNG.next_pity(int(state.get("loot_pity",0)),int(rarity["rarity_index"]))
  var rolls:Array=state.get("loot_rolls",[])
  rolls.append(rarity)
  if rolls.size()>250:rolls.pop_front()
  state["loot_rolls"]=rolls
  var drop:Dictionary=item.duplicate(true)
  drop["roll"]=rarity
  return drop
 return item

static func totals(state:Dictionary)->Dictionary:
 var stats:Dictionary={"weapon_power":0,"armor":0,"magic":0,"vitality":0,"haste":0,"crit":0,"ward":0,"elemental":0}
 var slots:Dictionary=state.get("equipped",{})
 for slot in SLOTS:
  var item:Dictionary=CATALOG.get_item(str(slots.get(slot,"")))
  if item.is_empty():continue
  var power:int=int(item["power"])
  match str(item["category"]):
   "weapon":stats["weapon_power"]+=power
   "armor":stats["armor"]+=power
   "offhand","accessory":stats["magic"]+=power
  # Apply only a matching owned item's highest rolled bonus.
  var best_roll:Dictionary={}
  for roll in state.get("loot_rolls",[]):
   if roll is Dictionary and str(roll.get("item_id",""))==str(item["id"]):
    if int(roll.get("rarity_index",0))>int(best_roll.get("rarity_index",-1)):
     best_roll=roll
  var bonus:int=clampi(int(best_roll.get("power_bonus",0)),0,12)
  if str(item["category"])=="weapon":stats["weapon_power"]+=bonus
  elif str(item["category"])=="armor":stats["armor"]+=bonus
  else:stats["magic"]+=bonus
  for affix in best_roll.get("affixes",[]):
   if not affix is Dictionary:continue
   var value:int=clampi(int(affix.get("value",0)),0,30)
   match str(affix.get("stat","")):
    "Power":stats["weapon_power"]+=value
    "Vitality":stats["vitality"]+=value
    "Haste":stats["haste"]+=value
    "Crit":stats["crit"]+=value
    "Ward":stats["ward"]+=value
    "Elemental":stats["elemental"]+=value
 return stats

static func normalize(saved:Dictionary)->Dictionary:
 var state:Dictionary=new_state()
 var prior_items:Variant=saved.get("items",{})
 if prior_items is Dictionary:
  for id in prior_items:
   var count:int=clampi(int(prior_items[id]),0,9999)
   if count>0 and not CATALOG.get_item(str(id)).is_empty():state["items"][str(id)]=count
 var prior_materials:Variant=saved.get("materials",{})
 if prior_materials is Dictionary:
  for material in prior_materials:
   if str(material)!="" and int(prior_materials[material])>0:
    state["materials"][str(material)]=clampi(int(prior_materials[material]),0,999999)
 state["gold"]=clampi(int(saved.get("gold",0)),0,99999999)
 state["loot_pity"]=clampi(int(saved.get("loot_pity",0)),0,80)
 var prior_rolls:Variant=saved.get("loot_rolls",[])
 if prior_rolls is Array:
  for entry in prior_rolls.slice(-250):
   if entry is Dictionary and not CATALOG.get_item(str(entry.get("item_id",""))).is_empty():
    state["loot_rolls"].append(entry.duplicate(true))
 var prior_slots:Variant=saved.get("equipped",{})
 if prior_slots is Dictionary:
  for slot in SLOTS:
   var id:String=str(prior_slots.get(slot,""))
   var item:Dictionary=CATALOG.get_item(id)
   if not item.is_empty() and slot_for(item)==slot and state["items"].has(id):
    state["equipped"][slot]=id
 return state
