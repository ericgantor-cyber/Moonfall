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
var dungeon_floor_radii:Array[float]=[]
var dungeon_passages:Array[Dictionary]=[]
var dungeon_pillars:Array[Vector3]=[]
var dungeon_trace:Array[Vector3]=[]
var player_facing:Vector2=Vector2.UP
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
const DUNGEON_DEFAULT_ZOOM:float=75.0
const DUNGEON_MAX_FLOOR_RADIUS:float=50.0
const MAX_DUNGEON_PASSAGES:int=48
const MAX_DUNGEON_PILLARS:int=192
const MAX_DUNGEON_TRAIL_POINTS:int=384
const DUNGEON_TRAIL_SPACING:float=2.5
const DUNGEON_TRAIL_CONNECT:float=9.0

# Sample local paths densely, without crossing teleports or changing global exploration.
static func append_dungeon_sample(points:Array[Vector3],position:Vector3)->void:
 if not _valid_world_point(position):return
 var flat:Vector3=Vector3(position.x,0.0,position.z)
 if not points.is_empty() and points.back().distance_squared_to(flat)<DUNGEON_TRAIL_SPACING*DUNGEON_TRAIL_SPACING:return
 points.append(flat)
 if points.size()>MAX_DUNGEON_TRAIL_POINTS:points.pop_front()

# Godot forward is -Z; chart north is -Z, so up is forward.
static func chart_facing(forward:Vector3)->Vector2:
 if not _valid_world_point(forward):return Vector2.UP
 var heading:Vector2=Vector2(forward.x,forward.z)
 return heading.normalized() if heading.length_squared()>0.0001 else Vector2.UP

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

# Reject off-screen corridor geometry before allocating polygons or draw commands.
# The expanded segment bounds include the actual passage width.
static func passage_near_chart(start:Vector3,finish:Vector3,width:float,centre_position:Vector3,half_extent:Vector2)->bool:
 if not _valid_world_point(start) or not _valid_world_point(finish) or not _valid_world_point(centre_position):return false
 if not is_finite(width) or width<=0.0 or width>20.0:return false
 if not is_finite(half_extent.x) or not is_finite(half_extent.y) or half_extent.x<=0.0 or half_extent.y<=0.0:return false
 var margin:float=width*0.5
 return maxf(start.x,finish.x)+margin>=centre_position.x-half_extent.x and minf(start.x,finish.x)-margin<=centre_position.x+half_extent.x and maxf(start.z,finish.z)+margin>=centre_position.z-half_extent.y and minf(start.z,finish.z)-margin<=centre_position.z+half_extent.y

# World objectives remain visible; only quest markers inside an undiscovered
# physical dungeon chamber are hidden by that chamber's fog of war.
static func dungeon_quest_marker_visible(marker:Vector3,rooms:Array[Vector3],radii:Array[float],known:Array[Vector3])->bool:
 if not _valid_world_point(marker):return false
 for i in range(mini(rooms.size(),MAX_CHAMBER_MARKERS)):
  if i>=radii.size():continue
  var radius:float=radii[i]
  if not is_finite(radius) or radius<=0.0:continue
  var chamber:Vector3=rooms[i]
  if not _valid_world_point(chamber):continue
  if Vector2(marker.x-chamber.x,marker.z-chamber.z).length_squared()>radius*radius:continue
  for visited_position in known:
   if Vector2(visited_position.x-chamber.x,visited_position.z-chamber.z).length_squared()<1.0:return true
  return false
 return true

