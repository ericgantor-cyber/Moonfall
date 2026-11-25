extends SceneTree
## Run: godot --headless --path godot-v38 --script res://tests/crafting_capacity.gd
const INVENTORY = preload("res://scripts/systems/equipment_inventory.gd")
const CATALOG = preload("res://scripts/systems/equipment_catalog.gd")
var failures:int = 0

func _initialize() -> void:
 call_deferred("_run")

func _check(ok:bool, message:String) -> void:
 if not ok:
  failures += 1
  push_error("crafting_capacity: " + message)

func _run() -> void:
 var id:String = "ashwood_sword"
 var item:Dictionary = CATALOG.get_item(id)
 if item.is_empty():
  push_error("crafting_capacity: missing fixture item")
  quit(1)
  return
 var material:String = "Ashwood"
 var cost:int = 2 + int(item["tier"]) * 3
 var state:Dictionary = INVENTORY.new_state()
 state["materials"][material] = cost * 2
 state["items"][id] = 9999

 var full_result:bool = INVENTORY.craft(state,id,1)
 _check(not full_result, "full stack must reject craft")
 _check(int(state["materials"][material]) == cost * 2, "rejected craft must preserve materials")
 _check(int(state["items"][id]) == 9999, "rejected craft must preserve items")

 state["items"][id] = 9998
 var valid_result:bool = INVENTORY.craft(state,id,1)
 _check(valid_result, "valid craft must succeed")
 _check(int(state["materials"][material]) == cost, "valid craft must deduct cost once")
 _check(int(state["items"][id]) == 9999, "valid craft must reach stack cap")

 var repeat_result:bool = INVENTORY.craft(state,id,1)
 _check(not repeat_result, "repeat craft must reject full stack")
 _check(int(state["materials"][material]) == cost, "repeat rejection must preserve materials")

 var invalid_result:bool = INVENTORY.craft(state,"not_an_item",1)
 _check(not invalid_result, "invalid item must reject craft")
 _check(int(state["materials"][material]) == cost, "invalid rejection must preserve materials")

 if failures == 0:
  print("crafting_capacity: PASS")
 else:
  push_error("crafting_capacity: %d checks failed" % failures)
 quit(1 if failures > 0 else 0)
