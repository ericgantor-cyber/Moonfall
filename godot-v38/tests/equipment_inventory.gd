extends SceneTree
const GEAR=preload("res://scripts/systems/equipment_inventory.gd")
var errors:int=0
func check(ok:bool,msg:String)->void:
 if ok:print("PASS inventory: "+msg)
 else:
  errors+=1
  push_error("FAIL inventory: "+msg)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var s:Dictionary=GEAR.new_state()
 check(not GEAR.equip(s,"moonsteel_sword",99),"cannot equip unowned weapon")
 check(GEAR.add_item(s,"moonsteel_sword"),"gain item")
 check(not GEAR.equip(s,"moonsteel_sword",1),"level gate")
 check(GEAR.equip(s,"moonsteel_sword",99),"equip owned weapon")
 check(GEAR.totals(s)["weapon_power"]>0,"equipped weapon has power")
 check(not GEAR.add_item(s,"fake_item"),"invalid loot rejected")
 check(not GEAR.craft(s,"ironbark_helmet",99),"craft requires ingredients")
 check(GEAR.add_material(s,"Ironbark",30),"earn crafting material")
 check(GEAR.craft(s,"ironbark_sentinel_chest",99),"craft armor")
 check(GEAR.equip(s,"ironbark_sentinel_chest",99),"equip crafted armor")
 check(GEAR.totals(s)["armor"]>0,"armor increases stats")
 var drop:Dictionary=GEAR.loot_enemy(s,12,100)
 check(not drop.is_empty() and s["items"].has(drop["id"]),"enemy loot enters inventory")
 var roundtrip:Dictionary=GEAR.normalize(s.duplicate(true))
 check(roundtrip["equipped"]==s["equipped"],"save round trip of equipment")
 check(roundtrip["items"]==s["items"],"save round trip of loot")
 check(GEAR.slot_for({"category":"accessory","family":"Amulet"})=="amulet","accessory slot")
 print("GEAR INVENTORY "+("PASS" if errors==0 else "FAIL"))
 quit(0 if errors==0 else 1)
