extends SceneTree
## Run: godot --headless --path godot-v38 --script res://tests/crafting_capacity.gd
const INVENTORY = preload("res://scripts/systems/equipment_inventory.gd")
const CATALOG = preload("res://scripts/systems/equipment_catalog.gd")

func _initialize() -> void:
 call_deferred("_run")

func _run() -> void:
 var id:String = "ashwood_sword"
 var item:Dictionary = CATALOG.get_item(id)
 assert(not item.is_empty())
 var material:String = "Ashwood"
 var cost:int = 2 + int(item["tier"]) * 3
 var state:Dictionary = INVENTORY.new_state()
 state["materials"][material] = cost * 2
 state["items"][id] = 9999
 assert(not INVENTORY.craft(state,id,1))
 assert(int(state["materials"][material]) == cost * 2)
 assert(int(state["items"][id]) == 9999)
 state["items"][id] = 9998
 assert(INVENTORY.craft(state,id,1))
 assert(int(state["materials"][material]) == cost)
 assert(int(state["items"][id]) == 9999)
 assert(not INVENTORY.craft(state,id,1))
 assert(int(state["materials"][material]) == cost)
 assert(not INVENTORY.craft(state,"not_an_item",1))
 assert(int(state["materials"][material]) == cost)
 print("crafting_capacity: PASS")
 quit(0)