# Point toward distant objectives without placing indicators outside the chart.
static func quest_chart_edge_offset(offset:Vector2,half_bounds:Vector2)->Vector2:
 if not is_finite(offset.x) or not is_finite(offset.y):return Vector2.ZERO
 if half_bounds.x<=0.0 or half_bounds.y<=0.0:return Vector2.ZERO
 var overflow:float=maxf(1.0,maxf(absf(offset.x)/half_bounds.x,absf(offset.y)/half_bounds.y))
 return offset/overflow

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
 # These solid walkways exist as StaticBody3D platforms in the connected world.
 if dungeon_active:
  for passage in dungeon_passages:
   var start:Vector3=passage["start"]
   var finish:Vector3=passage["end"]
   var width:float=passage["width"]
   if not bool(passage.get("explored",false)):continue # Skip hidden geometry before viewport and polygon work.
   if not passage_near_chart(start,finish,width,player_position,Vector2(size.x,size.y)*0.5/scale_factor):continue
   var path:Vector2=Vector2(finish.x-start.x,finish.z-start.z)
   if path.length_squared()<0.0001:continue
   var normal:Vector2=Vector2(-path.y,path.x).normalized()*width*0.5*scale_factor
   var from_point:Vector2=centre+Vector2(start.x-player_position.x,start.z-player_position.z)*scale_factor
   var to_point:Vector2=centre+Vector2(finish.x-player_position.x,finish.z-player_position.z)*scale_factor
   var corners:=PackedVector2Array([from_point+normal,to_point+normal,to_point-normal,from_point-normal])
   draw_colored_polygon(corners,Color(0.34,0.37,0.44,0.82))
   var edge_color:Color=Color(0.72,0.76,0.83,0.88)
   draw_line(from_point+normal,to_point+normal,edge_color,1.5)
   draw_line(from_point-normal,to_point-normal,edge_color,1.5)
 # Draw only authored floor meshes as a local top-down plan. No invented corridors.
 if dungeon_active:
  for room_index in range(dungeon_rooms.size()):
   var room_position:Vector3=dungeon_rooms[room_index]
   var room_offset:Vector2=Vector2(room_position.x-player_position.x,room_position.z-player_position.z)*scale_factor
   var floor_radius:float=dungeon_floor_radii[room_index]*scale_factor if room_index<dungeon_floor_radii.size() else 0.0
   if floor_radius<=0.0 or absf(room_offset.x)>size.x*0.5+floor_radius or absf(room_offset.y)>size.y*0.5+floor_radius:continue
   var room_screen:Vector2=centre+room_offset
   var visited:bool=false
   for visited_position in discovered_chambers:
    if Vector2(visited_position.x-room_position.x,visited_position.z-room_position.z).length_squared()<1.0:
     visited=true
     break
   if not visited:continue # Hide unvisited chamber geometry and labels.
   draw_circle(room_screen,floor_radius,Color(0.29,0.33,0.39,0.77))
   draw_arc(room_screen,floor_radius,0.0,TAU,24,Color(0.93,0.76,0.48) if visited else Color(0.35,0.42,0.52,0.65),2.0)
   if room_index==dungeon_current_room:
    draw_arc(room_screen,floor_radius+4.0,0.0,TAU,24,Color(0.38,0.90,1.0),2.5)
   draw_string(ThemeDB.fallback_font,room_screen+Vector2(-4.0,5.0),str(room_index+1),HORIZONTAL_ALIGNMENT_LEFT,-1,14,Color(0.95,0.96,0.99) if visited else Color(0.58,0.63,0.71))
 # Local breadcrumbs reflect real sampled hero movement, not inferred corridors.
 if dungeon_active:
  var last_offset:Vector2=Vector2.ZERO
  var last_visible:bool=false
  for sample in dungeon_trace:
   var offset:Vector2=Vector2(sample.x-player_position.x,sample.z-player_position.z)*scale_factor
   var visible:bool=absf(offset.x)<=size.x*0.45 and absf(offset.y)<=size.y*0.45
   if visible:
    if last_visible and last_offset.distance_squared_to(offset)<=DUNGEON_TRAIL_CONNECT*DUNGEON_TRAIL_CONNECT*scale_factor*scale_factor:
     draw_line(centre+last_offset,centre+offset,Color(0.24,0.84,0.75,0.74),1.8)
    draw_circle(centre+offset,1.6,Color(0.24,0.84,0.75,0.74))
   last_offset=offset
   last_visible=visible
 # World-space structural pillars are visible only in discovered dungeon rooms.
 if dungeon_active:
  for pillar in dungeon_pillars:
   var pillar_offset:Vector2=Vector2(pillar.x-player_position.x,pillar.z-player_position.z)*scale_factor
   if absf(pillar_offset.x)>size.x*0.5 or absf(pillar_offset.y)>size.y*0.5:continue
   var pillar_discovered:bool=false
   for room_index in range(dungeon_rooms.size()):
    if room_index>=dungeon_floor_radii.size():continue
    var room_position:Vector3=dungeon_rooms[room_index]
    var radius:float=dungeon_floor_radii[room_index]
    if Vector2(pillar.x-room_position.x,pillar.z-room_position.z).length_squared()>radius*radius:continue
    for visited_position in discovered_chambers:
     if Vector2(visited_position.x-room_position.x,visited_position.z-room_position.z).length_squared()<1.0:
      pillar_discovered=true
      break
    if pillar_discovered:break
   if not pillar_discovered:continue
   var mapped:Vector2=centre+pillar_offset
   draw_rect(Rect2(mapped-Vector2(2,2),Vector2(4,4)),Color(0.75,0.80,0.89,0.9),true)
 # Visited authored chambers are gold-ring landmarks independent of quests.
 for chamber_position in discovered_chambers:
  var chamber_offset:Vector2=Vector2(chamber_position.x-player_position.x,chamber_position.z-player_position.z)*scale_factor
  if absf(chamber_offset.x)>size.x*0.45 or absf(chamber_offset.y)>size.y*0.45:continue
  var chamber_screen:Vector2=centre+chamber_offset
  draw_circle(chamber_screen,4.0,Color(1.0,0.68,0.24))
  draw_arc(chamber_screen,7.0,0.0,TAU,12,Color(1.0,0.81,0.44),1.5)
 var forward_tip:Vector2=centre+player_facing*17.0
 var side:Vector2=Vector2(-player_facing.y,player_facing.x)
 draw_colored_polygon(PackedVector2Array([forward_tip,centre-player_facing*7.0+side*6.0,centre-player_facing*7.0-side*6.0]),Color(0.36,0.85,1.0))
 draw_circle(centre,3.0,Color(0.88,0.98,1.0))
 if has_waypoint:
  var delta:Vector2=Vector2(waypoint_position.x-player_position.x,waypoint_position.z-player_position.z)*scale_factor
  delta=delta.limit_length(minf(size.x,size.y)*0.44)
  var marker:Vector2=centre+delta
  draw_line(centre,marker,Color(1.0,0.77,0.22,0.7),2.0)
  draw_circle(marker,7.0,Color(1.0,0.74,0.19))
 # The first visible priority objective gets a gold marker; other quests stay purple.
 var primary_quest_drawn:bool=false
 for quest_position in quest_markers:
  if dungeon_active and not dungeon_quest_marker_visible(quest_position,dungeon_rooms,dungeon_floor_radii,discovered_chambers):continue
  var primary:bool=not primary_quest_drawn
  primary_quest_drawn=true
  var quest_color:Color=Color(1.0,0.78,0.24) if primary else Color(0.8,0.45,1.0)
  var quest_delta:Vector2=Vector2(quest_position.x-player_position.x,quest_position.z-player_position.z)*scale_factor
  if absf(quest_delta.x)>size.x*0.45 or absf(quest_delta.y)>size.y*0.45:
   var direction:Vector2=quest_delta.normalized()
   var side:Vector2=Vector2(-direction.y,direction.x)
   var edge:Vector2=centre+quest_chart_edge_offset(quest_delta,size*0.40)
   draw_colored_polygon(PackedVector2Array([edge+direction*5.0,edge-direction*7.0+side*4.0,edge-direction*7.0-side*4.0]),quest_color)
   continue
  var quest_screen:Vector2=centre+quest_delta
  draw_circle(quest_screen,8.0 if primary else 7.0,quest_color)
  draw_arc(quest_screen,12.0 if primary else 10.0,0.0,TAU,16,Color(1.0,0.93,0.65) if primary else Color(1.0,0.8,1.0),1.5)
 draw_string(ThemeDB.fallback_font,Vector2(size.x*0.5-6.0,20.0),"N",HORIZONTAL_ALIGNMENT_LEFT,-1,16,Color(0.65,0.9,1.0))

