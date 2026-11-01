extends "res://scripts/native_v38_5.gd"
## Loot variety and bounded progression; preserves the original gear generation pipeline.
const LOOT_RARITIES:Array[String]=["Common","Uncommon","Rare","Epic","Legendary","Mythic"]
const LOOT_CHANCE_BONUS:float=0.08
var loot_roll_count:int=0

func _create_gear_drop(at:Vector3,kind:String,variant:String,zone:int,existing:Dictionary={})->void:
 # Existing guaranteed drops and special variant gear must stay untouched.
 if not existing.is_empty() or variant!="normal":
  super._create_gear_drop(at,kind,variant,zone,existing)
  return
 var previous:int=gear_drops.size()
 super._create_gear_drop(at,kind,variant,zone,existing)
 if gear_drops.size()<=previous:return
 var drop:Dictionary=gear_drops[gear_drops.size()-1]
 var item:Dictionary=drop.get("data",{})
 if item.is_empty():return
 # RNG is intentionally moderate; higher regions shift odds toward stronger tiers.
 var roll:float=rng.randf()
 var tier:int=0
 if roll<clampf(0.01+float(zone)*0.004,0.01,0.05):tier=4
 elif roll<clampf(0.045+float(zone)*0.008,0.045,0.11):tier=3
 elif roll<clampf(0.16+float(zone)*0.016,0.16,0.34):tier=2
 elif roll<clampf(0.48+float(zone)*0.020,0.48,0.70):tier=1
 var original_tier:int=LOOT_RARITIES.find(str(item.get("rarity","Common")))
 tier=maxi(tier,maxi(0,original_tier))
 item["rarity"]=LOOT_RARITIES[tier]
 # Bounded level-scaled progression, avoiding infinite power inflation.
 item["power"]=clampi(int(item.get("power",0))+mini(hero_level/4,20)+tier*2,1,180)
 if tier>=4:
  item["affix"]=AFFIXES[rng.randi_range(0,AFFIXES.size()-1)]
  item["affix_value"]=clampf(0.06+float(zone)*0.018+rng.randf_range(0.0,0.03),0.06,0.25)
 drop["data"]=item
 loot_roll_count+=1
 # Keep ground loot labeling consistent with the generated item.
 var node:Node3D=drop.get("node") as Node3D
 if is_instance_valid(node):
  node.name="GroundGear_"+str(item["rarity"])
  for child in node.get_children():
   if child is Label3D:
    (child as Label3D).text="◆ "+str(item["rarity"]).to_upper()+" "+str(item["name"])+" (G)"
