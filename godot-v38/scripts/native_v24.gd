extends "res://scripts/native_v23.gd"
## v24 full-system quality pass. Shared visual pools, adaptive decoration LOD,
## fair elite attack markers, accessible camera zoom and readable combat feedback.
## Damage, hitboxes, saves, source textures and progression remain unchanged.
const V24_POPUP_LIMIT := 18
const V24_MARKER_LIMIT := 8
const V24_LOD_REFRESH := 0.45
const V24_POPUP_LIFE := 0.85
const V24_SCENERY_DISTANCE_SQ := 32.0 * 32.0
const V24_ELITE_WARNING_COLOR := Color(1.0, 0.33, 0.26, 0.62)
var v24_popups: Array[Label3D] = []
var v24_popup_time: Array[float] = []
var v24_popup_index := 0
var v24_elite_markers: Array[MeshInstance3D] = []
var v24_marker_count := 0
var v24_lod_clock := 0.0
var v24_dioramas_visible := 0
var v24_diorama_cache: Array[Node3D] = []
var v24_damage_events := 0
var v24_camera_size := 19.0
var v24_zoom_events := 0
var v24_status_clock := 0.0
var v24_status_label: Label
var v24_elite_mesh: CylinderMesh
var v24_elite_mat: StandardMaterial3D

func _ready() -> void:
 super._ready()
 _v24_build_pools()
 _v24_update_lod()
 _show_message("Moonfall v24: combat clarity and world optimization", 3.0)

func _v24_build_pools() -> void:
 v24_elite_mesh=CylinderMesh.new()
 v24_elite_mesh.top_radius=1.0
 v24_elite_mesh.bottom_radius=1.0
 v24_elite_mesh.height=0.045
 v24_elite_mesh.radial_segments=40
 v24_elite_mat=StandardMaterial3D.new()
 v24_elite_mat.albedo_color=V24_ELITE_WARNING_COLOR
 v24_elite_mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 v24_elite_mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 v24_elite_mat.cull_mode=BaseMaterial3D.CULL_DISABLED
 for i in range(V24_MARKER_LIMIT):
  var marker:=MeshInstance3D.new()
  marker.name="V24_EliteWarning_%02d" % i
  marker.mesh=v24_elite_mesh
  marker.material_override=v24_elite_mat
  marker.visible=false
  world.add_child(marker)
  v24_elite_markers.append(marker)
 for i in range(V24_POPUP_LIMIT):
  var label:=Label3D.new()
  label.name="V24_FloatingDamage_%02d" % i
  label.font_size=48
  label.pixel_size=0.006
  label.modulate=Color(1.0,0.92,0.72)
  label.outline_modulate=Color(0.04,0.025,0.07)
  label.outline_size=12
  label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  label.no_depth_test=true
  label.visible=false
  world.add_child(label)
  v24_popups.append(label)
  v24_popup_time.append(0.0)
 # Dioramas are static after world generation. Cache only those; preserve landmarks.
 v24_diorama_cache.clear()
 for region_root in v21_region_roots:
  if not is_instance_valid(region_root):continue
  for item in region_root.get_children():
   if item is Node3D and item.name.begins_with("V21_Diorama_"):
    v24_diorama_cache.append(item)
 if is_instance_valid(camera):v24_camera_size=camera.size
 var layer:=CanvasLayer.new()
 layer.name="V24_AccessibleCombatHUD"
 layer.layer=3
 add_child(layer)
 v24_status_label=Label.new()
 v24_status_label.name="V24_ThreatAndPerformance"
 v24_status_label.anchor_left=1.0
 v24_status_label.anchor_right=1.0
 v24_status_label.offset_left=-286
 v24_status_label.offset_right=-14
 v24_status_label.offset_top=277
 v24_status_label.offset_bottom=328
 v24_status_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_RIGHT
 v24_status_label.add_theme_font_size_override("font_size",15)
 v24_status_label.add_theme_color_override("font_color",Color("ffe1c4"))
 v24_status_label.add_theme_color_override("font_shadow_color",Color(0,0,0,0.95))
 v24_status_label.add_theme_constant_override("shadow_offset_x",1)
 v24_status_label.add_theme_constant_override("shadow_offset_y",2)
 v24_status_label.mouse_filter=Control.MOUSE_FILTER_IGNORE
 layer.add_child(v24_status_label)

func _v24_set_zoom(target:float) -> void:
 v24_camera_size=clampf(target,14.0,30.0)
 if is_instance_valid(camera):camera.size=v24_camera_size
 v24_zoom_events+=1

