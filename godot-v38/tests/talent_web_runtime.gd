extends SceneTree
## Real Godot headless verification for the 108-node, keyboard-accessible web.
const WEB = preload("res://scripts/systems/moonfall_talent_web.gd")
const TALENTS = preload("res://scripts/systems/moonfall_talent_tree.gd")

func _initialize() -> void:
 var web: Control = WEB.new()
 root.add_child(web)
 web.size = Vector2(800, 600)
 var tree: Dictionary = TALENTS.load_tree()
 web.set_data(tree, {}, 16, 15)
 if web.points.size() != 108:
  _fail("Web should plot 108 unique nodes")
  return
 for branch in ["might", "agility", "arcana"]:
  web.focus_branch(branch)
  if web.pan == Vector2.ZERO:
   _fail("Focusing a branch should move the map")
   return
 web.fit_all()
 if web.pan != Vector2.ZERO:
  _fail("Fit-all must recenter the complete web")
  return
 web.zoom_by(30.0)
 if web.zoom > web.MAX_ZOOM:
  _fail("Zoom must be bounded above")
  return
 web.zoom_by(0.00001)
 if web.zoom < web.MIN_ZOOM:
  _fail("Zoom must be bounded below")
  return
 var root_id: String = "iron_will"
 var root_pos: Vector2 = web._screen_pos(web.points[root_id])
 if web._node_at(root_pos) != root_id:
  _fail("Node hit-testing should select an original talent")
  return
 web.set_data(tree, {"iron_will": 1}, 16, 14)
 if not web.points.has("might_moonforged_blade"):
  _fail("Expanded constellation points are missing")
  return
 print("MOONFALL CONSTELLATION WEB PASS")
 quit(0)

func _fail(reason: String) -> void:
 push_error(reason)
 quit(1)