func update_player_facing(forward:Vector3)->void:
 var next_heading:Vector2=chart_facing(forward)
 if next_heading!=player_facing:
  player_facing=next_heading
  queue_redraw()

func set_dungeon_passages(passages:Array[Dictionary])->void:
 var accepted:Array[Dictionary]=[]
 for passage in passages:
  if accepted.size()>=MAX_DUNGEON_PASSAGES:break
  var start:Variant=passage.get("start")
  var finish:Variant=passage.get("end")
  var width:Variant=passage.get("width")
  if not (start is Vector3) or not (finish is Vector3) or not (width is float or width is int):continue
  var beginning:Vector3=start
  var ending:Vector3=finish
  var metres:float=float(width)
  if not _valid_world_point(beginning) or not _valid_world_point(ending) or not is_finite(metres) or metres<=0.0 or metres>20.0:continue
  if beginning.distance_squared_to(ending)<0.01:continue
  accepted.append({"start":beginning,"end":ending,"width":metres,"explored":bool(passage.get("explored",false))})
 if dungeon_passages==accepted:return
 dungeon_passages=accepted
 queue_redraw()

func set_dungeon_trace(points:Array[Vector3])->void:
 var valid:Array[Vector3]=_validated_markers(points,MAX_DUNGEON_TRAIL_POINTS,true)
 if dungeon_trace==valid:return
 dungeon_trace=valid
 queue_redraw()

