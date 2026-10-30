extends "res://scripts/native_v37_2.gd"
## v37.3 Deep Waters: bounded three-axis swimming, actual below-surface grottos,
## and six discoverable caches in the SAME world. No new mode or hero art edits.
const V373_RULES:=preload("res://scripts/systems/deep_water_v373.gd")
var v373_depth:float=0.0
var v373_original_collision_mask:int=1
var v373_caves:Array[Node3D]=[]
var v373_cache_positions:Array[Vector3]=[]
var v373_cache_visuals:Array[MeshInstance3D]=[]
var v373_underwater_tint:ColorRect
var v373_bubbles:Array[MeshInstance3D]=[]
var v373_visual_elapsed:float=0.0

func _ready()->void:
 super._ready()
 if is_instance_valid(player):v373_original_collision_mask=player.collision_mask
 _v373_build_grottos()
 _v373_build_underwater_overlay()

func _start_game(selected:String)->void:
 _v373_reset_depth()
 super._start_game(selected)

func _v373_reset_depth()->void:
 v373_depth=0.0
 if is_instance_valid(player):
  player.collision_mask=v373_original_collision_mask
  player.position.y=V373_RULES.PLAYER_SURFACE_Y

func _v372_dive_held()->bool:
 # Breathing follows real submersion even after the dive button is released.
 return super._v372_dive_held() or V373_RULES.truly_submerged(v373_depth)

func _physics_process(delta:float)->void:
 # Keep land and cave collision ACTIVE during the inherited CharacterBody3D slide.
 # Turning the mask off underwater let the hero swim through cave walls.
 var prior_water_position:Vector3=player.global_position if is_instance_valid(player) else Vector3.ZERO
 if is_instance_valid(player):player.collision_mask=v373_original_collision_mask
 super._physics_process(delta)
 if not is_instance_valid(player):return
 var can_swim:bool=hero_playing and not death_active and not mounted and v37_swimming
 if not menu_open:
  # If a frame of lateral motion crosses the lake volume boundary while deep,
  # restore the last valid X/Z. Depth must remain inside connected lake or tunnel.
  if v373_depth>V373_RULES.SUBMERGED_DEPTH and can_swim:
   var reachable:int=V37_RULES.swim_zone(player.global_position.x,player.global_position.z,REGIONS,player.global_position.y)
   if reachable<0:
    player.position.x=prior_water_position.x
    player.position.z=prior_water_position.z
  var descending:bool=can_swim and _v373_descend_held()
  var ascending:bool=can_swim and _v373_ascend_held()
  var requested:float=V373_RULES.advance_depth(v373_depth,delta,can_swim,descending,ascending)
  var target_y:float=V373_RULES.PLAYER_SURFACE_Y-requested
  var vertical_motion:float=target_y-player.position.y
  if absf(vertical_motion)>0.0001:
   # Collision-aware motion stops at the grotto roof or floor instead of
   # teleporting the CharacterBody through solid geometry.
   player.move_and_collide(Vector3(0.0,vertical_motion,0.0))
  v373_depth=clampf(V373_RULES.PLAYER_SURFACE_Y-player.position.y,0.0,V373_RULES.MAX_DEPTH)
  # Leaving a swim zone must never strand a hero beneath the terrain.
  if not can_swim and v373_depth>0.0 and v37_swim_zone<0:
   _v373_reset_depth()
 if not hero_playing or death_active:
  _v373_reset_depth()
 _v373_refresh_overlay()

func _v373_descend_held()->bool:
 if Input.is_key_pressed(KEY_C):return true
 for pad in Input.get_connected_joypads():
  if Input.is_joy_button_pressed(pad,JOY_BUTTON_RIGHT_STICK):return true
 return false

func _v373_ascend_held()->bool:
 if Input.is_key_pressed(KEY_SPACE):return true
 for pad in Input.get_connected_joypads():
  if Input.is_joy_button_pressed(pad,JOY_BUTTON_A):return true
 return false

func _v373_build_grottos()->void:
 if not v362_water_ready or region_nodes.size()<V373_RULES.REGION_COUNT:return
 var stone:=BoxMesh.new()
 stone.size=Vector3.ONE
 var pearl:=SphereMesh.new()
 pearl.radius=0.31
 pearl.height=0.62
 pearl.radial_segments=9
 pearl.rings=5
 var rock_mat:=_v361_material(Color("283947"))
 var floor_mat:=_v361_material(Color("284b55"))
 var treasure_mat:=_v361_material(Color("92e5e3"),true)
 var bubble_mat:=_v361_material(Color("78c9df"),true)
 for zone in range(V373_RULES.REGION_COUNT):
  var mid:Vector2=REGIONS[zone]["at"]
  var offset:Vector2=V362_JOBS.initial_water_offset(zone)
  var cx:float=mid.x+offset.x
  var cz:float=mid.y+offset.y
  # Replace the visual ground tile under this lake with four border strips.
  # This creates an actual visible opening to the water/cavern below.
  _v373_open_ground(zone,offset)
  var root:=Node3D.new()
  root.name="V373_DeepGrotto_%d"%zone
  region_nodes[zone].add_child(root)
  v373_caves.append(root)
  # Submerged lake bed, then a small accessible rock tunnel and chamber.
  _v361_primitive(root,stone,floor_mat,Vector3(cx+1.6,-3.72,cz),Vector3(12.2,0.28,6.7))
  for post in range(4):
   var tx:float=cx+2.3+float(post)*1.13
   for side in [-1,1]:
    var z:float=cz+float(side)*1.38
    _v361_primitive(root,stone,rock_mat,Vector3(tx,-2.60,z),Vector3(0.40,2.2,0.40))
  _v361_primitive(root,stone,rock_mat,Vector3(cx+4.35,-0.90,cz),Vector3(4.75,0.42,3.2))
  for side in [-1,1]:
   _v361_primitive(root,stone,rock_mat,Vector3(cx+6.12,-2.5,cz+float(side)*1.27),Vector3(0.5,2.5,0.40))
  var treasure_pos:=Vector3(cx+5.35,-2.6,cz)
  var cache:=_v361_primitive(root,pearl,treasure_mat,treasure_pos,Vector3(1.0,1.0,1.0))
  cache.name="V373_AbyssalCache_%d"%zone
  v373_cache_visuals.append(cache)
  v373_cache_positions.append(treasure_pos)
  # Two bubbles per cave; no particle emitters, no collision or extra lights.
  for bubble_index in range(2):
   var bubble:=_v361_primitive(root,pearl,bubble_mat,Vector3(cx+2.6+float(bubble_index)*1.6,-2.8,cz+0.65),Vector3(0.13,0.13,0.13))
   v373_bubbles.append(bubble)

