extends RefCounted
## Isolated, integer-only Moonfall currency rules. No real-money transactions.
const COPPER_PER_SILVER:int=100
const SILVER_PER_GOLD:int=100
const COPPER_PER_GOLD:int=10000
const MAX_GOLD:int=99999999
const MAX_COPPER:int=MAX_GOLD*COPPER_PER_GOLD+9999

static func total(gold:int,silver:int=0,copper:int=0)->int:
 return clampi(maxi(0,gold)*COPPER_PER_GOLD+maxi(0,silver)*COPPER_PER_SILVER+maxi(0,copper),0,MAX_COPPER)

static func split_total(coins:int)->Dictionary:
 var safe:int=clampi(coins,0,MAX_COPPER)
 return {"gold":safe/COPPER_PER_GOLD,"silver":(safe%COPPER_PER_GOLD)/COPPER_PER_SILVER,"copper":safe%COPPER_PER_SILVER}

static func format_wallet(gold:int,silver:int,copper:int)->String:
 return format_total(total(gold,silver,copper))

static func format_total(coins:int)->String:
 var values:Dictionary=split_total(coins)
 return str(values["gold"])+"g  "+str(values["silver"])+"s  "+str(values["copper"])+"c"

static func sell_value(item:Dictionary)->int:
 var rarity:String=str(item.get("rarity","Common"))
 var weight:int= {"Common":1,"Uncommon":2,"Rare":4,"Crafted":3,"Epic":8,"Legendary":14,"Mythic":25}.get(rarity,1)
 var power:int=clampi(int(item.get("power",0)),0,250)
 # Gold is rare for low-level items; stronger rarities are worth far more.
 return clampi((10+power*3)*weight*25,100,250000)

static func bonus_loot(zone:int,variant:String,boss:bool,roll:int)->int:
 # Supplemental physical coin drops; base legacy gold rewards are unchanged.
 var silver:int=6+clampi(zone,0,5)*5+(clampi(roll,0,99)%18)
 if variant=="moon":silver+=10
 if variant=="dusk":silver+=20
 if variant=="elder":silver+=40
 if boss:silver+=100
 return silver*COPPER_PER_SILVER+(clampi(roll,0,99)%93)
