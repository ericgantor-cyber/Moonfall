extends SceneTree
const Rules = preload("res://scripts/systems/inventory_rules.gd")

func _initialize() -> void:
 var supplies:Dictionary={"ore":5,"wood":3}
 var recipe:Dictionary={"result":"Iron Sword","power":10,"cost":{"ore":3,"wood":2}}
 var result:Dictionary=Rules.craft_commit(recipe,supplies)
 assert(result["gear"].get("name","")=="Iron Sword")
 assert(supplies["ore"]==2 and supplies["wood"]==1)
 var snapshot:Dictionary=supplies.duplicate(true)
 assert(Rules.craft_commit(recipe,supplies)["gear"].is_empty())
 assert(supplies==snapshot)
 var invalid:Dictionary={"result":"Broken","power":10,"cost":{"ore":1,"wood":0}}
 assert(Rules.craft_commit(invalid,supplies)["gear"].is_empty())
 assert(supplies==snapshot)
 invalid["cost"]={"ore":1,"wood":-1}
 assert(Rules.craft_commit(invalid,supplies)["gear"].is_empty())
 assert(supplies==snapshot)
 invalid["cost"]={"ore":1,"wood":1.5}
 assert(Rules.craft_commit(invalid,supplies)["gear"].is_empty())
 assert(supplies==snapshot)
 invalid["cost"]={"ore":1,"wood":2}
 assert(Rules.craft_commit(invalid,supplies)["gear"].is_empty())
 assert(supplies==snapshot)
 print("Crafting atomicity regression passed")
 quit(0)
