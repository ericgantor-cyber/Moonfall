extends SceneTree
## Run with: godot --headless --path godot-v38 --script res://tests/talent_tree_runtime.gd
const TALENTS = preload("res://scripts/systems/moonfall_talent_tree.gd")

func _initialize() -> void:
 var base: Dictionary = TALENTS.evaluate(5, {})
 if not bool(base.get("ok", false)) or int(base.get("available", -1)) != 4:
  _fail("Level 5 should have four unspent points")
  return
 var first: Dictionary = TALENTS.spend(5, {}, "iron_will")
 if not bool(first.get("ok", false)):
  _fail("Root talent must be purchasable")
  return
 var second: Dictionary = TALENTS.spend(5, first["allocations"], "breaker")
 if not bool(second.get("ok", false)) or int(second.get("spent", -1)) != 2:
  _fail("Valid prerequisite chain must be purchasable")
  return
 var ranked: Dictionary = TALENTS.evaluate(10, {"iron_will": 5, "breaker": 3})
 if not bool(ranked.get("ok", false)) or int(ranked.get("spent", -1)) != 8:
  _fail("Multi-rank allocations must charge eight points")
  return
 if abs(float(ranked["bonuses"]["melee_damage"]["percent"]) - 0.27) > 0.0001:
  _fail("Multi-rank melee bonuses must stack correctly")
  return
 var over_cap: Dictionary = TALENTS.spend(10, ranked["allocations"], "breaker")
 if bool(over_cap.get("ok", true)):
  _fail("Max-rank talents must reject further purchases")
  return
 var over_budget: Dictionary = TALENTS.evaluate(5, {"iron_will": 5})
 if bool(over_budget.get("ok", true)):
  _fail("Multi-rank talents must respect earned point budget")
  return
 var orphan: Dictionary = TALENTS.spend(5, {}, "breaker")
 if bool(orphan.get("ok", true)):
  _fail("Missing prerequisites must reject purchases")
  return
 var removed: Dictionary = TALENTS.spend(5, {}, "hit")
 if bool(removed.get("ok", true)):
  _fail("Unknown talents must be rejected")
  return
 var reset: Dictionary = TALENTS.respec(5, second["allocations"])
 if not bool(reset.get("ok", false)) or int(reset.get("available", -1)) != 4:
  _fail("Full-tree respec must restore all points")
  return
 if not reset.get("allocations", {}).is_empty():
  _fail("Full-tree respec must clear all talents")
  return
 var invalid: Dictionary = TALENTS.respec(5, {"breaker": 1})
 if bool(invalid.get("ok", true)):
  _fail("Invalid existing talent builds must not be silently reset")
  return
 print("MOONFALL TALENT TREE RUNTIME PASS")
 quit(0)

func _fail(reason: String) -> void:
 push_error(reason)
 quit(1)