func set_dungeon_pillars(points:Array[Vector3])->void:
 var valid:Array[Vector3]=_validated_markers(points,MAX_DUNGEON_PILLARS)
 if dungeon_pillars==valid:return
 dungeon_pillars=valid
 queue_redraw()

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
   # Reject clicks anywhere on undiscovered chamber floors, including edges
   # outside the narrower room-center snapping radius.
   if not dungeon_quest_marker_visible(location,dungeon_rooms,dungeon_floor_radii,discovered_chambers):return
   var click_radius:float=minf(14.0/scale_factor,14.0) # Cap hit testing in world metres at wide zoom.
   var nearest_room:int=dungeon_room_index_at(location,dungeon_rooms,click_radius)
   if nearest_room>=0:
    var candidate:Vector3=dungeon_rooms[nearest_room]
    var room_discovered:bool=false
    for visited_position in discovered_chambers:
     if Vector2(visited_position.x-candidate.x,visited_position.z-candidate.z).length_squared()<1.0:
      room_discovered=true
      break
    if not room_discovered:return # Do not place waypoints inside hidden chambers.
    location=candidate
  waypoint_selected.emit(location)
  accept_event()

func set_quest_markers(markers:Array[Vector3])->void:
 quest_markers=_validated_markers(markers,MAX_QUEST_MARKERS)
 queue_redraw()

func set_dungeon_context(active:bool,rooms:Array[Vector3],radii:Array[float]=[])->void:
 var accepted_rooms:Array[Vector3]=[]
 if active:accepted_rooms=_validated_markers(rooms,MAX_CHAMBER_MARKERS)
 var accepted_radii:Array[float]=[]
 for i in range(accepted_rooms.size()):
  var radius:float=radii[i] if i<radii.size() else 0.0
  accepted_radii.append(clampf(radius,0.0,DUNGEON_MAX_FLOOR_RADIUS) if is_finite(radius) else 0.0)
 if dungeon_active==active and dungeon_rooms==accepted_rooms and dungeon_floor_radii==accepted_radii:return
 dungeon_active=active
 dungeon_rooms=accepted_rooms
 dungeon_floor_radii=accepted_radii
 dungeon_current_room=-1
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
