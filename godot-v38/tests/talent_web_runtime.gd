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
 if web.next_available() != "iron_will":
  _fail("The first available star must be the initial Might root")
  return
 if web.available_talent_ids().size() != 3:
  _fail("Unspent level-16 character should see three accessible roots")
  return
 web.select_node("might_ashen_grip")
 for prerequisite in ["might_ashen_grip", "titan_strike", "breaker", "guard", "iron_will"]:
  if not web.prerequisite_path.has(prerequisite):
   _fail("Selection must highlight the entire prerequisite web")
   return
 web.select_node("missing_talent")
 if web.selected_id != "might_ashen_grip" or not web.prerequisite_path.has("iron_will"):
  _fail("Unknown stars must preserve the current selection and route")
  return
 web.set_data(tree, {"iron_will": 1}, 16, 14)
 if not web.available_talent_ids().has("breaker") or not web.available_talent_ids().has("guard"):
  _fail("Unspent points and valid roots must unlock second-tier skills")
  return
 web.set_data(tree, {}, 16, 0)
 if not web.available_talent_ids().is_empty() or web.next_available() != "":
  _fail("No available points must disable next-talent navigation")
  return
 web.set_data(tree, {}, 16, 15)
 for branch in ["might", "agility", "arcana"]:
  web.focus_branch(branch)
  if web.pan == Vector2.ZERO:
   _fail("Focusing a branch should move the map")
   return
 web.fit_all()
 if web.pan != Vector2.ZERO:
  _fail("Fit-all must recenter the complete web")
  return
 # Zooming at a location must keep that point stationary on screen.
 web.fit_all()
 var anchor_world: Vector2 = web.points["iron_will"]
 var anchor_screen: Vector2 = web._screen_pos(anchor_world)
 web.zoom_at(1.6, anchor_screen)
 if web._screen_pos(anchor_world).distance_to(anchor_screen) > 0.1:
  _fail("Cursor-anchored zoom must preserve the selected position")
  return
 web.zoom_at(0.625, anchor_screen)
 if web._screen_pos(anchor_world).distance_to(anchor_screen) > 0.1:
  _fail("Cursor-anchored zoom-out must preserve the selected position")
  return
 if web.node_info.size() != 108:
  _fail("All talent hover metadata must be available")
  return
 web.zoom_by(30.0)
 if web.zoom > web.MAX_ZOOM:
  _fail("Zoom must be bounded above")
  return
 web.zoom_by(0.00001)
 if web.zoom < web.MIN_ZOOM:
  _fail("Zoom must be bounded below")
  return
 web.focus_node("might_moonforged_blade")
 if web.selected_id != "might_moonforged_blade":
  _fail("Keyboard talent index must select the requested constellation node")
  return
 if web._screen_pos(web.points["might_moonforged_blade"]).distance_to(web.size * 0.5) > 0.1:
  _fail("Focused talent should be centered in the viewport")
  return
 web.focus_node("missing_talent")
 if web.selected_id != "might_moonforged_blade":
  _fail("Unknown focus target should not lose current selection")
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
