extends SceneTree
## Regression: capacity rejection must not grant phantom loot or mutate RNG history.
const INV = preload("res://scripts/systems/equipment_inventory.gd")
const CATALOG = preload("res://scripts/systems/equipment_catalog.gd")

func _initialize() -> void:
 var state:Dictionary=INV.new_state()
 var item:Dictionary=CATALOG.loot_for_level(1,12345)
 assert(not item.is_empty(), "Expected deterministic catalog drop")
 var id:String=str(item["id"])
 assert(INV.add_item(state,id,9999), "Initial stack should fit")
 assert(not INV.add_item(state,id), "Full stack should reject additions")
 assert(int(state["items"][id])==9999, "Full stack must remain unchanged")
 assert(not INV.add_item(state,id,2), "Overflow must reject whole addition")
 assert(not INV.add_item(state,id,-1), "Negative count must be rejected")
 var pity:int=int(state["loot_pity"])
 var history:int=state["loot_rolls"].size()
 var dropped:Dictionary=INV.loot_enemy(state,1,12345)
 assert(dropped.is_empty(), "Full stack must not announce phantom loot")
 assert(int(state["loot_pity"])==pity, "Rejected loot must not change pity")
 assert(state["loot_rolls"].size()==history, "Rejected loot must not add roll history")
 assert(int(state["items"][id])==9999, "Rejected loot must not change stack")
 state["items"][id]=9998
 assert(not INV.loot_enemy(state,1,12345).is_empty(), "One free slot must permit loot")
 assert(int(state["items"][id])==9999, "Successful loot must fill last slot")
 assert(state["loot_rolls"].size()==history+1, "Successful loot must record its roll")
 print("PASS: enemy loot capacity and roll integrity")
 quit(0)
