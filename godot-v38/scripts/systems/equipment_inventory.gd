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

static func _roll_strength(roll:Dictionary)->int:
 # Count only bounded bonuses that contribute to a hero stat.
 var strength:int=clampi(int(roll.get("power_bonus",0)),0,12)
 var used:Dictionary={}
 var affixes:Variant=roll.get("affixes",[])
 if not affixes is Array:return strength
 for affix in affixes:
  if not affix is Dictionary:continue
  var stat:String=str(affix.get("stat",""))
  if not RNG.AFFIXES.has(stat) or used.has(stat):continue
  used[stat]=true
  strength+=clampi(int(affix.get("value",0)),0,30)
 return strength

static func _best_owned_rolls(state:Dictionary)->Dictionary:
 # Scan bounded roll history once, not once for every equipment slot.
 # A rolled bonus never applies unless its corresponding item is owned.
 var owned:Dictionary=state.get("items",{})
 var best:Dictionary={}
 var history:Variant=state.get("loot_rolls",[])
 if not history is Array:return best
 for roll in history:
  if not roll is Dictionary:continue
  var item_id:String=str(roll.get("item_id",""))
  if int(owned.get(item_id,0))<=0 or CATALOG.get_item(item_id).is_empty():continue
  var rarity:int=clampi(int(roll.get("rarity_index",0)),0,4)
  var previous:Dictionary=best.get(item_id,{})
  if previous.is_empty() or rarity>clampi(int(previous.get("rarity_index",0)),0,4) or (rarity==clampi(int(previous.get("rarity_index",0)),0,4) and _roll_strength(roll)>_roll_strength(previous)):
   best[item_id]=roll
 return best

static func totals(state:Dictionary)->Dictionary:
 var stats:Dictionary={"weapon_power":0,"armor":0,"magic":0,"vitality":0,"haste":0,"crit":0,"ward":0,"elemental":0}
 var slots:Dictionary=state.get("equipped",{})
 var owned:Dictionary=state.get("items",{})
 var best_rolls:Dictionary=_best_owned_rolls(state)
 for slot in SLOTS:
  var id:String=str(slots.get(slot,""))
  if int(owned.get(id,0))<=0:continue
  var item:Dictionary=CATALOG.get_item(id)
  if item.is_empty() or slot_for(item)!=slot:continue
  var power:int=int(item["power"])
  match str(item["category"]):
   "weapon":stats["weapon_power"]+=power
   "armor":stats["armor"]+=power
   "offhand","accessory":stats["magic"]+=power
  var best_roll:Dictionary=best_rolls.get(id,{})
  var bonus:int=clampi(int(best_roll.get("power_bonus",0)),0,12)
  if str(item["category"])=="weapon":stats["weapon_power"]+=bonus
  elif str(item["category"])=="armor":stats["armor"]+=bonus
  else:stats["magic"]+=bonus
  # Duplicate or unknown affixes in corrupted saves cannot stack bonuses.
  var used_affixes:Dictionary={}
  var affixes:Variant=best_roll.get("affixes",[])
  if not affixes is Array:continue
  for affix in affixes:
   if not affix is Dictionary:continue
   var kind:String=str(affix.get("stat",""))
   if not RNG.AFFIXES.has(kind) or used_affixes.has(kind):continue
   used_affixes[kind]=true
   var value:int=clampi(int(affix.get("value",0)),0,30)
   match kind:
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
