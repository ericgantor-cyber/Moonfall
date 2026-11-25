extends RefCounted
## Classless Moonfall talent calculations. No hero artwork, scene or save edits.
## A native source module, not yet wired to the player or character screen.
const DATA_PATH: String = "res://data/moonfall_talents.json"

static func load_tree() -> Dictionary:
 if not FileAccess.file_exists(DATA_PATH):
  return {}
 var file := FileAccess.open(DATA_PATH, FileAccess.READ)
 if file == null:
  return {}
 var data: Variant = JSON.parse_string(file.get_as_text())
 if typeof(data) != TYPE_DICTIONARY:
  return {}
 return data

static func evaluate(level: int, allocations: Dictionary = {}) -> Dictionary:
 var tree: Dictionary = load_tree()
 if int(tree.get("schema_version", 0)) != 1:
  return {"ok": false, "error": "Talent data unavailable"}
 var maximum_level: int = int(tree.get("maximum_level", 0))
 if level < 1 or level > maximum_level:
  return {"ok": false, "error": "Invalid character level"}
 var index: Dictionary = {}
 var nodes: Array = tree.get("nodes", [])
 for entry in nodes:
  if typeof(entry) != TYPE_DICTIONARY or not entry.has("id"):
   return {"ok": false, "error": "Invalid talent node"}
  index[str(entry["id"])] = entry
 var normalized: Dictionary = {}
 var spent: int = 0
 var bonuses: Dictionary = {}
 for stat in ["health", "mana", "stamina", "melee_damage", "ranged_damage", "magic_damage"]:
  bonuses[stat] = {"flat": 0.0, "percent": 0.0}
 for key in allocations:
  var talent_id: String = str(key)
  if not index.has(talent_id):
   return {"ok": false, "error": "Unknown talent choice"}
  var rank: Variant = allocations[key]
  var node: Dictionary = index[talent_id]
  if typeof(rank) != TYPE_INT or int(rank) < 0 or int(rank) > int(node.get("max_rank", 0)):
   return {"ok": false, "error": "Invalid allocated rank"}
  if int(rank) > 0:
   if level < int(node.get("min_level", 1)):
    return {"ok": false, "error": "Talent level gate not reached"}
   normalized[talent_id] = rank
   spent += int(rank) * int(node.get("cost_per_rank", 1))
 for talent_id in normalized:
  var node: Dictionary = index[talent_id]
  for requirement in node.get("requires", []):
   if int(normalized.get(str(requirement), 0)) < 1:
    return {"ok": false, "error": "Missing talent prerequisite"}
  var effect: Dictionary = node.get("effect", {})
  var stat: String = str(effect.get("stat", ""))
  var kind: String = str(effect.get("kind", ""))
  if not bonuses.has(stat) or not ["flat", "percent"].has(kind):
   return {"ok": false, "error": "Invalid talent effect"}
  bonuses[stat][kind] += int(normalized[talent_id]) * float(effect.get("amount_per_rank", 0.0))
 var earned: int = int(tree.get("starting_points", 0)) + (level - 1) * int(tree.get("points_per_level", 0))
 if spent > earned:
  return {"ok": false, "error": "Not enough talent points"}
 return {"ok": true, "level": level, "earned": earned, "spent": spent,
  "available": earned - spent, "allocations": normalized, "bonuses": bonuses,
  "ruleset": "classless_talent_tree"}

static func spend(level: int, allocations: Dictionary, talent_id: String) -> Dictionary:
 var old_state: Dictionary = evaluate(level, allocations)
 if not bool(old_state.get("ok", false)):
  return old_state
 var new_allocations: Dictionary = old_state["allocations"].duplicate()
 new_allocations[talent_id] = int(new_allocations.get(talent_id, 0)) + 1
 return evaluate(level, new_allocations)

static func respec(level: int, allocations: Dictionary) -> Dictionary:
 ## Only full-tree respec. Validate the existing build before clearing it.
 var old_state: Dictionary = evaluate(level, allocations)
 if not bool(old_state.get("ok", false)):
  return old_state
 return evaluate(level, {})
