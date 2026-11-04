extends Control
signal waypoint_selected(world_position:Vector3)
signal zoom_requested(factor:float)
## Simple real-position navigation chart. North = negative world Z.
var player_position:Vector3=Vector3.ZERO
var waypoint_position:Vector3=Vector3.ZERO
var has_waypoint:bool=false
var quest_markers:Array[Vector3]=[]
var discovered:Array[Vector3]=[]
var discovered_chambers:Array[Vector3]=[]
var dungeon_rooms:Array[Vector3]=[]
var dungeon_active:bool=false
var dungeon_current_room:int=-1
var metres_per_half_width:float=120.0
const MAX_DISCOVERED_MARKERS:int=256
const MAX_QUEST_MARKERS:int=128
const MAX_WORLD_COORD:float=1000000.0
const MAX_HISTORY_POINTS:int=2048
const MAX_CHAMBER_MARKERS:int=64
const MAX_TRAIL_SEGMENT_METRES:float=160.0
const DUNGEON_ENTER_RADIUS:float=24.0
const DUNGEON_LEAVE_RADIUS:float=35.0
const DUNGEON_DEFAULT_ZOOM:float=35.0

# Hysteresis prevents the map flickering at a dungeon boundary.
static func dungeon_site_at(position:Vector3,anchors:Array[Vector3],previous_site:int)->int:
 if not _valid_world_point(position):return -1
 if previous_site>=0 and previous_site<anchors.size():
  if _valid_world_point(anchors[previous_site]):
   var prior:Vector3=anchors[previous_site]
   if Vector2(position.x-prior.x,position.z-prior.z).length_squared()<=DUNGEON_LEAVE_RADIUS*DUNGEON_LEAVE_RADIUS:return previous_site
 var closest:int=-1
 var best_distance:float=DUNGEON_ENTER_RADIUS*DUNGEON_ENTER_RADIUS
 for i in range(anchors.size()):
  var anchor:Vector3=anchors[i]
  if not _valid_world_point(anchor):continue
  var distance:float=Vector2(position.x-anchor.x,position.z-anchor.z).length_squared()
  if distance<best_distance:
   best_distance=distance
   closest=i
 return closest

# Return the earliest authored chamber not yet recorded in this dungeon.
# -1 means no valid chamber remains; nothing about dungeon completion is fabricated.
static func next_unvisited_room(site:int,visited:Array[int],room_count:int=3)->int:
 if site<0 or room_count<=0 or room_count>MAX_CHAMBER_MARKERS:return -1
 for room_index in range(room_count):
  if not visited.has(site*room_count+room_index):return room_index
 return -1

# Identify the room the hero is physically approaching, not merely the closest distant dot.
static func dungeon_current_room_at(position:Vector3,rooms:Array[Vector3])->int:
 return dungeon_room_index_at(position,rooms,12.0)

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

# Existing world has three rifts with three authored chambers each.
# Keep checkpoint IDs validated and deduplicated; no extra scene nodes are created.
static func decode_chambers(raw:Variant,room_count:int)->Array[int]:
 var seen:Array[int]=[]
 if not (raw is Array):return seen
 for item in raw:
  if not (item is int):continue
  var chamber_id:int=item
  if chamber_id<0 or chamber_id>=room_count or chamber_id in seen:continue
  seen.append(chamber_id)
  if seen.size()>=room_count:break
 return seen

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

