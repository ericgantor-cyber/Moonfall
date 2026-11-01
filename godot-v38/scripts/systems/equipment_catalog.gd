extends RefCounted
## Original Moonfall equipment inspired by broad fantasy RPG equipment families.
## Data-only catalog: inventory, drops and equip UI can consume these definitions.
const WEAPONS:Array[String] = [
 "Sword", "Greatsword", "Hand Axe", "War Axe", "Mace", "Warhammer",
 "Club", "Daggers", "Bow", "Wand", "Staff", "Throwing Knives",
 "Spear", "Crossbow"
]
const ARMOR_SLOTS:Array[String] = ["Helmet","Chest","Gloves","Legs","Boots"]
const ARMOR_STYLES:Array[String] = ["Scout","Sentinel","Battlemage","Warden","Reaver"]
const OFFHANDS:Array[String] = ["Shield","Arcane Ward","Quiver","Tome"]
const ACCESSORIES:Array[String] = ["Ring","Amulet","Cloak"]
const MATERIAL_TIERS:Array[String] = ["Ashwood","Ironbark","Moonsteel","Starforged","Voidglass","Dawnfire"]
const RARITIES:Array[String] = ["Common","Uncommon","Rare","Epic","Legendary"]
const ELEMENTS:Array[String] = ["Physical","Fire","Frost","Shock","Poison","Arcane"]

static func generate_catalog()->Array[Dictionary]:
 var result:Array[Dictionary]=[]
 for tier in range(MATERIAL_TIERS.size()):
  for family in WEAPONS:
   result.append(_item("weapon",family,MATERIAL_TIERS[tier],tier))
  for style in ARMOR_STYLES:
   for slot in ARMOR_SLOTS:
    result.append(_item("armor",style+" "+slot,MATERIAL_TIERS[tier],tier))
  for offhand in OFFHANDS:
   result.append(_item("offhand",offhand,MATERIAL_TIERS[tier],tier))
  for accessory in ACCESSORIES:
   result.append(_item("accessory",accessory,MATERIAL_TIERS[tier],tier))
 return result

static func _item(category:String,family:String,material:String,tier:int)->Dictionary:
 var id:String=(material+"_"+family).to_lower().replace(" ","_")
 var rarity:String=RARITIES[mini(tier,RARITIES.size()-1)]
 # Predictable progression: gear gains roughly 20-25% per tier, with
 # family tradeoffs instead of every weapon in a tier having identical stats.
 var power:int=0
 if category=="weapon":
  var base:int=12
  if family in ["Greatsword","Warhammer","War Axe","Crossbow"]:base=16
  elif family in ["Daggers","Throwing Knives","Wand"]:base=9
  elif family in ["Bow","Staff","Spear"]:base=13
  power=base+3*tier
 elif category=="armor":
  var slot:String=family.split(" ")[-1]
  var base_armor:int=7 if slot=="Chest" else 4 if slot in ["Helmet","Legs"] else 2
  power=base_armor+maxi(1,base_armor*tier/3)
 elif category=="offhand":power=4+2*tier
 else:power=2+tier
 var element:String=ELEMENTS[(tier+family.length())%ELEMENTS.size()] if category=="weapon" else "Physical"
 return {"id":id,"name":material+" "+family,"category":category,"family":family,
  "tier":tier+1,"rarity":rarity,"power":power,"element":element,
  "level_required":1+tier*6,"upgrade_cap":mini(tier+1,5)}

static func get_item(item_id:String)->Dictionary:
 for entry in generate_catalog():
  if str(entry["id"])==item_id:return entry
 return {}

static func loot_for_level(level:int,seed:int)->Dictionary:
 var tier:int=clampi((maxi(1,level)-1)/6,0,MATERIAL_TIERS.size()-1)
 var entries:Array[Dictionary]=generate_catalog()
 var options:Array[Dictionary]=[]
 for entry in entries:
  if int(entry["tier"])==tier+1:options.append(entry)
 if options.is_empty():return {}
 var index:int=posmod(seed,options.size())
 return options[index].duplicate(true)

static func weapon_damage_multiplier(item:Dictionary)->float:
 if item.get("category","")!="weapon":return 1.0
 return minf(1.5, 1.0 + float(item.get("power",0))*0.008)

static func armor_bonus(item:Dictionary)->int:
 return int(item.get("power",0)) if item.get("category","")=="armor" else 0
