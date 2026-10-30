extends "res://scripts/native_v34_3.gd"
## v34.4 Fair Fight: commander aggression, true flawless hits, cleaner battle feedback.
## Keeps a single world, unchanged character sprites, three invaders and zero survival timers.
const V344_RULES:=preload("res://scripts/systems/invasion_fairness_v344.gd")
var v344_revivals_disqualified:int=0
var v344_aggression_ticks:int=0

func _release_label()->String:return "v34.4 • Fair Fight"

func _apply_incoming_damage(raw:float)->void:
 # In v34.3 a lethal hit could heal via a Phoenix Feather before the old
 # health-difference check, incorrectly granting a no-hit boss reward.
 var feather_used_before:bool=phoenix_used_this_expedition
 var hits_before:int=v343_real_hits
 super._apply_incoming_damage(raw)
 if v341_active_site>=0 and v341_wave==3 and V344_RULES.recovered_during_hit(feather_used_before,phoenix_used_this_expedition,v343_real_hits>hits_before):
  v343_real_hits+=1
  v344_revivals_disqualified+=1

func _process(delta:float)->void:
 super._process(delta)
 _v361_animate(delta)
 # Higher commander phases attack more often, but NEVER shorten their marked
 # warning window. Inherited logic still owns hazard and invader budgets.
 if not hero_playing or death_active or menu_open or mounted or v341_active_site<0 or v341_wave!=3 or v342_phase!=0 or not is_instance_valid(player):return
 var site:int=v341_active_site
 if region_index!=int(v341_sites[site]["zone"]):return
 if player.global_position.distance_squared_to(v341_sites[site]["anchor"])>55.0*55.0:return
 var boss:Dictionary=_v343_commander()
 if boss.is_empty() or not bool(boss.get("alive",false)):return
 var extra:float=V344_RULES.rest_acceleration(v343_phase)
 if extra>0.0:
  v342_phase_clock+=clampf(delta,0.0,0.15)*extra
  v344_aggression_ticks+=1

func _v343_update_phase()->void:
 var previous:int=v343_phase
 super._v343_update_phase()
 if v343_phase>previous and v343_phase>1:
  _show_message("COMMANDER PHASE "+str(v343_phase)+": faster assaults, same visible dodge warning.",2.1)

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="Professions" and is_instance_valid(menu_body):
  _v362_add_profession_menu()
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]LIVING WATERS[/b]\nSix moonlit water sites, three gatherable resources per region, and shallow-water wading (18% slower). River reeds, fish and ore feed Cooking, Alchemy and Smithing. Resource nodes replenish on the next in-game day. No real-time survival clock."
  menu_body.text+="\n\n[b]FAIR FIGHT UPDATE[/b]\nIn later phases, commanders recover faster between attacks, but red, orange and violet warnings remain the same duration. Phoenix rescue means you survived, not a flawless no-hit victory. No survival stopwatch."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v34.4"
 report["aggression_ticks"]=v344_aggression_ticks
 report["revival_hit_corrections"]=v344_revivals_disqualified
 report["art_overhaul"]="v36.1 lunar world art"
 report["art_landmarks"]=v361_landmarks.size()
 report["art_rift_markers"]=v361_rift_emblems.size()
 report["art_new_character_assets"]=0
 report["water_overhaul"]="v36.2 visual water + professions"
 report["water_bodies"]=v362_water_roots.size()
 report["profession_sites"]=v362_water_sites.size()
 report["profession_skills"]=water_professions.size()
 return report


## v36 third-person 3D camera foundation. This is deliberately additive: no
## new game modes, no replacement of Z/Nyra's original sprite assets or saves.
const V36_CAMERA_RULES:=preload("res://scripts/systems/third_person_camera_v36.gd")
var v36_yaw:float=V36_CAMERA_RULES.DEFAULT_YAW
var v36_pitch:float=V36_CAMERA_RULES.DEFAULT_PITCH
var v36_distance:float=V36_CAMERA_RULES.DEFAULT_DISTANCE
var v36_dragging:bool=false
var v36_camera_clipped:bool=false
var v36_safe_follow_distance:float=V36_CAMERA_RULES.DEFAULT_DISTANCE

