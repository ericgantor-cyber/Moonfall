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
 var value:Variant=state.get("items",{})
 if not value is Dictionary:return false
 var items:Dictionary=value
 var owned:int=_safe_roll_number(items.get(item_id,0),9999)
 if owned>=9999:return false
 # Saturate before addition: an untrusted large count must not overflow int64.
 items[item_id]=owned+mini(count,9999-owned)
 state["items"]=items
 return true

static func equip(state:Dictionary,item_id:String,hero_level:int)->bool:
 var item:Dictionary=CATALOG.get_item(item_id)
 if item.is_empty() or hero_level<int(item.get("level_required",1)):return false
 var owned_value:Variant=state.get("items",{})
 if not owned_value is Dictionary or _safe_roll_number(owned_value.get(item_id,0),9999)<1:return false
 var slot:String=slot_for(item)
 if slot=="" or not SLOTS.has(slot):return false
 var slots_value:Variant=state.get("equipped",{})
 if not slots_value is Dictionary:return false
 var slots:Dictionary=slots_value
 slots[slot]=item_id
 state["equipped"]=slots
 return true

static func add_material(state:Dictionary,material:String,count:int)->bool:
 if material=="" or count<=0:return false
 var value:Variant=state.get("materials",{})
 if not value is Dictionary:return false
 var materials:Dictionary=value
 var available:int=_safe_roll_number(materials.get(material,0),999999)
 if available>=999999:return false
 # Bound the increment first to avoid signed integer overflow.
 materials[material]=available+mini(count,999999-available)
 state["materials"]=materials
 return true

static func craft(state:Dictionary,item_id:String,hero_level:int)->bool:
 var item:Dictionary=CATALOG.get_item(item_id)
 if item.is_empty() or hero_level<int(item["level_required"]):return false
 var material:String=str(item["name"]).split(" ")[0]
 var cost:int=2+int(item["tier"])*3
 var materials_value:Variant=state.get("materials",{})
 var items_value:Variant=state.get("items",{})
 if not materials_value is Dictionary or not items_value is Dictionary:return false
 var materials:Dictionary=materials_value
 var items:Dictionary=items_value
 var available:int=_safe_roll_number(materials.get(material,0),999999)
 var owned:int=_safe_roll_number(items.get(item_id,0),9999)
 # Crafting is atomic: never consume materials when the item stack is full.
 if available<cost or owned>=9999:return false
 if not add_item(state,item_id):return false
 materials[material]=available-cost
 state["materials"]=materials
 return true

static func loot_enemy(state:Dictionary,hero_level:int,seed:int)->Dictionary:
 var item:Dictionary=CATALOG.loot_for_level(hero_level,seed)
 if item.is_empty():return {}
 # Validate all mutable reward fields before granting the item. A malformed
 # legacy save must not grant an item and then fail while recording its roll.
 var pity_value:Variant=state.get("loot_pity",0)
 var rolls_value:Variant=state.get("loot_rolls",[])
 if not rolls_value is Array:return {}
 var pity:int=_safe_roll_number(pity_value,80)
 var rarity:Dictionary=RNG.roll(item,seed+pity*9973,0,pity)
 if rarity.is_empty() or not rarity.has("rarity_index"):return {}
 if not add_item(state,str(item["id"])):return {}
 # Bound even corrupted in-memory histories before recording the new drop.
 # Work on a copy so a failed reward never mutates the caller's roll list.
 var rolls:Array=rolls_value.slice(-249)
 rolls.append(rarity)
 state["loot_rolls"]=rolls
 state["loot_pity"]=RNG.next_pity(pity,int(rarity["rarity_index"]))
 var drop:Dictionary=item.duplicate(true)
 drop["roll"]=rarity
 return drop

static func _safe_roll_number(value:Variant,maximum:int)->int:
 # Saved loot metadata is untrusted; avoid casting objects or arrays to int.
 if typeof(value)!=TYPE_INT and typeof(value)!=TYPE_FLOAT:return 0
 return clampi(int(value),0,maximum)

