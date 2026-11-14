extends Control
## Moonfall-original, zoomable 108-node constellation web.
## Mouse: wheel zoom, drag empty space to pan, click a node to select.
## Keyboard: +/- zoom, arrow keys pan, Home recenters the map.
signal talent_selected(talent_id: String)

const BRANCH_COLORS: Dictionary = {
 "might": Color("#d66773"),
 "agility": Color("#65c9ac"),
 "arcana": Color("#a99bff")
}
const BRANCH_ANGLES: Dictionary = {"might": -150.0, "agility": -30.0, "arcana": 90.0}
const LANE_ANGLES: Array[float] = [-26.0, -8.0, 8.0, 26.0]
const MIN_ZOOM: float = 0.16
const MAX_ZOOM: float = 2.1
const NODE_RADIUS: float = 13.0
var tree: Dictionary = {}
var allocations: Dictionary = {}
var level: int = 1
var available: int = 0
var points: Dictionary = {}
var zoom: float = 0.21
var pan: Vector2 = Vector2.ZERO
var selected_id: String = ""
var dragging: bool = false
var drag_distance: float = 0.0
var hovered_id: String = ""
var mouse_down_on_node: String = ""
var node_info: Dictionary = {}
var prerequisite_path: Dictionary = {}

func _ready() -> void:
 mouse_filter = Control.MOUSE_FILTER_STOP
 focus_mode = Control.FOCUS_ALL
 clip_contents = true
 queue_redraw()

func set_data(next_tree: Dictionary, next_allocations: Dictionary, character_level: int, unspent: int) -> void:
 tree = next_tree
 allocations = next_allocations.duplicate()
 level = character_level
 available = unspent
 _layout_nodes()
 _update_prerequisite_path()
 queue_redraw()

func focus_branch(branch_id: String) -> void:
 if not BRANCH_ANGLES.has(branch_id):return
 var direction: Vector2 = Vector2.from_angle(deg_to_rad(float(BRANCH_ANGLES[branch_id])))
 pan = -direction * 480.0
 zoom = 0.75
 queue_redraw()

func focus_node(talent_id: String) -> void:
 if not points.has(talent_id):return
 selected_id = talent_id
 _update_prerequisite_path()
 pan = -points[talent_id]
 zoom = clampf(maxf(zoom, 0.75), MIN_ZOOM, MAX_ZOOM)
 queue_redraw()

func select_node(talent_id: String) -> void:
 if not points.has(talent_id):return
 selected_id = talent_id
 _update_prerequisite_path()
 queue_redraw()

func _update_prerequisite_path() -> void:
 prerequisite_path.clear()
 if not node_info.has(selected_id):return
 var pending: Array[String] = [selected_id]
 while not pending.is_empty():
  var current: String = pending.pop_back()
  if prerequisite_path.has(current):continue
  prerequisite_path[current] = true
  var node: Dictionary = node_info.get(current, {})
  for requirement in node.get("requires", []):
   var dependency: String = str(requirement)
   if node_info.has(dependency) and not prerequisite_path.has(dependency):
    pending.append(dependency)

func available_talent_ids() -> Array[String]:
 var result: Array[String] = []
 if available <= 0:return result
 for node in tree.get("nodes", []):
  if typeof(node) != TYPE_DICTIONARY:continue
  var id: String = str(node.get("id", ""))
  if not points.has(id):continue
  if int(allocations.get(id, 0)) >= int(node.get("max_rank", 1)):continue
  if level < int(node.get("min_level", 1)):continue
  var requirements_met: bool = true
  for requirement in node.get("requires", []):
   if int(allocations.get(str(requirement), 0)) <= 0:
    requirements_met = false
    break
  if requirements_met and int(node.get("cost_per_rank", 1)) <= available:
   result.append(id)
 return result

func next_available(after_id: String = "") -> String:
 var candidates: Array[String] = available_talent_ids()
 if candidates.is_empty():return ""
 var index: int = candidates.find(after_id)
 return candidates[(index + 1) % candidates.size()]

func fit_all() -> void:
 pan = Vector2.ZERO
 zoom = clampf((minf(size.x, size.y) - 36.0) / 2660.0, MIN_ZOOM, MAX_ZOOM)
 queue_redraw()

func zoom_by(factor: float) -> void:
 zoom = clampf(zoom * factor, MIN_ZOOM, MAX_ZOOM)
 queue_redraw()

func zoom_at(factor: float, screen_point: Vector2) -> void:
 ## Keep the star under the cursor fixed while changing scale.
 var before: Vector2 = _world_pos(screen_point)
 zoom = clampf(zoom * factor, MIN_ZOOM, MAX_ZOOM)
 pan = (screen_point - size * 0.5) / zoom - before
 queue_redraw()