func _build_scene()->void:
 super._build_scene()
 if not is_instance_valid(camera):return
 camera.name="MoonfallThirdPersonPerspective"
 camera.projection=Camera3D.PROJECTION_PERSPECTIVE
 camera.fov=66.0
 camera.near=0.08
 camera.current=true
 _update_camera(1.0)

func _map_movement_to_camera(raw_direction:Vector3)->Vector3:
 if not is_instance_valid(camera) or camera.projection!=Camera3D.PROJECTION_PERSPECTIVE:
  return raw_direction
 var camera_move:Vector3=V36_CAMERA_RULES.move_relative(raw_direction,v36_yaw)
 return camera_move*0.82 if v362_is_wading else camera_move

func _v24_set_zoom(target:float)->void:
 v24_camera_size=clampf(target,12.0,31.0)
 v36_distance=V36_CAMERA_RULES.zoom_from_legacy(v24_camera_size)
 v24_zoom_events+=1

func _unhandled_input(event:InputEvent)->void:
 # WoW-style third-person zoom: wheel up approaches, wheel down retreats.
 # Only active during gameplay; UI scroll and menus remain unaffected.
 if event is InputEventMouseButton and event.pressed and hero_playing and not menu_open and not death_active:
  if event.button_index==MOUSE_BUTTON_WHEEL_UP or event.button_index==MOUSE_BUTTON_WHEEL_DOWN:
   var direction:float=-1.5 if event.button_index==MOUSE_BUTTON_WHEEL_UP else 1.5
   _v24_set_zoom(v24_camera_size+direction)
   get_viewport().set_input_as_handled()
   return
 if event is InputEventMouseButton and event.button_index==MOUSE_BUTTON_RIGHT:
  if not event.pressed:
   v36_dragging=false
  elif hero_playing and not menu_open and not death_active:
   v36_dragging=true
  if hero_playing and not menu_open:
   get_viewport().set_input_as_handled()
   return
 if event is InputEventMouseMotion and v36_dragging and hero_playing and not menu_open and not death_active:
  v36_yaw-=event.relative.x*0.006
  v36_pitch=V36_CAMERA_RULES.orbit_pitch(v36_pitch,event.relative.y*0.005)
  get_viewport().set_input_as_handled()
  return
 super._unhandled_input(event)

func _update_camera(delta:float)->void:
 if not is_instance_valid(camera) or not is_instance_valid(player):return
 if camera.projection!=Camera3D.PROJECTION_PERSPECTIVE:
  super._update_camera(delta)
  return
 # Right stick moves the camera, left stick moves the hero relative to it.
 # No forced cursor capture: the original click-to-attack and UI still work.
 if hero_playing and not menu_open and not death_active and delta<0.5:
  var pads:Array[int]=Input.get_connected_joypads()
  if not pads.is_empty():
   var right_x:float=Input.get_joy_axis(pads[0],JOY_AXIS_RIGHT_X)
   var right_y:float=Input.get_joy_axis(pads[0],JOY_AXIS_RIGHT_Y)
   if absf(right_x)>0.18:v36_yaw-=right_x*delta*2.1
   if absf(right_y)>0.18:v36_pitch=V36_CAMERA_RULES.orbit_pitch(v36_pitch,right_y*delta*1.65)
 var focus:Vector3=player.global_position+Vector3(0.0,1.35,0.0)
 var offset:Vector3=V36_CAMERA_RULES.camera_offset(v36_yaw,v36_pitch,v36_distance)
 var desired:Vector3=focus+offset
 # Physics owns the wall/terrain raycast; rendering reads the last safe result.
 # This avoids querying a potentially locked physics space during _process().
 desired=focus+offset.normalized()*minf(v36_safe_follow_distance,offset.length())
 var blend:float=1.0 if delta>=1.0 else 1.0-exp(-9.0*maxf(0.0,delta))
 camera.global_position=camera.global_position.lerp(desired,blend)
 camera.look_at(focus,Vector3.UP)