static func _roll_strength(roll:Dictionary)->int:
 # Count only bounded bonuses that contribute to a hero stat.
 var strength:int=_safe_roll_number(roll.get("power_bonus",0),12)
 var used:Dictionary={}
 var affixes:Variant=roll.get("affixes",[])
 if not affixes is Array:return strength
 for affix in affixes:
  if not affix is Dictionary:continue
  var stat:String=str(affix.get("stat",""))
  if not RNG.AFFIXES.has(stat) or used.has(stat):continue
  used[stat]=true
  strength+=_safe_roll_number(affix.get("value",0),30)
 return strength

static func totals(state:Dictionary)->Dictionary:
 var stats:Dictionary={"weapon_power":0,"armor":0,"magic":0,"vitality":0,"haste":0,"crit":0,"ward":0,"elemental":0}
 var slots_value:Variant=state.get("equipped",{})
 var owned_value:Variant=state.get("items",{})
 var rolls_value:Variant=state.get("loot_rolls",[])
 if not slots_value is Dictionary or not owned_value is Dictionary:return stats
 var slots:Dictionary=slots_value
 var owned:Dictionary=owned_value
 var rolls:Array=rolls_value if rolls_value is Array else []
 # Index the strongest roll once per item instead of scanning the entire
 # saved roll history for every equipped slot on every stat refresh.
 var eligible_ids:Dictionary={}
 for slot in SLOTS:
  var equipped_id:String=str(slots.get(slot,""))
  if _safe_roll_number(owned.get(equipped_id,0),9999)<1:continue
  var equipped_item:Dictionary=CATALOG.get_item(equipped_id)
  if not equipped_item.is_empty() and slot_for(equipped_item)==slot:
   eligible_ids[equipped_id]=true
 var best_by_item:Dictionary={}
 for roll in rolls:
  if not roll is Dictionary:continue
  var roll_id:String=str(roll.get("item_id",""))
  if not eligible_ids.has(roll_id):continue
  var current:Dictionary=best_by_item.get(roll_id,{})
  var rarity:int=_safe_roll_number(roll.get("rarity_index",0),4)
  var current_rarity:int=int(current.get("rarity_index",-1))
  if rarity>current_rarity or (rarity==current_rarity and _roll_strength(roll)>_roll_strength(current)):
   best_by_item[roll_id]=roll
 for slot in SLOTS:
  var item_id:String=str(slots.get(slot,""))
  if _safe_roll_number(owned.get(item_id,0),9999)<1:continue
  var item:Dictionary=CATALOG.get_item(item_id)
  if item.is_empty() or slot_for(item)!=slot:continue
  var power:int=int(item["power"])
  match str(item["category"]):
   "weapon":stats["weapon_power"]+=power
   "armor":stats["armor"]+=power
   "offhand","accessory":stats["magic"]+=power
  # Apply only a matching owned item's highest rolled bonus.
  var best_roll:Dictionary=best_by_item.get(item_id,{})
  var bonus:int=_safe_roll_number(best_roll.get("power_bonus",0),12)
  if str(item["category"])=="weapon":stats["weapon_power"]+=bonus
  elif str(item["category"])=="armor":stats["armor"]+=bonus
  else:stats["magic"]+=bonus
  var affixes:Variant=best_roll.get("affixes",[])
  if not affixes is Array:continue
  var applied_affixes:Dictionary={}
  for affix in affixes:
   if not affix is Dictionary:continue
   var stat:String=str(affix.get("stat",""))
   if not RNG.AFFIXES.has(stat) or applied_affixes.has(stat):continue
   applied_affixes[stat]=true
   var value:int=_safe_roll_number(affix.get("value",0),30)
   match stat:
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
   var count:int=_safe_roll_number(prior_items[id],9999)
   if count>0 and not CATALOG.get_item(str(id)).is_empty():state["items"][str(id)]=count
 var prior_materials:Variant=saved.get("materials",{})
 if prior_materials is Dictionary:
  for material in prior_materials:
   if str(material)!="" and _safe_roll_number(prior_materials[material],999999)>0:
    state["materials"][str(material)]=_safe_roll_number(prior_materials[material],999999)
 state["gold"]=_safe_roll_number(saved.get("gold",0),99999999)
 state["loot_pity"]=_safe_roll_number(saved.get("loot_pity",0),80)
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
