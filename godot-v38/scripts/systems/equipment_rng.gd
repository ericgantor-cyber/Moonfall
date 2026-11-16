extends RefCounted
## Seeded independent rarity rolls with no pity or guaranteed rarity thresholds.
const RARITIES:Array[String]=["Common","Uncommon","Rare","Epic","Legendary"]
const AFFIXES:Array[String]=["Power","Vitality","Haste","Crit","Ward","Elemental"]
static func roll(item:Dictionary,seed:int,luck:int=0,_legacy_pity:int=0)->Dictionary:
 # Legacy fourth argument is accepted for compatibility but never affects odds.
 if item.is_empty() or str(item.get("id","")).is_empty():return {}
 # Clamp imported or corrupted tiers before generating combat affixes.
 var tier:int=clampi(int(item.get("tier",1)),1,100)
 var rng:RandomNumberGenerator=RandomNumberGenerator.new()
 rng.seed=seed
 var chance:int=rng.randi_range(0,999)
 var rarity:int=rarity_from_chance(chance,luck)
 var affixes:Array[Dictionary]=[]
 var pool:Array[String]=AFFIXES.duplicate()
 for i in range(mini(rarity,3)):
  var index:int=rng.randi_range(0,pool.size()-1)
  var kind:String=pool[index]
  pool.remove_at(index)
  affixes.append({"stat":kind,"value":1+tier+rng.randi_range(0,rarity+1)})
 return {"item_id":str(item.get("id","")),"rarity":RARITIES[rarity],"rarity_index":rarity,"affixes":affixes,"power_bonus":rarity*2+rng.randi_range(0,2),"roll_seed":seed}

## Maps one independent 0..999 roll to rarity; no previous-drop state.
## At zero luck: 1% Legendary, 4% Epic, 12% Rare, 24% Uncommon.
## Luck only moves the thresholds, capped to preserve a Common chance.
static func rarity_from_chance(chance:int,luck:int=0)->int:
 var boost:int=clampi(luck,0,40)
 var value:int=clampi(chance,0,999)
 if value<10+boost/2:return 4
 if value<50+boost:return 3
 if value<170+boost*2:return 2
 if value<410+boost*3:return 1
 return 0