func _physics_process(delta:float)->void:
 # Shallow wading only; no separate swimming mode or survival statistic.
 v362_is_wading=false
 if v362_water_ready and is_instance_valid(player) and hero_playing:
  for zone in range(6):
   var center:Vector2=REGIONS[zone]["at"]
   var offset:Vector2=V362_JOBS.initial_water_offset(zone)
   var distance:Vector2=Vector2(player.global_position.x-center.x-offset.x,player.global_position.z-center.y-offset.y)
   if distance.length_squared()<19.5:
    v362_is_wading=true
    break
 super._physics_process(delta)
 _v36_trace_camera_obstacles()

func _v36_trace_camera_obstacles()->void:
 if not is_instance_valid(player) or not is_instance_valid(camera):return
 if camera.projection!=Camera3D.PROJECTION_PERSPECTIVE:return
 var focus:Vector3=player.global_position+Vector3(0.0,1.35,0.0)
 var offset:Vector3=V36_CAMERA_RULES.camera_offset(v36_yaw,v36_pitch,v36_distance)
 var space:PhysicsDirectSpaceState3D=get_world_3d().direct_space_state
 var ray:=PhysicsRayQueryParameters3D.create(focus,focus+offset)
 ray.exclude=[player.get_rid()]
 ray.collide_with_areas=false
 var hit:Dictionary=space.intersect_ray(ray)
 v36_camera_clipped=not hit.is_empty()
 v36_safe_follow_distance=V36_CAMERA_RULES.safe_distance(focus.distance_to(hit["position"])) if v36_camera_clipped else offset.length()


## v36.1 WORLD ART OVERHAUL. Atmosphere and props only: zero gameplay colliders.
const V361_ART:=preload("res://scripts/systems/lunar_art_palette_v36.gd")
const V361_SKY:Texture2D=preload("res://assets/v36_world_art/lunar_pink_moon_sky.png")
var v361_art_ready:bool=false
var v361_landmarks:Array[Node3D]=[]
var v361_rift_emblems:Array[MeshInstance3D]=[]
var v361_sanctuary_beacons:Array[Node3D]=[]
var v361_landmark_lights:Array[OmniLight3D]=[]
var v361_aura:MeshInstance3D
var v361_art_time:float=0.0
var v361_art_elapsed:float=0.0
var v361_material_cache:Dictionary={}
var v361_ring_mesh:TorusMesh
var v361_crystal_mesh:CylinderMesh
var v361_column_mesh:CylinderMesh
var v361_orb_mesh:SphereMesh

func _ready()->void:
 super._ready()
 _v361_build_world_art()
 _v362_build_water_and_resources()
 v361_art_ready=true
 _v361_art_quality()

func _v361_material(tint:Color,glowing:bool=false)->StandardMaterial3D:
 var key:String=tint.to_html()+("_light" if glowing else "_stone")
 if v361_material_cache.has(key):return v361_material_cache[key] as StandardMaterial3D
 var mat:=StandardMaterial3D.new()
 mat.albedo_color=tint
 mat.roughness=0.42 if glowing else 0.83
 mat.metallic=0.26 if glowing else 0.06
 if glowing:
  mat.emission_enabled=true
  mat.emission=tint
  mat.emission_energy_multiplier=1.65
  mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 v361_material_cache[key]=mat
 return mat

func _v361_primitive(root:Node3D,mesh:Mesh,mat:Material,at:Vector3,scale_at:Vector3=Vector3.ONE)->MeshInstance3D:
 var body:=MeshInstance3D.new()
 body.mesh=mesh
 body.material_override=mat
 body.position=at
 body.scale=scale_at
 body.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 root.add_child(body)
 return body

