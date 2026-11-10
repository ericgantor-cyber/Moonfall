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
 # Stale or tampered equipped references must not grant free combat stats.
 var unowned:Dictionary=INVENTORY.new_state()
 unowned["equipped"]["weapon"]="ashwood_sword"
 check(int(INVENTORY.totals(unowned)["weapon_power"])==0,"unowned equipped item grants no power")
 var wrong_slot:Dictionary=INVENTORY.new_state()
 INVENTORY.add_item(wrong_slot,"ashwood_sword")
 wrong_slot["equipped"]["helmet"]="ashwood_sword"
 check(int(INVENTORY.totals(wrong_slot)["weapon_power"])==0,"wrong-slot equipment grants no power")
 var stale:Dictionary=empowered.duplicate(true)
 stale["items"].erase("ashwood_sword")
 check(int(INVENTORY.totals(stale)["weapon_power"])==0,"removed item grants no stale equipment power")
 check(int(INVENTORY.totals(empowered)["weapon_power"])==ordinary,"valid equipped item retains its power")
 # Malformed legacy save fields must not crash combat stat calculation.
 for field in ["equipped","items"]:
  var malformed:Dictionary=empowered.duplicate(true)
  malformed[field]="invalid"
  check(int(INVENTORY.totals(malformed)["weapon_power"])==0,"invalid "+field+" safely grants no stats")
 var bad_rolls:Dictionary=empowered.duplicate(true)
 bad_rolls["loot_rolls"]="invalid"
 check(int(INVENTORY.totals(bad_rolls)["weapon_power"])==ordinary,"invalid loot rolls retain base stats")
 bad_rolls["loot_rolls"]=[{"item_id":"ashwood_sword","rarity_index":4,"power_bonus":5,"affixes":"invalid"}]
 check(int(INVENTORY.totals(bad_rolls)["weapon_power"])==ordinary+5,"invalid affixes retain bounded rarity bonus")
 # A matching owned roll may contain malformed numeric values in an old save.
 var malformed_numbers:Dictionary=empowered.duplicate(true)
 malformed_numbers["loot_rolls"]=[{"item_id":"ashwood_sword","rarity_index":{"bad":true},"power_bonus":["bad"],"affixes":[{"stat":"Power","value":{"bad":true}},{"stat":"Crit","value":["bad"]}]}]
 var numeric_stats:Dictionary=INVENTORY.totals(malformed_numbers)
 check(int(numeric_stats["weapon_power"])==ordinary,"malformed numeric loot bonuses are ignored")
 check(int(numeric_stats["crit"])==0,"malformed numeric affixes grant no crit")
 check(INVENTORY.totals(INVENTORY.normalize(malformed_numbers))==numeric_stats,"malformed numeric loot survives safe save normalization")
 empowered["loot_rolls"].append({"item_id":"ashwood_sword","rarity_index":3,"power_bonus":6,"affixes":[{"stat":"Power","value":4},{"stat":"Crit","value":3}]})
 var stats:Dictionary=INVENTORY.totals(empowered)
 check(int(stats["weapon_power"])==ordinary+10,"rolled Power and rarity modify equipped damage")
 check(int(stats["crit"])==3,"critical affix enters character stats")
 # A corrupt saved roll cannot multiply a stat by repeating its affix.
 var duplicate_affix:Dictionary=empowered.duplicate(true)
 duplicate_affix["loot_rolls"]=[{"item_id":"ashwood_sword","rarity_index":3,"power_bonus":6,"affixes":[{"stat":"Power","value":4},{"stat":"Power","value":30},{"stat":"Crit","value":3},{"stat":"Crit","value":30},"invalid"]}]
 var deduped:Dictionary=INVENTORY.totals(duplicate_affix)
 check(int(deduped["weapon_power"])==ordinary+10,"duplicate Power affix cannot stack")
 check(int(deduped["crit"])==3,"duplicate Crit affix cannot stack")
 check(INVENTORY.totals(INVENTORY.normalize(duplicate_affix))==deduped,"duplicate-affix guard survives save roundtrip")
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
 # Unrelated loot history must not change equipped stats after roll indexing.
 var noisy:Dictionary=empowered.duplicate(true)
 for i in range(200):
  noisy["loot_rolls"].append({"item_id":"missing_item_"+str(i),"rarity_index":4,"power_bonus":12,"affixes":[{"stat":"Power","value":30}]})
 check(INVENTORY.totals(noisy)==legendary,"unrelated roll history cannot change equipped stats")
 # Unrelated malformed roll metadata must never be parsed for combat totals.
 noisy["loot_rolls"].append({"item_id":"missing_item_bad","rarity_index":{"corrupt":true},"power_bonus":{"corrupt":true},"affixes":[{"stat":"Power","value":{"corrupt":true}}]})
 check(INVENTORY.totals(noisy)==legendary,"unrelated malformed rolls are ignored before stat ranking")
 var empty_gear:Dictionary=INVENTORY.new_state()
 empty_gear["loot_rolls"]=noisy["loot_rolls"].duplicate(true)
 check(int(INVENTORY.totals(empty_gear)["weapon_power"])==0,"unequipped loot history never grants weapon power")
 print("GEAR RNG "+("PASS" if errors==0 else "FAIL"))
 quit(0 if errors==0 else 1)