func _unhandled_input(event:InputEvent) -> void:
 if hero_playing and not menu_open and event is InputEventMouseButton and event.pressed:
  if event.button_index==MOUSE_BUTTON_WHEEL_UP:
   _v24_set_zoom(v24_camera_size-1.5)
   get_viewport().set_input_as_handled()
   return
  if event.button_index==MOUSE_BUTTON_WHEEL_DOWN:
   _v24_set_zoom(v24_camera_size+1.5)
   get_viewport().set_input_as_handled()
   return
 super._unhandled_input(event)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0) -> void:
 var before:=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 var amount:=maxf(0.0,before-float(target.get("hp",0.0)))
 if amount<=0.0 or not hero_playing or graphics_quality=="Smooth":return
 var body:CharacterBody3D=target.get("body")
 if not is_instance_valid(body) or not body.visible:return
 if not is_instance_valid(player) or body.global_position.distance_squared_to(player.global_position)>36.0*36.0:return
 var index:int=v24_popup_index%V24_POPUP_LIMIT
 v24_popup_index+=1
 var label:Label3D=v24_popups[index]
 label.text=str(ceili(amount))
 label.font_size=58 if last_hit_critical else 48
 label.position=body.global_position+Vector3(0,2.45 if last_hit_critical else 2.2,0)
 label.modulate=Color(1.0,0.74,0.25) if last_hit_critical else Color(0.92,0.95,1.0)
 label.visible=graphics_quality!="Smooth"
 v24_popup_time[index]=V24_POPUP_LIFE
 v24_damage_events+=1

func _v24_sync_elite_markers() -> void:
 # Mark exactly the position the base AI will check when the windup expires.
 # Reuse meshes rather than allocating nodes during battle.
 var used:=0
 if hero_playing and not menu_open:
  for beast in zone_beast_lists[region_index]:
   if used>=V24_MARKER_LIMIT:break
   if not bool(beast.get("alive",false)) or float(beast.get("elite_lunge_windup",0.0))<=0.0:continue
   var target:Vector3=beast.get("elite_lunge_target",Vector3.ZERO)
   var marker:MeshInstance3D=v24_elite_markers[used]
   marker.position=Vector3(target.x,0.075,target.z)
   marker.scale=Vector3(1.85,1.0,1.85)
   marker.visible=true
   used+=1
 for i in range(used,V24_MARKER_LIMIT):v24_elite_markers[i].visible=false
 v24_marker_count=used

func _v24_update_lod() -> void:
 # No per-tick tree scan: only static optional decorative nodes are cached.
 v24_dioramas_visible=0
 if not is_instance_valid(player):return
 var player_pos:Vector3=player.global_position
 for item in v24_diorama_cache:
  if not is_instance_valid(item):continue
  var parent_region:Node3D=item.get_parent() as Node3D
  var nearby:bool=item.global_position.distance_squared_to(player_pos)<=V24_SCENERY_DISTANCE_SQ
  var enabled:bool=graphics_quality!="Smooth" and nearby and is_instance_valid(parent_region) and parent_region.is_visible_in_tree()
  item.visible=enabled
  if enabled:v24_dioramas_visible+=1

func _physics_process(delta:float) -> void:
 super._physics_process(delta)
 _v24_sync_elite_markers()

func _process(delta:float) -> void:
 super._process(delta)
 var step:=clampf(delta,0.0,0.2)
 for i in range(v24_popups.size()):
  if v24_popup_time[i]<=0.0:continue
  if graphics_quality=="Smooth":
   v24_popup_time[i]=0.0
   v24_popups[i].visible=false
   continue
  v24_popup_time[i]=maxf(0.0,v24_popup_time[i]-step)
  var label:Label3D=v24_popups[i]
  if v24_popup_time[i]<=0.0:label.visible=false
  else:
   label.position.y+=step*0.9
   if graphics_quality=="Smooth":label.visible=false
 v24_lod_clock+=step
 if v24_lod_clock>=V24_LOD_REFRESH:
  v24_lod_clock=fmod(v24_lod_clock,V24_LOD_REFRESH)
  _v24_update_lod()
 v24_status_clock+=step
 if v24_status_clock>=0.20:
  v24_status_clock=fmod(v24_status_clock,0.20)
  if is_instance_valid(v24_status_label):
   v24_status_label.visible=hero_playing and not menu_open
   if v24_status_label.visible:
    var danger:=0
    for creature in zone_beast_lists[region_index]:
     if bool(creature.get("alive",false)) and bool(creature.get("engaged",false)):danger+=1
    var state: String="CLEAR" if danger==0 else "THREAT %d" % danger
    if hp/maxf(1.0,max_hp)<=0.25:state="LOW HEALTH | "+state
    v24_status_label.text=state+"  •  "+str(Engine.get_frames_per_second())+" FPS\nRight-drag: orbit  |  Wheel: zoom  |  Esc: Atlas"

func _apply_graphics_quality(mode:String) -> void:
 super._apply_graphics_quality(mode)
 if v24_elite_markers.size()>0:_v24_sync_elite_markers()
 if v24_popups.size()>0 and graphics_quality=="Smooth":
  for label in v24_popups:label.visible=false
 _v24_update_lod()

func _reset_new_adventure() -> void:
 super._reset_new_adventure()
 for i in range(v24_popup_time.size()):
  v24_popup_time[i]=0.0
  v24_popups[i].visible=false
 for marker in v24_elite_markers:marker.visible=false
 v24_marker_count=0
