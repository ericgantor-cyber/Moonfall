extends RefCounted
## Integer copper prices and bounded upgrades; no real-money purchases.
const MAX_ENCHANT:int=5
const COPPER_PER_GOLD:int=10000

static func repair_cost(item:Dictionary)->int:
 var wear:int=100-clampi(int(item.get("durability",100)),0,100)
 if wear<=0:return 0
 var rarity:String=str(item.get("rarity","Common"))
 var rarity_weight:int=int({"Common":1,"Uncommon":2,"Crafted":2,"Rare":3,"Epic":5,"Legendary":8,"Mythic":12}.get(rarity,1))
 var power:int=clampi(int(item.get("power",0)),0,250)
 return clampi((wear*(25+power)*rarity_weight),100,500000)

static func enchant_cost(item:Dictionary)->int:
 var rank:int=clampi(int(item.get("enchant_level",0)),0,MAX_ENCHANT)
 if rank>=MAX_ENCHANT:return 0
 var rarity:String=str(item.get("rarity","Common"))
 var weight:int=int({"Common":2,"Uncommon":3,"Crafted":4,"Rare":5,"Epic":8,"Legendary":12,"Mythic":16}.get(rarity,2))
 return (rank+1)*weight*COPPER_PER_GOLD

static func enchanted_copy(item:Dictionary,affixes:Array)->Dictionary:
 var changed:Dictionary=item.duplicate(true)
 var rank:int=clampi(int(changed.get("enchant_level",0)),0,MAX_ENCHANT)
 if rank>=MAX_ENCHANT:return changed
 changed["enchant_level"]=rank+1
 changed["power"]=clampi(int(changed.get("power",0))+3+rank*2,0,250)
 if not affixes.is_empty() and str(changed.get("rarity","")) in ["Rare","Epic","Crafted","Legendary","Mythic"]:
  changed["affix"]=str(affixes[(rank+int(changed["power"]))%affixes.size()])
  changed["affix_value"]=clampf(0.04+float(rank+1)*0.018,0.0,0.35)
 return changed

static func chest_reward(zone:int)->Dictionary:
 var safe:int=clampi(zone,0,5)
 var rarity:String="Rare" if safe<=1 else "Epic" if safe<=3 else "Legendary"
 return {"coins":(12+safe*14)*COPPER_PER_GOLD+3500,"gear":{"name":"Moonbound Cache Relic "+str(safe+1),"rarity":rarity,"power":20+safe*9,"durability":100},"material":"Moon Sigil" if safe%2==0 else "Dusk Cinder"}

static func carrier_reward(zone:int,variant:String)->int:
 var safe:int=clampi(zone,0,5)
 return ((5+safe*6)+(5 if variant=="dusk" else 0))*COPPER_PER_GOLD+1800