func _v361_region_signature(root:Node3D,zone:int,jewel:Color,stone:Color)->void:
 # Each zone has a distinct silhouette, with no physics, NPCs or gameplay triggers.
 match zone:
  0: # Bronze sanctuary: four hanging golden lanterns.
   for i in range(4):
    var a:float=float(i)*TAU/4.0+0.40
    _v361_primitive(root,v361_orb_mesh,_v361_material(Color("ffcc7a"),true),Vector3(cos(a)*2.05,2.1,sin(a)*2.05),Vector3(0.38,0.52,0.38))
  1: # Witchwood: leaning moonwood branches surrounding a green heart.
   for i in range(3):
    var branch:=_v361_primitive(root,v361_column_mesh,_v361_material(stone),Vector3(-1.6+float(i)*1.6,2.0,1.65),Vector3(0.33,1.7,0.33))
    branch.rotation_degrees.z=-27.0+float(i)*27.0
   _v361_primitive(root,v361_orb_mesh,_v361_material(jewel,true),Vector3(0.0,5.5,0.0),Vector3(0.50,0.7,0.50))
  2: # Marsh: suspended cold-blue will-o'-wisps.
   for i in range(5):
    var a:float=float(i)*TAU/5.0
    _v361_primitive(root,v361_orb_mesh,_v361_material(jewel,true),Vector3(cos(a)*2.75,1.9+0.5*float(i%3),sin(a)*2.75),Vector3(0.3,0.42,0.3))
  3: # Quarry: tilted ember shards fractured from the rock.
   for i in range(3):
    var shard:=_v361_primitive(root,v361_crystal_mesh,_v361_material(jewel,true),Vector3(-2.0+float(i)*2.0,1.2,2.5),Vector3(1.25,1.5,1.25))
    shard.rotation_degrees.z=-19.0+float(i)*19.0
  4: # Ruins: crossed moon arches suggest a shattered eclipse gate.
   for i in range(2):
    var arch:=_v361_primitive(root,v361_ring_mesh,_v361_material(jewel,true),Vector3(0.0,2.85,0.0),Vector3(2.4,2.4,2.4))
    arch.rotation_degrees=Vector3(90.0,float(i)*70.0,0.0)
  5: # Citadel: four high scarlet-black gothic spires.
   for i in range(4):
    var a:float=float(i)*TAU/4.0+0.785
    _v361_primitive(root,v361_column_mesh,_v361_material(stone),Vector3(cos(a)*2.85,2.0,sin(a)*2.85),Vector3(0.58,1.4,0.58))

