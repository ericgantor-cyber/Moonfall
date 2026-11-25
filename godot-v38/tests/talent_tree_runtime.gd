extends SceneTree
## Run with: godot --headless --path godot-v38 --script res://tests/talent_tree_runtime.gd
const TALENTS = preload("res://scripts/systems/moonfall_talent_tree.gd")

func _initialize() -> void:
 var tree: Dictionary = TALENTS.load_tree()
 var nodes: Array = tree.get("nodes", [])
 if nodes.size() != 141:
  _fail("Expanded tree must contain 141 talents")
  return
 var counts: Dictionary = {"might": 0, "agility": 0, "arcana": 0}
 for node in nodes:
  var branch_name: String = str(node.get("branch", ""))
  if not counts.has(branch_name):
   _fail("Unexpected talent branch")
   return
  counts[branch_name] += 1
 for branch_name in counts:
  if int(counts[branch_name]) != 47:
   _fail("Each classless branch must contain 47 talents")
   return
 var high_tier: Dictionary = {"iron_will": 1, "breaker": 1, "guard": 1, "titan_strike": 1, "might_ashen_grip": 1, "might_rift_breaker": 1, "might_siege_instinct": 1, "might_oath_of_iron": 1, "might_warborn": 1, "might_moonforged_blade": 1, "might_worldsplitter": 1, "might_titan_s_legacy": 1, "might_cinder_vanguard": 1, "might_sovereign_hammer": 1}
 var new_capstone: Dictionary = TALENTS.spend(96, high_tier, "might_eclipse_champion")
 if not bool(new_capstone.get("ok", false)):
  _fail("New tier-14 capstone should be purchasable at level 96")
  return
 var too_early: Dictionary = TALENTS.spend(95, high_tier, "might_eclipse_champion")
 if bool(too_early.get("ok", true)):
  _fail("Tier-14 capstone should be level-gated until 96")
  return
 var advanced: Dictionary = {"iron_will": 1, "breaker": 1, "guard": 1, "titan_strike": 1}
 var extended: Dictionary = TALENTS.spend(16, advanced, "might_ashen_grip")
 if not bool(extended.get("ok", false)) or int(extended.get("spent", -1)) != 5:
  _fail("First expanded talent must unlock at level 16")
  return
 var premature: Dictionary = TALENTS.spend(15, advanced, "might_ashen_grip")
 if bool(premature.get("ok", true)):
  _fail("Expanded level gates must reject premature purchases")
  return
 var advanced_reset: Dictionary = TALENTS.respec(16, extended["allocations"])
 if not bool(advanced_reset.get("ok", false)) or int(advanced_reset.get("available", -1)) != 30:
  _fail("Full respec must clear expanded talent allocations")
  return
 var base: Dictionary = TALENTS.evaluate(5, {})
 if not bool(base.get("ok", false)) or int(base.get("available", -1)) != 8:
  _fail("Level 5 should have eight unspent points")
  return
 var first: Dictionary = TALENTS.spend(5, {}, "iron_will")
 if not bool(first.get("ok", false)):
  _fail("Root talent must be purchasable")
  return
 var second: Dictionary = TALENTS.spend(5, first["allocations"], "breaker")
 if not bool(second.get("ok", false)) or int(second.get("spent", -1)) != 2:
  _fail("Valid prerequisite chain must be purchasable")
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
 if not bool(reset.get("ok", false)) or int(reset.get("available", -1)) != 8:
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
