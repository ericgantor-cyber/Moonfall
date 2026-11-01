extends SceneTree
const CATALOG = preload("res://scripts/systems/equipment_catalog.gd")
var failures:int = 0
func check(ok:bool,label:String)->void:
 if not ok:
  failures+=1
  push_error("FAIL equipment: "+label)
 else:print("PASS equipment: "+label)
func _initialize()->void:
 call_deferred("_run")
func _run()->void:
 var items:Array[Dictionary]=CATALOG.generate_catalog()
 check(items.size()==276,"six tiers of 46 unique items")
 var ids:Dictionary={}
 for item in items:
  check(not ids.has(item["id"]),"unique id "+str(item["id"]))
  ids[item["id"]]=true
  check(int(item["power"])>0,"positive power")
  check(int(item["upgrade_cap"])<=5,"bounded upgrade cap")
 check(CATALOG.WEAPONS.size()==14,"14 weapon families")
 check(CATALOG.ARMOR_SLOTS.size()==5,"five armor slots")
 check(CATALOG.get_item("moonsteel_sword").get("family")=="Sword","lookup works")
 check(CATALOG.loot_for_level(18,-7).get("tier")==3,"loot tier clamps and handles negative seed")
 check(CATALOG.weapon_damage_multiplier(CATALOG.get_item("moonsteel_sword"))>1.0,"weapon scaling")
 check(CATALOG.armor_bonus(CATALOG.get_item("ironbark_sentinel_chest"))>0,"armor stat")
 print("EQUIPMENT CATALOG "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