func _v361_build_world_art()->void:
 if not is_instance_valid(world) or not is_instance_valid(player):return
 # Single resource pool, no per-frame allocations, only a few fixed landmarks.
 v361_ring_mesh=TorusMesh.new()
 v361_ring_mesh.inner_radius=0.93
 v361_ring_mesh.outer_radius=1.09
 v361_ring_mesh.rings=12
 v361_ring_mesh.ring_segments=28
 v361_crystal_mesh=CylinderMesh.new()
 v361_crystal_mesh.top_radius=0.0
 v361_crystal_mesh.bottom_radius=0.37
 v361_crystal_mesh.height=2.0
 v361_crystal_mesh.radial_segments=6
 v361_column_mesh=CylinderMesh.new()
 v361_column_mesh.top_radius=0.37
 v361_column_mesh.bottom_radius=0.53
 v361_column_mesh.height=3.1
 v361_column_mesh.radial_segments=8
 v361_orb_mesh=SphereMesh.new()
 v361_orb_mesh.radius=0.42
 v361_orb_mesh.height=0.84
 v361_orb_mesh.radial_segments=12
 v361_orb_mesh.rings=6
 # The in-game sky is a new original environment asset. No hero pixels edited.
 if living_environment!=null:
  var panorama:=PanoramaSkyMaterial.new()
  panorama.panorama=V361_SKY
  var sky:=Sky.new()
  sky.sky_material=panorama
  living_environment.sky=sky
  living_environment.background_mode=Environment.BG_SKY
  living_environment.reflected_light_source=Environment.REFLECTION_SOURCE_BG
 for zone in range(mini(6,region_nodes.size())):
  var root:=Node3D.new()
  root.name="V361_Art_"+str(V361_ART.REGION_NAMES[zone]).replace(" ","_")
  region_nodes[zone].add_child(root)
  var spot:Vector2=REGIONS[zone]["at"]
  # World-scaled, visual-only off-road monument at a safe decorative location.
  root.position=Vector3(spot.x+16.0,0.0,spot.y+16.0)
  var jewel:Color=V361_ART.COLORS[zone]
  var stone:Color=V361_ART.STONE[zone]
  _v361_primitive(root,v361_column_mesh,_v361_material(stone),Vector3(0.0,1.45,0.0),Vector3(1.65,0.95,1.65))
  _v361_primitive(root,v361_orb_mesh,_v361_material(jewel,true),Vector3(0.0,3.65,0.0),Vector3(1.4,1.4,1.4))
  for side in range(4):
   var a:float=float(side)*TAU/4.0
   var p:=Vector3(cos(a)*3.1,0.0,sin(a)*3.1)
   _v361_primitive(root,v361_crystal_mesh,_v361_material(jewel.darkened(0.23),true),p+Vector3(0.0,0.83,0.0),Vector3(1.0,0.83+float(side%2)*0.33,1.0))
  var halo:=_v361_primitive(root,v361_ring_mesh,_v361_material(jewel,true),Vector3(0.0,3.6,0.0),Vector3(1.75,1.75,1.75))
  halo.rotation_degrees.x=75.0
  var base_halo:=_v361_primitive(root,v361_ring_mesh,_v361_material(jewel.darkened(0.18)),Vector3(0.0,0.12,0.0),Vector3(3.8,3.8,3.8))
  base_halo.rotation_degrees=Vector3.ZERO
  _v361_region_signature(root,zone,jewel,stone)
  # One dim, unshadowed accent light per region, culled with region visibility.
  var glow:=OmniLight3D.new()
  glow.name="V361_RegionalAccentLight"
  glow.position=Vector3(0.0,3.6,0.0)
  glow.light_color=jewel
  glow.light_energy=0.75
  glow.omni_range=7.0
  glow.shadow_enabled=false
  root.add_child(glow)
  v361_landmark_lights.append(glow)
  v361_landmarks.append(root)
 # Shrine art reuses the existing destination and collision. It cannot block travel.
 for zone in range(mini(region_shrines.size(),6)):
  var shrine:=region_shrines[zone]
  if not is_instance_valid(shrine):continue
  var shrine_art:=Node3D.new()
  shrine_art.name="V361_MoonlitShrineCrest"
  shrine.add_child(shrine_art)
  var tint:Color=V361_ART.COLORS[zone]
  var crest:=_v361_primitive(shrine_art,v361_ring_mesh,_v361_material(tint,true),Vector3(0,2.65,0),Vector3(0.63,0.63,0.63))
  crest.rotation_degrees.x=90.0
  v361_sanctuary_beacons.append(shrine_art)
 # Existing rift gameplay anchors remain unchanged. Visible crests show their locations.
 for entry in rifts:
  var at:Vector3=entry["anchor"]
  var rift_root:=Node3D.new()
  rift_root.name="V361_RiftVisualOnly"
  world.add_child(rift_root)
  rift_root.position=at+Vector3(0.0,2.55,0.0)
  var ring:=_v361_primitive(rift_root,v361_ring_mesh,_v361_material(Color("b188ff"),true),Vector3.ZERO,Vector3(1.6,1.6,1.6))
  ring.rotation_degrees.x=90.0
  _v361_primitive(rift_root,v361_orb_mesh,_v361_material(Color("7d4cad"),true),Vector3.ZERO,Vector3(0.95,0.95,0.95))
  v361_rift_emblems.append(ring)
 # A ground aura emphasizes 3D positioning without editing the original Sprite3D.
 v361_aura=_v361_primitive(player,v361_ring_mesh,_v361_material(V361_ART.hero_tint(hero),true),Vector3(0.0,-0.50,0.0),Vector3(0.57,0.07,0.57))
 v361_aura.name="V361_OriginalHeroGroundAura"

func _v361_art_quality()->void:
 if not v361_art_ready:return
 var enabled:bool=V361_ART.details_enabled(gfx_settings,graphics_quality)
 for root in v361_landmarks:
  if is_instance_valid(root):root.visible=enabled
 for glow in v361_landmark_lights:
  if is_instance_valid(glow):glow.visible=enabled
 for crest in v361_sanctuary_beacons:
  if is_instance_valid(crest):crest.visible=enabled
 # Never hide rift locations or combat interaction cues at low settings.
 for ring in v361_rift_emblems:
  if is_instance_valid(ring):ring.visible=true
 if is_instance_valid(v361_aura):v361_aura.visible=hero_playing and enabled

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 if v361_art_ready:_v361_art_quality()
 if v362_water_ready:_v362_apply_water_quality()

