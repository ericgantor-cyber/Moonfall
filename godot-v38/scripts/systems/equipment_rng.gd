extends RefCounted
## Deterministic rarity rolls that never change global randomness.
const RARITIES:Array[String]=["Common","Uncommon","Rare","Epic","Legendary"]
const AFFIXES:Array[String]=["Power","Vitality","Haste","Crit","Ward","Elemental"]
static func roll(item:Dictionary,seed:int,luck:int=0,pity:int=0)->Dictionary:
 if item.is_empty():return {}
 var rng:RandomNumberGenerator=RandomNumberGenerator.new()
 rng.seed=seed
 var boost:int=clampi(luck,0,40)
 var chance:int=rng.randi_range(0,999)
 var rarity:int=0
 if chance<10+boost/2:rarity=4
 elif chance<50+boost:rarity=3
 elif chance<170+boost*2:rarity=2
 elif chance<410+boost*3:rarity=1
 if pity>=30:rarity=maxi(rarity,2)
 if pity>=80:rarity=maxi(rarity,3)
 var affixes:Array[Dictionary]=[]
 var pool:Array[String]=AFFIXES.duplicate()
 for i in range(mini(rarity,3)):
  var index:int=rng.randi_range(0,pool.size()-1)
  var kind:String=pool[index]
  pool.remove_at(index)
  affixes.append({"stat":kind,"value":1+int(item.get("tier",1))+rng.randi_range(0,rarity+1)})
 return {"item_id":str(item.get("id","")),"rarity":RARITIES[rarity],"rarity_index":rarity,"affixes":affixes,"power_bonus":rarity*2+rng.randi_range(0,2),"roll_seed":seed}
static func next_pity(current:int,rarity_index:int)->int:
 return 0 if rarity_index>=2 else mini(80,maxi(0,current)+1)
