extends RefCounted
## Data-only deterministic combinations. Does not spawn nodes or allocate an entire catalog.
## Use derived values at spawn time; existing hero textures and encounters are untouched.
const FAMILIES:=["beast","undead","construct","elemental","plant","demon","specter","insect","dragon","aquatic","lunar"]
const ROLES:=["skirmisher","bruiser","sniper","guardian","summoner","support","ambusher","caster","charger","controller","elite"]
const ELEMENTS:=["physical","fire","frost","storm","earth","water","shadow","light","poison","arcane","lunar"]
const MODIFIERS:=["swift","armored","frenzied","vampiric","volatile","shielded","regenerating","splitter","reflective","burrowing","enraged"]
const ATTACKS:=["melee","lunge","projectile","beam","area","wave","trap","summon","dash","charge","burst","cone"]
const ARENAS:=["forest","quarry","ruins","cavern","shore","citadel","rift","foundry","crypt","temple","moonfield"]
const LOOT_TYPES:=["blade","bow","staff","armor","charm","ring","relic","material","potion","gem","recipe"]
const RARITIES:=["common","uncommon","rare","epic","legendary","mythic","ancient","lunar","astral","celestial","primordial"]

static func _pick(items:Array,seed:int,salt:int)->String:
 var index:int=posmod(seed+salt*7919,items.size())
 return str(items[index])

static func enemy(seed:int,level:int=1)->Dictionary:
 var lvl:int=clampi(level,1,1000)
 var family:String=_pick(FAMILIES,seed,1)
 var role:String=_pick(ROLES,floori(float(seed)/11.0),2)
 var element:String=_pick(ELEMENTS,floori(float(seed)/121.0),3)
 var modifier:String=_pick(MODIFIERS,floori(float(seed)/1331.0),4)
 return {"seed":seed,"family":family,"role":role,"element":element,"modifier":modifier,"level":lvl,"hp_scale":minf(20.0,1.0+0.045*float(lvl-1)),"damage_scale":minf(12.0,1.0+0.025*float(lvl-1))}

static func boss(seed:int,level:int=1)->Dictionary:
 var creature:Dictionary=enemy(seed,level)
 creature["boss"]=true
 creature["attack"]=_pick(ATTACKS,floori(float(seed)/14641.0),5)
 creature["arena"]=_pick(ARENAS,floori(float(seed)/161051.0),6)
 creature["phase_count"]=2+posmod(seed,3)
 return creature

static func loot(seed:int,level:int=1)->Dictionary:
 var lvl:int=clampi(level,1,1000)
 return {"seed":seed,"type":_pick(LOOT_TYPES,seed,7),"rarity":_pick(RARITIES,floori(float(seed)/11.0),8),"affix":_pick(MODIFIERS,floori(float(seed)/121.0),9),"level":lvl,"power":mini(250,lvl*3+posmod(seed,12))}

static func encounter(seed:int,level:int=1)->Dictionary:
 return {"enemy":enemy(seed,level),"boss":boss(seed+1,level),"loot":loot(seed+2,level),"arena":_pick(ARENAS,floori(float(seed)/11.0),10)}