func _v361_animate(delta:float)->void:
 if not v361_art_ready:return
 # Bounded 10Hz art updates. Gameplay, physics and AI are not touched.
 v361_art_elapsed+=delta
 if v361_art_elapsed<0.1:return
 var elapsed:float=v361_art_elapsed
 v361_art_elapsed=0.0
 v361_art_time+=elapsed
 if is_instance_valid(v361_aura):
  v361_aura.visible=hero_playing and not death_active and V361_ART.details_enabled(gfx_settings,graphics_quality)
  v361_aura.material_override=_v361_material(V361_ART.hero_tint(hero),true)
  v361_aura.scale=Vector3(0.57+0.025*sin(v361_art_time*2.2),0.07,0.57+0.025*sin(v361_art_time*2.2))
 for index in range(v361_rift_emblems.size()):
  var ring:=v361_rift_emblems[index]
  if not is_instance_valid(ring):continue
  ring.rotation_degrees.y+=elapsed*11.0
  if index<rifts.size():
   var sealed:bool=bool(rifts[index].get("cleared",false))
   ring.material_override=_v361_material(Color("8fe3c8") if sealed else Color("b188ff"),true)



## v36.2 LIVING WATERS AND PROFESSIONS. Original Z/Nyra imagery is never modified.
const V362_JOBS:=preload("res://scripts/systems/water_professions_v362.gd")
var v362_water_ready:bool=false
var v362_water_roots:Array[Node3D]=[]
var v362_water_sites:Array[Dictionary]=[]
var v362_water_shader:ShaderMaterial
var v362_water_ambient:float=0.0
var v362_is_wading:bool=false
var v362_water_visible:int=0

func _v362_make_material(color:Color,emissive:bool=false)->StandardMaterial3D:
 return _v361_material(color,emissive)