# Convert map clicks near numbered dungeon rooms into exact room waypoints.
static func dungeon_room_index_at(point:Vector3,rooms:Array[Vector3],radius_metres:float)->int:
 if not _valid_world_point(point) or not is_finite(radius_metres) or radius_metres<=0.0:return -1
 var best:float=radius_metres*radius_metres
 var nearest:int=-1
 for i in range(mini(rooms.size(),MAX_CHAMBER_MARKERS)):
  var chamber:Vector3=rooms[i]
  if not _valid_world_point(chamber):continue
  var distance:float=Vector2(point.x-chamber.x,point.z-chamber.z).length_squared()
  if distance<best:
   best=distance
   nearest=i
 return nearest

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
 # Connect nearby samples into a route trail, but never draw across teleports
 # or spend draw calls on segments entirely outside the chart viewport.
 var previous:Vector2=Vector2.ZERO
 var previous_valid:bool=false
 for visit in discovered:
  var offset:Vector2=Vector2(visit.x-player_position.x,visit.z-player_position.z)*scale_factor
  var visible:bool=absf(offset.x)<=size.x*0.45 and absf(offset.y)<=size.y*0.45
  if visible:
   if previous_valid and previous.distance_to(offset)<=MAX_TRAIL_SEGMENT_METRES*scale_factor:
    draw_line(centre+previous,centre+offset,Color(0.19,0.65,0.58,0.50),2.0)
   draw_circle(centre+offset,2.5,Color(0.22,0.65,0.58,0.65))
  previous=offset
  previous_valid=visible
 # These connections show the authored chamber sequence, not walkable corridors.
 if dungeon_active:
  for segment in range(1,dungeon_rooms.size()):
   var start_room:Vector3=dungeon_rooms[segment-1]
   var end_room:Vector3=dungeon_rooms[segment]
   var start_offset:Vector2=Vector2(start_room.x-player_position.x,start_room.z-player_position.z)*scale_factor
   var end_offset:Vector2=Vector2(end_room.x-player_position.x,end_room.z-player_position.z)*scale_factor
   if absf(start_offset.x)<=size.x*0.45 and absf(start_offset.y)<=size.y*0.45 and absf(end_offset.x)<=size.x*0.45 and absf(end_offset.y)<=size.y*0.45:
    draw_dashed_line(centre+start_offset,centre+end_offset,Color(0.50,0.58,0.70,0.52),1.5,5.0)
  for room_index in range(dungeon_rooms.size()):
   var room_position:Vector3=dungeon_rooms[room_index]
   var room_offset:Vector2=Vector2(room_position.x-player_position.x,room_position.z-player_position.z)*scale_factor
   if absf(room_offset.x)>size.x*0.45 or absf(room_offset.y)>size.y*0.45:continue
   var room_screen:Vector2=centre+room_offset
   draw_arc(room_screen,14.0 if room_index==dungeon_current_room else 11.0,0.0,TAU,20,Color(0.40,0.92,1.0) if room_index==dungeon_current_room else Color(0.47,0.55,0.66,0.85),2.5 if room_index==dungeon_current_room else 1.5)
   draw_string(ThemeDB.fallback_font,room_screen+Vector2(-4.0,5.0),str(room_index+1),HORIZONTAL_ALIGNMENT_LEFT,-1,14,Color(0.80,0.88,0.98))
 # Visited authored chambers are gold-ring landmarks independent of quests.
 for chamber_position in discovered_chambers:
  var chamber_offset:Vector2=Vector2(chamber_position.x-player_position.x,chamber_position.z-player_position.z)*scale_factor
  if absf(chamber_offset.x)>size.x*0.45 or absf(chamber_offset.y)>size.y*0.45:continue
  var chamber_screen:Vector2=centre+chamber_offset
  draw_circle(chamber_screen,4.0,Color(1.0,0.68,0.24))
  draw_arc(chamber_screen,7.0,0.0,TAU,12,Color(1.0,0.81,0.44),1.5)
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
  if dungeon_active:
   var nearest_room:int=dungeon_room_index_at(location,dungeon_rooms,14.0/scale_factor)
   if nearest_room>=0:location=dungeon_rooms[nearest_room]
  waypoint_selected.emit(location)
  accept_event()

func set_quest_markers(markers:Array[Vector3])->void:
 quest_markers=_validated_markers(markers,MAX_QUEST_MARKERS)
 queue_redraw()

func set_dungeon_context(active:bool,rooms:Array[Vector3])->void:
 if dungeon_active==active and (not active or dungeon_rooms==rooms):return
 dungeon_active=active
 dungeon_rooms.clear()
 dungeon_current_room=-1
 if active:dungeon_rooms=_validated_markers(rooms,MAX_CHAMBER_MARKERS)
 queue_redraw()

func update_dungeon_room(position:Vector3)->void:
 var next_room:int=dungeon_current_room_at(position,dungeon_rooms) if dungeon_active else -1
 if next_room!=dungeon_current_room:
  dungeon_current_room=next_room
  queue_redraw()

func set_chamber_markers(positions:Array[Vector3])->void:
 discovered_chambers=_validated_markers(positions,MAX_CHAMBER_MARKERS)
 queue_redraw()

func set_discovered(positions:Array[Vector3])->void:
 discovered=_validated_markers(positions,MAX_DISCOVERED_MARKERS,true)
 queue_redraw()
