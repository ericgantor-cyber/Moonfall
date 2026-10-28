extends RefCounted
## v26 pure item validation; inventory, crafting and drops share one rule set.
const VALID_RARITIES := ["Common","Uncommon","Rare","Epic","Legendary","Mythic","Crafted"]

static func sanitize(source:Dictionary,affixes:Array) -> Dictionary:
 var rarity:=str(source.get("rarity","Common"))
 if not (rarity in VALID_RARITIES):rarity="Common"
 var item:Dictionary={"name":str(source.get("name","Unknown Gear")).substr(0,60),"rarity":rarity,"power":clampi(int(source.get("power",0)),0,250)}
 if rarity in ["Rare","Epic","Crafted","Legendary","Mythic"] and str(source.get("affix","")) in affixes:
  item["affix"]=str(source["affix"])
  item["affix_value"]=clampf(float(source.get("affix_value",0.0)),0.0,0.35)
 # Optional fields maintain schema-2 compatibility with v30.6 and older saves.
 item["durability"]=clampi(int(source.get("durability",100)),0,100)
 item["enchant_level"]=clampi(int(source.get("enchant_level",0)),0,5)
 var uid:String=str(source.get("uid","")).substr(0,40)
 if uid!="":item["uid"]=uid
 return item