func _v362_build_water_and_resources()->void:
 if not is_instance_valid(world) or region_nodes.size()<6:return
 # GPU-animated shallows; no CPU fluid simulation, expensive physics, or colliders.
 var water_shader:=Shader.new()
 water_shader.code="""shader_type spatial;
render_mode blend_mix, cull_disabled, shadows_disabled;
uniform vec4 shallow_color : source_color = vec4(0.12,0.57,0.67,1.0);
uniform vec4 deep_color : source_color = vec4(0.04,0.12,0.28,1.0);
void fragment() {
 float wave = sin(UV.x*21.0 + TIME*0.95)*cos(UV.y*19.0 - TIME*0.82);
 float glimmer = sin((UV.x+UV.y)*51.0 + TIME*1.35)*0.035;
 float shade = clamp(0.34 + wave*0.19 + glimmer,0.0,1.0);
 ALBEDO=mix(shallow_color.rgb, deep_color.rgb, shade);
 ROUGHNESS=0.16;
 METALLIC=0.12;
 ALPHA=0.84;
}
"""
 v362_water_shader=ShaderMaterial.new()
 v362_water_shader.shader=water_shader
 var plane:=PlaneMesh.new()
 plane.size=Vector2(1.0,1.0)
 var reeds:=CylinderMesh.new()
 reeds.top_radius=0.05
 reeds.bottom_radius=0.12
 reeds.height=1.35
 reeds.radial_segments=5
 var rock:=SphereMesh.new()
 rock.radius=0.42
 rock.height=0.84
 rock.radial_segments=8
 rock.rings=5
 var fish_mesh:=SphereMesh.new()
 fish_mesh.radius=0.27
 fish_mesh.height=0.54
 fish_mesh.radial_segments=8
 fish_mesh.rings=4
 for zone in range(6):
  var center:Vector2=REGIONS[zone]["at"]
  var offset:Vector2=V362_JOBS.initial_water_offset(zone)
  var sx:float=center.x+offset.x
  var sz:float=center.y+offset.y
  var water_root:=Node3D.new()
  water_root.name="V362_Water_"+str(zone)
  region_nodes[zone].add_child(water_root)
  var lake_scale:Vector3=Vector3(10.2 if zone==2 else 7.8,1.0,7.0 if zone==2 else 5.8)
  var region_water:=v362_water_shader.duplicate() as ShaderMaterial
  var water_hues:Array[Color]=[Color("517fbb"),Color("468e8d"),Color("3e91a5"),Color("667c96"),Color("6665a4"),Color("8d527e")]
  var hue:Color=water_hues[zone]
  region_water.set_shader_parameter("shallow_color",hue)
  var lake:=_v361_primitive(water_root,plane,region_water,Vector3(sx,0.069,sz),lake_scale)
  lake.name="V362_AnimatedWaterSurface"
  var shore_mat:=StandardMaterial3D.new()
  shore_mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
  shore_mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
  shore_mat.albedo_color=Color(hue.r,hue.g,hue.b,0.45)
  shore_mat.roughness=0.45
  var shallow:=_v361_primitive(water_root,plane,shore_mat,Vector3(sx,0.051,sz),lake_scale*1.11)
  shallow.name="V362_ShorelineGradient"
  # Visible shore stones, reeds, lilies and rune markers: fixed small budget.
  for detail in range(6):
   var angle:float=TAU*float(detail)/6.0
   var at:Vector3=Vector3(sx+cos(angle)*6.4,0.0,sz+sin(angle)*4.8)
   if detail%2==0:
    _v361_primitive(water_root,reeds,_v362_make_material(Color("56896b")),at+Vector3(0,0.54,0),Vector3(1.0,0.86,1.0))
   else:
    _v361_primitive(water_root,rock,_v362_make_material(Color("727b87")),at+Vector3(0,0.14,0),Vector3(1.20,0.52,0.90))
  var site_positions:=[Vector3(sx,0.25,sz+4.35),Vector3(sx-6.3,0.25,sz),Vector3(sx+6.3,0.25,sz)]
  for slot in range(3):
   var kind:String=V362_JOBS.harvest_kind(slot)
   var site:=Node3D.new()
   site.name="V362_"+kind+"_"+str(zone)
   site.position=site_positions[slot]
   water_root.add_child(site)
   var mesh:Mesh=fish_mesh if slot==0 else (reeds if slot==1 else rock)
   var tint:Color=Color("74c5e9") if slot==0 else (Color("80c69b") if slot==1 else Color("d7b483"))
   var shape_scale:Vector3=Vector3(1.4,0.85,1.4) if slot==0 else Vector3.ONE
   _v361_primitive(site,mesh,_v362_make_material(tint,true),Vector3(0,0.42,0),shape_scale)
   var marker:=Label3D.new()
   marker.text=kind+"  [G]"
   marker.font_size=36
   marker.pixel_size=0.007
   marker.billboard=BaseMaterial3D.BILLBOARD_ENABLED
   marker.modulate=tint
   marker.position=Vector3(0.0,1.32,0.0)
   site.add_child(marker)
   v362_water_sites.append({"id":zone*3+slot,"zone":zone,"kind":kind,"node":site,"label":marker})
  v362_water_roots.append(water_root)
 v362_water_ready=true
 _v362_apply_water_quality()

func _v362_apply_water_quality()->void:
 if not v362_water_ready:return
 # Essential water and resource markers remain visible even on Smooth.
 # Region streaming owns visibility of water roots as descendants.
 v362_water_visible=v362_water_roots.size()

func _v362_nearest_site()->Dictionary:
 if not is_instance_valid(player):return {}
 var nearest:Dictionary={}
 var best:float=3.4*3.4
 for site in v362_water_sites:
  var node:Node3D=site["node"]
  if not is_instance_valid(node):continue
  var distance_sq:float=player.global_position.distance_squared_to(node.global_position)
  if distance_sq<best:
   nearest=site
   best=distance_sq
 return nearest

