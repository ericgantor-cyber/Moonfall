extends SceneTree
const RNG=preload("res://scripts/systems/equipment_rng.gd")
const INVENTORY=preload("res://scripts/systems/equipment_inventory.gd")
const CATALOG=preload("res://scripts/systems/equipment_catalog.gd")
var errors:int=0
func check(ok:bool,name:String)->void:
 if ok:print("PASS RNG: "+name)
 else:
  errors+=1
  push_error("FAIL RNG: "+name)
func _initialize()->void:
 call_deferred("_run")
func _run()->void:
 var item:Dictionary=CATALOG.get_item("ashwood_sword")
 var first:Dictionary=RNG.roll(item,2345)
 check(first==RNG.roll(item,2345),"same seed reproducible")
 check(first["affixes"].size()<=3,"bounded affixes")
 check(int(RNG.roll(item,3,0,80)["rarity_index"])>=3,"pity guarantees epic")
 check(RNG.next_pity(80,3)==0,"pity resets after rare roll")
 var state:Dictionary=INVENTORY.new_state()
 var drop:Dictionary=INVENTORY.loot_enemy(state,12,42)
 check(drop.has("roll") and state["loot_rolls"].size()==1,"loot roll persisted")
 check(INVENTORY.normalize(state)["loot_rolls"].size()==1,"save normalization preserves rolled loot")
 var empowered:Dictionary=INVENTORY.new_state()
 INVENTORY.add_item(empowered,"ashwood_sword")
 INVENTORY.equip(empowered,"ashwood_sword",1)
 var ordinary:int=int(INVENTORY.totals(empowered)["weapon_power"])
 empowered["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":3,"power_bonus":6,"affixes":[{"stat":"Power","value":4},{"stat":"Crit","value":3}]})
 var stats:Dictionary=INVENTORY.totals(empowered)
 check(int(stats["weapon_power"])==ordinary+10,"rolled Power and rarity modify equipped damage")
 check(int(stats["crit"])==3,"critical affix enters character stats")
 check(int(INVENTORY.normalize(empowered)["loot_rolls"].size())==1,"bonuses survive save normalization")
 print("GEAR RNG "+("PASS" if errors==0 else "FAIL"))
 quit(0 if errors==0 else 1)
