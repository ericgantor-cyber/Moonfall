extends Control
signal waypoint_selected(world_position:Vector3)
signal zoom_requested(factor:float)
## Simple real-position navigation chart. North = negative world Z.
var player_position:Vector3=Vector3.ZERO
var waypoint_position:Vector3=Vector3.ZERO
var has_waypoint:bool=false
var quest_markers:Array[Vector3]=[]
var discovered:Array[Vector3]=[]
var metres_per_half_width:float=120.0
const MAX_DISCOVERED_MARKERS:int=256
const MAX_QUEST_MARKERS:int=128
const MAX_WORLD_COORD:float=1000000.0
const MAX_HISTORY_POINTS:int=2048

# Save history separately from the small, viewport-friendly marker collection.
# Read newest valid positions first so malformed entries cannot evict discoveries.
static func encode_history(points:Array[Vector3])->Array:
 var encoded:Array=[]
 for index in range(points.size()-1,-1,-1):
  var point:Vector3=points[index]
  if not _valid_world_point(point):continue
  encoded.append([point.x,point.z])
  if encoded.size()>=MAX_HISTORY_POINTS:break
 encoded.reverse()
 return encoded

static func decode_history(raw:Variant)->Array[Vector3]:
 var history:Array[Vector3]=[]
 if not (raw is Array):return history
 var entries:Array=raw
 for index in range(entries.size()-1,-1,-1):
  var item:Variant=entries[index]
  if not (item is Array) or item.size()!=2:continue
  if not (item[0] is float or item[0] is int) or not (item[1] is float or item[1] is int):continue
  var point:Vector3=Vector3(float(item[0]),0.0,float(item[1]))
  if not _valid_world_point(point):continue
  history.append(point)
  if history.size()>=MAX_HISTORY_POINTS:break
 history.reverse()
 return history

static func _valid_world_point(point:Vector3)->bool:
 return is_finite(point.x) and is_finite(point.y) and is_finite(point.z) and absf(point.x)<=MAX_WORLD_COORD and absf(point.y)<=MAX_WORLD_COORD and absf(point.z)<=MAX_WORLD_COORD

func _validated_markers(points:Array[Vector3],limit:int,recent_first:bool=false)->Array[Vector3]:
 var accepted:Array[Vector3]=[]
 # The newest discoveries are closest to the current exploration area.
 if recent_first:
  for index in range(points.size()-1,-1,-1):
   if not _valid_world_point(points[index]):continue
   accepted.append(points[index])
   if accepted.size()>=limit:break
  accepted.reverse()
 else:
  # Quest order is meaningful; preserve the earliest priority objectives.
  for point in points:
   if not _valid_world_point(point):continue
   accepted.append(point)
   if accepted.size()>=limit:break
 return accepted

func _draw()->void:
 var rect:Rect2=Rect2(Vector2.ZERO,size)
 draw_rect(rect,Color(0.025,0.04,0.075,0.94),true)
 if size.x<20.0 or size.y<20.0:return
 var centre:Vector2=size*0.5
 var scale_factor:float=minf(size.x,size.y)*0.45/maxf(20.0,metres_per_half_width)
 for index in range(-4,5):
  var offset:float=float(index)*metres_per_half_width*scale_factor/4.0
  draw_line(Vector2(centre.x+offset,0),Vector2(centre.x+offset,size.y),Color(0.16,0.27,0.34,0.55),1.0)
  draw_line(Vector2(0,centre.y+offset),Vector2(size.x,centre.y+offset),Color(0.16,0.27,0.34,0.55),1.0)
 for visit in discovered:
  var visited_offset:Vector2=Vector2(visit.x-player_position.x,visit.z-player_position.z)*scale_factor
  if absf(visited_offset.x)<=size.x*0.45 and absf(visited_offset.y)<=size.y*0.45:
   draw_circle(centre+visited_offset,2.5,Color(0.22,0.65,0.58,0.65))
 draw_line(centre,centre+Vector2(0,-26),Color(0.35,0.85,1.0),3.0)
 draw_circle(centre,7.0,Color(0.36,0.85,1.0))
 if has_waypoint:
  var delta:Vector2=Vector2(waypoint_position.x-player_position.x,waypoint_position.z-player_position.z)*scale_factor
  delta=delta.limit_length(minf(size.x,size.y)*0.44)
  var marker:Vector2=centre+delta
  draw_line(centre,marker,Color(1.0,0.77,0.22,0.7),2.0)
  draw_circle(marker,7.0,Color(1.0,0.74,0.19))
 for quest_position in quest_markers:
  var quest_delta:Vector2=Vector2(quest_position.x-player_position.x,quest_position.z-player_position.z)*scale_factor
  if absf(quest_delta.x)>size.x*0.45 or absf(quest_delta.y)>size.y*0.45:
   continue
  var quest_screen:Vector2=centre+quest_delta
  draw_circle(quest_screen,7.0,Color(0.8,0.45,1.0))
  draw_arc(quest_screen,10.0,0.0,TAU,16,Color(1.0,0.8,1.0),1.5)
 draw_string(ThemeDB.fallback_font,Vector2(size.x*0.5-6.0,20.0),"N",HORIZONTAL_ALIGNMENT_LEFT,-1,16,Color(0.65,0.9,1.0))

func update_navigation(position:Vector3, waypoint:Vector3, enabled:bool)->void:
 if _valid_world_point(position):player_position=position
 has_waypoint=enabled and _valid_world_point(waypoint)
 if has_waypoint:waypoint_position=waypoint
 queue_redraw()

func _gui_input(event:InputEvent)->void:
 if event is InputEventMouseButton and event.pressed:
  if event.button_index==MOUSE_BUTTON_WHEEL_UP or event.button_index==MOUSE_BUTTON_WHEEL_DOWN:
   zoom_requested.emit(0.75 if event.button_index==MOUSE_BUTTON_WHEEL_UP else 1.333333)
   accept_event()
   return
 if event is InputEventMouseButton and event.pressed and event.button_index==MOUSE_BUTTON_LEFT:
  if size.x<20.0 or size.y<20.0:return
  var scale_factor:float=minf(size.x,size.y)*0.45/maxf(20.0,metres_per_half_width)
  var offset:Vector2=(event.position-size*0.5)/scale_factor
  var location:Vector3=Vector3(player_position.x+offset.x,player_position.y,player_position.z+offset.y)
  if not _valid_world_point(location):return
  waypoint_selected.emit(location)
  accept_event()

func set_quest_markers(markers:Array[Vector3])->void:
 quest_markers=_validated_markers(markers,MAX_QUEST_MARKERS)
 queue_redraw()

func set_discovered(positions:Array[Vector3])->void:
 discovered=_validated_markers(positions,MAX_DISCOVERED_MARKERS,true)
 queue_redraw()