func _v362_gather()->bool:
 if not hero_playing or death_active or mounted or _combat_threat():return false
 var site:Dictionary=_v362_nearest_site()
 if site.is_empty():return false
 var site_id:int=int(site["id"])
 var kind:String=str(site["kind"])
 if not V362_JOBS.can_gather(site_id,world_day,water_site_days):
  _show_message("This "+kind.to_lower()+" resource returns next in-game day.",2.0)
  return true
 var yields:Dictionary=V362_JOBS.gather_yield(kind,int(water_professions.get(kind,1)),rng.randi())
 for ingredient in yields.keys():
  _gain_material(str(ingredient),int(yields[ingredient]))
 water_site_days[str(site_id)]=world_day
 water_professions[kind]=mini(300,int(water_professions.get(kind,1))+1)
 _show_message(kind+" gathered: "+str(yields)+" • Skill "+str(water_professions[kind])+"/300",2.3)
 _save_game()
 return true

func _interact()->void:
 # Priority to existing rifts/world objectives when no gather site is close.
 if _v362_gather():return
 super._interact()

func _v362_add_profession_menu()->void:
 menu_body.text+="\n\n[b]WATER & LAND PROFESSIONS[/b]\nGather with G at shore markers, no combat, no building. Sites replenish next world day.\n"
 for job in V362_JOBS.JOBS:
  menu_body.text+=str(job)+" "+str(int(water_professions.get(job,1)))+" / 300  •  "
 menu_body.text+="\n[b]COOKING · ALCHEMY · SMITHING[/b]\n"
 for index in range(V362_JOBS.RECIPES.size()):
  var recipe:Dictionary=V362_JOBS.RECIPES[index]
  var issue:String=V362_JOBS.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
  menu_body.text+="\n"+str(recipe["name"])+" ("+str(recipe["job"])+") • "+str(recipe["cost"])+( " ["+issue+"]" if issue!="" else " [READY]")
  _atlas_action("MAKE: "+str(recipe["name"]),"v362:craft:"+str(index),"Uses your existing material storage")
 menu_body.text+="\n\n[b]FIELD FOOD & POTIONS[/b]\nMoonfish Stew heals 25% HP. Moonwater Tonic heals 50% HP. Neither adds a new gameplay meter."
 _atlas_action("EAT MOONFISH STEW (25% HP)","v362:use:Moonfish Stew")
 _atlas_action("DRINK MOONWATER TONIC (50% HP)","v362:use:Moonwater Tonic")

func _atlas_execute(action:String)->void:
 if action.begins_with("v362:craft:"):
  _v362_craft(int(action.get_slice(":",2)))
  _menu_tab("Professions")
  return
 if action.begins_with("v362:use:"):
  _v362_consume(action.trim_prefix("v362:use:"))
  _menu_tab("Professions")
  return
 super._atlas_execute(action)

func _v362_craft(index:int)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Craft in safety, while on foot.",2.0)
  return
 if index<0 or index>=V362_JOBS.RECIPES.size():return
 var recipe:Dictionary=V362_JOBS.RECIPES[index]
 var issue:String=V362_JOBS.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
 if issue!="":
  _show_message(issue,1.8)
  return
 V362_JOBS.consume_recipe(recipe,materials)
 if int(recipe["power"])>0:
  gear_items.append({"name":recipe["result"],"rarity":"Crafted","power":recipe["power"]})
 else:
  _gain_material(str(recipe["result"]),1)
 var job:String=str(recipe["job"])
 water_professions[job]=mini(300,int(water_professions.get(job,1))+1)
 _show_message("Crafted "+str(recipe["name"])+"! "+job+" "+str(water_professions[job])+"/300",2.1)
 _save_game()

func _v362_consume(item:String)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Cannot eat or drink during combat or while mounted.",1.6)
  return
 var fraction:float=0.25 if item=="Moonfish Stew" else (0.50 if item=="Moonwater Tonic" else 0.0)
 if fraction<=0.0:return
 if hp>=max_hp:
  _show_message("Health is already full.",1.4)
  return
 if not INVENTORY_RULES.consume_one(materials,item):
  _show_message("No "+item+" available.",1.6)
  return
 hp=minf(max_hp,hp+max_hp*fraction)
 _show_message("Used "+item+". Health restored.",1.6)
 _save_game()