func _layout_nodes() -> void:
 points.clear()
 node_info.clear()
 var node_counts: Dictionary = {}
 for node in tree.get("nodes", []):
  if typeof(node) != TYPE_DICTIONARY:continue
  var branch: String = str(node.get("branch", ""))
  if not BRANCH_ANGLES.has(branch):continue
  var tier: int = int(node.get("tier", 1))
  var key: String = branch + "_" + str(tier)
  var lane: int = int(node_counts.get(key, 0))
  node_counts[key] = lane + 1
  var angle: float = float(BRANCH_ANGLES[branch])
  if tier == 2:
   angle += -13.0 if lane == 0 else 13.0
  elif tier >= 4:
   angle += LANE_ANGLES[mini(lane, 3)]
   if tier % 2 == 0:angle += 3.5 if lane % 2 == 0 else -3.5
  var radius: float = 105.0 + float(tier - 1) * 116.0
  if tier >= 4:
   radius += 24.0 if lane == 1 or lane == 2 else -12.0
  var talent_id: String = str(node.get("id", ""))
  points[talent_id] = Vector2.from_angle(deg_to_rad(angle)) * radius
  node_info[talent_id] = node

func _screen_pos(world: Vector2) -> Vector2:
 return size * 0.5 + (world + pan) * zoom

func _world_pos(screen: Vector2) -> Vector2:
 return (screen - size * 0.5) / zoom - pan

func _node_at(screen: Vector2) -> String:
 var closest: String = ""
 var closest_distance: float = 1.0e20
 for id in points:
  var distance: float = _screen_pos(points[id]).distance_to(screen)
  if distance < maxf(16.0, NODE_RADIUS * zoom + 10.0) and distance < closest_distance:
   closest = str(id)
   closest_distance = distance
 return closest

func _gui_input(event: InputEvent) -> void:
 if event is InputEventMouseButton:
  var mouse: InputEventMouseButton = event
  if mouse.button_index == MOUSE_BUTTON_WHEEL_UP and mouse.pressed:
   zoom_at(1.18, mouse.position)
   accept_event()
   return
  if mouse.button_index == MOUSE_BUTTON_WHEEL_DOWN and mouse.pressed:
   zoom_at(1.0 / 1.18, mouse.position)
   accept_event()
   return
  if mouse.button_index == MOUSE_BUTTON_LEFT or mouse.button_index == MOUSE_BUTTON_MIDDLE:
   if mouse.pressed:
    dragging = true
    drag_distance = 0.0
    mouse_down_on_node = _node_at(mouse.position) if mouse.button_index == MOUSE_BUTTON_LEFT else ""
    grab_focus()
   else:
    if dragging and mouse.button_index == MOUSE_BUTTON_LEFT and drag_distance < 9.0:
     var id: String = _node_at(mouse.position)
     if id != "" and id == mouse_down_on_node:
      select_node(id)
      talent_selected.emit(id)
    dragging = false
    mouse_down_on_node = ""
   accept_event()
   return
 if event is InputEventMouseMotion:
  var motion: InputEventMouseMotion = event
  if dragging:
   drag_distance += motion.relative.length()
   if mouse_down_on_node == "":pan += motion.relative / zoom
  var next_hover: String = _node_at(motion.position)
  if next_hover != hovered_id:
   hovered_id = next_hover
   tooltip_text = str(node_info.get(hovered_id, {}).get("title", "")) if hovered_id != "" else ""
  queue_redraw()
  accept_event()
  return
 if event is InputEventKey and event.pressed and not event.echo:
  var key: InputEventKey = event
  if key.keycode == KEY_EQUAL or key.keycode == KEY_KP_ADD:
   zoom_by(1.2)
  elif key.keycode == KEY_MINUS or key.keycode == KEY_KP_SUBTRACT:
   zoom_by(1.0 / 1.2)
  elif key.keycode == KEY_HOME:
   fit_all()
  elif key.keycode == KEY_N:
   var next_id: String = next_available(selected_id)
   if next_id != "":
    focus_node(next_id)
    talent_selected.emit(next_id)
  elif key.keycode == KEY_LEFT:
   pan.x += 100.0 / zoom
  elif key.keycode == KEY_RIGHT:
   pan.x -= 100.0 / zoom
  elif key.keycode == KEY_UP:
   pan.y += 100.0 / zoom
  elif key.keycode == KEY_DOWN:
   pan.y -= 100.0 / zoom
  else:
   return
  queue_redraw()
  accept_event()

