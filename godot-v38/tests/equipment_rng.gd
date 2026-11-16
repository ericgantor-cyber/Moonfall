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
 var catalog_size:int=CATALOG.generate_catalog().size()
 check(catalog_size>100,"catalog is populated")
 check(CATALOG.get_item("missing_id").is_empty(),"unknown ID yields no item")
 var altered:Dictionary=CATALOG.get_item("ashwood_sword")
 altered["power"]=999999
 check(int(CATALOG.get_item("ashwood_sword")["power"])==int(item["power"]),"cached catalog returns mutation-safe copies")
 for entry in CATALOG.generate_catalog():
  check(CATALOG.get_item(str(entry["id"]))==entry,"cached lookup matches generated entry "+str(entry["id"]))
 var first:Dictionary=RNG.roll(item,2345)
 check(first==RNG.roll(item,2345),"same seed reproducible")
 check(first["affixes"].size()<=3,"bounded affixes")
 check(RNG.roll({},12).is_empty(),"empty item rejected")
 check(RNG.roll({"tier":1},12).is_empty(),"missing item ID rejected")
 var safe:Dictionary=RNG.roll({"id":"test_item","tier":-1000},481,0,80)
 for affix in safe["affixes"]:
  check(int(affix["value"])>=2,"negative tier cannot create negative affix")
 var extreme:Dictionary=RNG.roll({"id":"test_item","tier":1000000},481,0,80)
 for affix in extreme["affixes"]:
  check(int(affix["value"])<=106,"extreme tiers remain bounded")
 for seed in range(100):
  var result:Dictionary=RNG.roll(item,seed)
  check(result==RNG.roll(item,seed),"RNG remains deterministic "+str(seed))
 check(RNG.roll(item,3,0,80)==RNG.roll(item,3,0,0),"legacy pity argument never changes rarity")
 # Every rarity boundary is independently testable, with no drop history.
 check(RNG.rarity_from_chance(0)==4 and RNG.rarity_from_chance(9)==4,"legendary only below 1% boundary")
 check(RNG.rarity_from_chance(10)==3 and RNG.rarity_from_chance(49)==3,"epic 1%-5% boundary")
 check(RNG.rarity_from_chance(50)==2 and RNG.rarity_from_chance(169)==2,"rare 5%-17% boundary")
 check(RNG.rarity_from_chance(170)==1 and RNG.rarity_from_chance(409)==1,"uncommon 17%-41% boundary")
 check(RNG.rarity_from_chance(410)==0 and RNG.rarity_from_chance(999)==0,"common remains possible on every independent roll")
 check(RNG.rarity_from_chance(999,40)==0,"maximum luck does not guarantee rarity")
 check(RNG.rarity_from_chance(999,999999)==0,"excessive luck clamps to bounded odds")
 check(RNG.rarity_from_chance(29,0)==3 and RNG.rarity_from_chance(29,40)==4,"luck changes odds but not past-roll memory")
 check(RNG.rarity_from_chance(30,-99)==RNG.rarity_from_chance(30,0),"negative luck clamps safely")
 check(RNG.AFFIXES.has("Luck"),"Luck rolls as an equipment affix")
 var no_history:Dictionary=RNG.roll(item,2345,0,0)
 check(no_history==RNG.roll(item,2345,0,1000000),"even extreme legacy pity cannot change a roll")
 var generated_affixes:Dictionary={}
 for seed in range(1000):
  var candidate_roll:Dictionary=RNG.roll(item,seed)
  var unique:Dictionary={}
  for affix in candidate_roll["affixes"]:
   unique[str(affix["stat"])]=true
   generated_affixes[str(affix["stat"])]=true
  if unique.size()!=candidate_roll["affixes"].size():
   check(false,"random affixes never repeat within an item")
   break
 check(generated_affixes.size()>=4,"multiple independent affix varieties appear")

 var observed:Dictionary={}
 for seed in range(2000):
  var rolled:Dictionary=RNG.roll(item,seed)
  observed[int(rolled["rarity_index"])]=true
 check(observed.size()>=3,"many seeds produce varied random rarities")
 var lucky:Dictionary=INVENTORY.new_state()
 check(INVENTORY.add_item(lucky,"ashwood_sword"),"Luck item owned")
 check(INVENTORY.equip(lucky,"ashwood_sword",99),"Luck item equipped")
 lucky["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":3,"power_bonus":0,"affixes":[{"stat":"Luck","value":30}]})
 check(int(INVENTORY.totals(lucky)["luck"])==30,"equipped Luck bonus enters character stats")
 var lucky_seed:int=42
 var lucky_item:Dictionary=CATALOG.loot_for_level(12,lucky_seed)
 check(INVENTORY.loot_enemy(lucky,12,lucky_seed).get("roll",{})==RNG.roll(lucky_item,lucky_seed,30),"enemy drops use equipped Luck")
 var unowned_luck:Dictionary=INVENTORY.new_state()
 unowned_luck["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":4,"affixes":[{"stat":"Luck","value":30}]})
 check(int(INVENTORY.totals(unowned_luck)["luck"])==0,"unowned gear cannot grant Luck")
 check(INVENTORY.loot_enemy(unowned_luck,12,lucky_seed).get("roll",{})==RNG.roll(lucky_item,lucky_seed),"unowned gear cannot alter rarity")
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
 # Within the same rarity, the strongest valid roll must win rather than the oldest.
 empowered["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":3,"power_bonus":6,"affixes":[{"stat":"Power","value":9},{"stat":"Crit","value":4}]})
 var better:Dictionary=INVENTORY.totals(empowered)
 check(int(better["weapon_power"])==ordinary+15,"strongest equal-rarity roll wins")
 check(int(better["crit"])==4,"strongest same-rarity crit enters stats")
 empowered["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":3,"power_bonus":0,"affixes":[{"stat":"Power","value":1}]})
 check(INVENTORY.totals(empowered)==better,"weaker same-rarity copy cannot downgrade")
 empowered["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":4,"power_bonus":8,"affixes":[{"stat":"Power","value":1}]})
 var legendary:Dictionary=INVENTORY.totals(empowered)
 check(int(legendary["weapon_power"])==ordinary+9,"higher rarity takes precedence")
 check(int(legendary["crit"])==0,"highest rarity supplies affixes")
 check(int(INVENTORY.normalize(empowered)["loot_rolls"].size())==4,"all valid duplicate rolls survive save normalization")
 # Corrupt/legacy rolls must not stack the same affix repeatedly.
 var malformed:Dictionary=INVENTORY.new_state()
 INVENTORY.add_item(malformed,"ashwood_sword")
 INVENTORY.equip(malformed,"ashwood_sword",1)
 malformed["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":4,"power_bonus":0,"affixes":[{"stat":"Power","value":5},{"stat":"Power","value":30},{"stat":"Crit","value":7},{"stat":"Unknown","value":30}]})
 var guarded:Dictionary=INVENTORY.totals(malformed)
 check(int(guarded["weapon_power"])==ordinary+5,"duplicate Power affix only applies once")
 check(int(guarded["crit"])==7,"distinct supported affix still applies")
 malformed["loot_rolls"][0]["affixes"]="invalid"
 check(int(INVENTORY.totals(malformed)["weapon_power"])==ordinary,"malformed affix payload ignored")
 # Reconciled from PR #905: equipment bonuses require actual ownership and
 # a matching slot even when stale or corrupted save data is supplied.
 var unowned:Dictionary=INVENTORY.new_state()
 unowned["equipped"]["weapon"]="ashwood_sword"
 check(int(INVENTORY.totals(unowned)["weapon_power"])==0,"unowned equipment cannot grant combat stats")
 var wrong_slot:Dictionary=INVENTORY.new_state()
 INVENTORY.add_item(wrong_slot,"ashwood_sword")
 wrong_slot["equipped"]["helmet"]="ashwood_sword"
 check(int(INVENTORY.totals(wrong_slot)["weapon_power"])==0,"weapon in helmet slot cannot grant bonuses")
 var stale:Dictionary=empowered.duplicate(true)
 stale["items"].erase("ashwood_sword")
 check(int(INVENTORY.totals(stale)["weapon_power"])==0,"removed item cannot keep phantom combat stats")
 # Preserve PR #900's separate duplicate-affix and malformed-list cases above.
 var corrupted_owned:Dictionary=empowered.duplicate(true)
 corrupted_owned["items"]["ashwood_sword"]={"bad":true}
 check(int(INVENTORY.totals(corrupted_owned)["weapon_power"])==0,"malformed owned count cannot grant stats")
 var corrupted_rolls:Dictionary=empowered.duplicate(true)
 corrupted_rolls["loot_rolls"]=[{"item_id":"ashwood_sword","rarity_index":4,"power_bonus":{"bad":true},"affixes":[{"stat":"Power","value":["bad"]}]}]
 check(int(INVENTORY.totals(corrupted_rolls)["weapon_power"])==ordinary,"malformed roll numbers are ignored")
 var saved_currency:Dictionary=INVENTORY.new_state()
 saved_currency["gold"]=999999999999
 check(INVENTORY.normalize(saved_currency)["gold"]==999999999999,"unlimited currency is preserved")
 saved_currency["gold"]={"bad":true}
 check(INVENTORY.normalize(saved_currency)["gold"]==0,"malformed gold cannot corrupt loaded stats")
 var full_stacks:Dictionary=INVENTORY.new_state()
 full_stacks["items"]["ashwood_sword"]=9999
 check(not INVENTORY.add_item(full_stacks,"ashwood_sword",1),"full item stacks reject rewards")
 full_stacks["materials"]["Ashwood"]=999999
 check(not INVENTORY.add_material(full_stacks,"Ashwood",1),"full material stacks reject rewards")
 # PR #901 reconciliation: retain current strict stack rejection and 64-bit gold.
 # Crafting failure must never spend materials or mutate a full stack.
 var craft_item:Dictionary=CATALOG.get_item("ashwood_sword")
 var craft_material:String=str(craft_item["name"]).split(" ")[0]
 var craft_cost:int=2+int(craft_item["tier"])*3
 var blocked_craft:Dictionary=INVENTORY.new_state()
 blocked_craft["items"]["ashwood_sword"]=9999
 blocked_craft["materials"][craft_material]=craft_cost+5
 check(not INVENTORY.craft(blocked_craft,"ashwood_sword",99),"full-stack craft is rejected")
 check(int(blocked_craft["materials"][craft_material])==craft_cost+5,"rejected craft does not charge materials")
 # Failed enemy loot must not change legacy metadata or append a phantom roll.
 var candidate:Dictionary=CATALOG.loot_for_level(12,42)
 if not candidate.is_empty():
  var refused:Dictionary=INVENTORY.new_state()
  refused["items"][str(candidate["id"])]=9999
  refused["loot_pity"]=17
  check(INVENTORY.loot_enemy(refused,12,42).is_empty(),"full stack rejects enemy loot")
  check(int(refused["loot_pity"])==17 and refused["loot_rolls"].is_empty(),"failed loot preserves legacy metadata and roll history")
  var state_a:Dictionary=INVENTORY.new_state()
  var state_b:Dictionary=INVENTORY.new_state()
  state_b["loot_pity"]=80
  check(INVENTORY.loot_enemy(state_a,12,42).get("roll",{})==INVENTORY.loot_enemy(state_b,12,42).get("roll",{}),"identical seeds yield identical RNG regardless of old pity values")
 # Extra unrelated history must not alter equipped stats.
 var noisy:Dictionary=empowered.duplicate(true)
 for i in range(200):
  noisy["loot_rolls"].append({"item_id":"missing_item_"+str(i),"rarity_index":4,"power_bonus":12})
 check(INVENTORY.totals(noisy)==legendary,"unrelated rolls do not modify equipped stats")
 print("GEAR RNG "+("PASS" if errors==0 else "FAIL"))
 quit(0 if errors==0 else 1)