func _v373_open_ground(zone:int,offset:Vector2)->void:
 var ground:Node3D=region_nodes[zone].get_node_or_null("RegionGround") as Node3D
 if not is_instance_valid(ground):return
 var original:MeshInstance3D=ground.get_child(0) as MeshInstance3D
 if not is_instance_valid(original):return
 var material:Material=original.material_override
 original.visible=false
 var half:float=29.75
 var left:float=offset.x-4.55
 var right:float=offset.x+7.2
 var north:float=offset.y-3.8
 var south:float=offset.y+3.8
 var sections:Array[Vector4]=[
  Vector4(-half,-half,left+half,2.0*half),
  Vector4(right,-half,half-right,2.0*half),
  Vector4(left,-half,right-left,north+half),
  Vector4(left,south,right-left,half-south)
 ]
 for i in range(sections.size()):
  var rect:Vector4=sections[i]
  if rect.z<=0.0 or rect.w<=0.0:continue
  var mesh:=MeshInstance3D.new()
  mesh.name="V373_GroundBorder_%d"%i
  var section_mesh:=BoxMesh.new()
  section_mesh.size=Vector3(rect.z,0.025,rect.w)
  mesh.mesh=section_mesh
  mesh.material_override=material
  mesh.position=Vector3(rect.x+rect.z*0.5,0.0,rect.y+rect.w*0.5)
  ground.add_child(mesh)

func _v373_build_underwater_overlay()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v373_underwater_tint=ColorRect.new()
 v373_underwater_tint.name="V373_SubmergedTint"
 v373_underwater_tint.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 v373_underwater_tint.color=Color(0.02,0.30,0.42,0.16)
 v373_underwater_tint.mouse_filter=Control.MOUSE_FILTER_IGNORE
 v373_underwater_tint.visible=false
 layer.add_child(v373_underwater_tint)
 layer.move_child(v373_underwater_tint,0)

func _v373_refresh_overlay()->void:
 if is_instance_valid(v373_underwater_tint):
  v373_underwater_tint.visible=hero_playing and V373_RULES.truly_submerged(v373_depth) and not menu_open

func _process(delta:float)->void:
 super._process(delta)
 v373_visual_elapsed+=minf(delta,0.15)
 if v373_visual_elapsed<0.15:return
 v373_visual_elapsed=0.0
 # Only current region updates its decoration; cache respawns on world-day turn.
 for zone in range(v373_caves.size()):
  if not is_instance_valid(v373_caves[zone]):continue
  var active:bool=hero_playing and zone==region_index
  if is_instance_valid(v373_cache_visuals[zone]):
   v373_cache_visuals[zone].visible=active and V373_RULES.cache_ready(zone,world_day,water_site_days)

func _interact()->void:
 if hero_playing and not menu_open and not death_active and v37_swim_zone>=0 and v373_depth>=V373_RULES.CAVE_MIN_DEPTH:
  var zone:int=v37_swim_zone
  if zone<v373_cache_positions.size() and V373_RULES.cache_ready(zone,world_day,water_site_days):
   if V373_RULES.in_cache_range(player.global_position,v373_cache_positions[zone],v373_depth):
    water_site_days[str(V373_RULES.CACHE_SLOT_START+zone)]=world_day
    _gain_material("Moon Sigil",1)
    _gain_material("Ore Shard",2)
    _show_message("Abyssal cache discovered! Moon Sigil +2 Ore Shards. Returns tomorrow.",2.7)
    _save_game()
    return
 super._interact()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]DEEP WATERS v37.3[/b]\nDive with C/right-stick press; SPACE/controller A rises quickly. Release dive for a gentle ascent. Oxygen drains while submerged. Swim east along the lake bottom into six grottos, each with a replenishing abyssal cache. Surfacing restores land collisions."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["swim_depth"]=v373_depth
 report["deep_grottos"]=v373_caves.size()
 report["underwater_cache_sites"]=v373_cache_positions.size()
 report["underwater"] =V373_RULES.truly_submerged(v373_depth)
 return report