func _draw() -> void:
 draw_rect(Rect2(Vector2.ZERO, size), Color("#090f1c"))
 var mid: Vector2 = _screen_pos(Vector2.ZERO)
 var center_color: Color = Color("#8995ba")
 for radius in [70.0, 135.0, 215.0]:
  draw_arc(mid, radius * zoom, 0.0, TAU, 64, Color(0.32, 0.42, 0.59, 0.18), 1.0, true)
 draw_circle(mid, maxf(4.0, 13.0 * zoom), center_color)
 # The three open branches radiate from a shared progression origin.
 for branch in BRANCH_ANGLES:
  var direction: Vector2 = Vector2.from_angle(deg_to_rad(float(BRANCH_ANGLES[branch])))
  var hub: Vector2 = _screen_pos(direction * 105.0)
  var ray: Color = BRANCH_COLORS[branch]
  ray.a = 0.35
  draw_line(mid, hub, ray, 2.0, true)
 var nodes: Array = tree.get("nodes", [])
 # Connect prerequisites before drawing nodes, keeping links readable.
 for node in nodes:
  var id: String = str(node.get("id", ""))
  if not points.has(id):continue
  var branch: String = str(node.get("branch", ""))
  var branch_color: Color = BRANCH_COLORS.get(branch, Color.WHITE)
  var target: Vector2 = _screen_pos(points[id])
  for dependency in node.get("requires", []):
   var source_id: String = str(dependency)
   if not points.has(source_id):continue
   var source: Vector2 = _screen_pos(points[source_id])
   var active: bool = int(allocations.get(source_id, 0)) > 0 and int(allocations.get(id, 0)) > 0
   var line_color: Color = branch_color
   var on_path: bool = prerequisite_path.has(source_id) and prerequisite_path.has(id)
   line_color.a = 0.95 if on_path else (0.9 if active else 0.16)
   draw_line(source, target, line_color, 4.0 if on_path else (3.2 if active else 1.4), true)
 for node in nodes:
  var id: String = str(node.get("id", ""))
  if not points.has(id):continue
  var center: Vector2 = _screen_pos(points[id])
  if center.x < -25.0 or center.y < -25.0 or center.x > size.x + 25.0 or center.y > size.y + 25.0:continue
  var branch: String = str(node.get("branch", ""))
  var col: Color = BRANCH_COLORS.get(branch, Color.WHITE)
  var rank: int = int(allocations.get(id, 0))
  var max_rank: int = int(node.get("max_rank", 1))
  var prereqs_ok: bool = true
  for requirement in node.get("requires", []):
   if int(allocations.get(str(requirement), 0)) == 0:prereqs_ok = false
  var available_now: bool = prereqs_ok and level >= int(node.get("min_level", 1)) and available > 0 and rank < max_rank
  var radius: float = maxf(5.0, NODE_RADIUS * zoom)
  if int(node.get("tier", 1)) == 3 or int(node.get("tier", 1)) == 11:radius *= 1.5
  draw_circle(center, radius + 3.0, Color("#070d17"))
  col.a = 1.0 if rank > 0 else (0.8 if available_now else 0.30)
  draw_circle(center, radius, col if rank > 0 else Color("#111c30"))
  draw_arc(center, radius + 1.5, 0, TAU, 24, col, 2.0, true)
  if available_now and rank == 0:
   draw_circle(center, maxf(2.0, radius * 0.27), col)
  if id == selected_id:
   draw_arc(center, radius + 6.0, 0, TAU, 36, Color.WHITE, 2.5, true)
  elif prerequisite_path.has(id):
   draw_arc(center, radius + 4.0, 0, TAU, 36, Color("#e3c98d"), 2.0, true)
  elif id == hovered_id:
   draw_arc(center, radius + 5.0, 0, TAU, 36, Color("#e3d9ab"), 2.2, true)
 var font: Font = ThemeDB.fallback_font
 if font != null:
  draw_string(font, Vector2(16, 32), "MOONFALL  •  CONSTELLATION WEB", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color("#e0e7fa"))
  draw_string(font, Vector2(16, size.y - 18), "Wheel: zoom  •  Drag: pan  •  N: next available  •  Home: fit  •  Arrows: move", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color("#b8c2d7"))
  if hovered_id != "" and node_info.has(hovered_id):
   var info: Dictionary = node_info[hovered_id]
   var rank: int = int(allocations.get(hovered_id, 0))
   var caption: String = str(info.get("title", hovered_id)) + "  [" + str(rank) + "/" + str(info.get("max_rank", 1)) + "]"
   draw_string(font, Vector2(16, 60), caption, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color("#f0e7ca"))
