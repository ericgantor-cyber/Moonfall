extends "res://scripts/native_v34_4.gd"
## Living World: moonlit shoreline effects, responsive plants and fish, taint and
## purification reflecting existing rift/invasion state, useful profession gear.
## 3D decorations have NO collision and spawn only once, six zones total.
const V363_RULES:=preload("res://scripts/systems/living_world_v363.gd")
var v363_ready:bool=false
var v363_water_materials:Array[ShaderMaterial]=[]
var v363_flora:Array[Dictionary]=[]
var v363_fish:Array[Dictionary]=[]
var v363_cascades:Array[MeshInstance3D]=[]
var v363_taint:Array[bool]=[]
var v363_update_elapsed:float=0.0
var v363_visual_time:float=0.0
var v363_applied_night_glow:float=-1.0
var v363_ward_seconds:float=0.0
var v363_fish_updates:int=0
var v363_flora_updates:int=0

func _ready()->void:
 super._ready()
 _v363_build_living_world()
 v363_ready=v363_water_materials.size()==V363_RULES.WATER_ZONE_COUNT
 _v363_refresh_quality()
 _v363_tick(0.0)

func _v363_build_living_world()->void:
 if not v362_water_ready or v362_water_roots.size()!=V363_RULES.WATER_ZONE_COUNT:return
 var shader:=Shader.new()
 shader.code="""shader_type spatial;
render_mode blend_mix, cull_disabled, shadows_disabled;
uniform vec4 clear_color : source_color = vec4(0.25,0.55,0.67,1.0);
uniform vec4 deep_color : source_color = vec4(0.03,0.13,0.27,1.0);
uniform float corrupted = 0.0;
uniform float night_glow = 0.0;
void fragment() {
 vec2 p=UV;
 float wave=sin(p.x*26.0+TIME*0.9)*cos(p.y*20.0-TIME*0.75);
 float ripples=sin(length(p-vec2(0.5))*38.0-TIME*2.4)*0.035;
 float edge=min(min(p.x,1.0-p.x),min(p.y,1.0-p.y));
 float foam=(1.0-smoothstep(0.014,0.085,edge))*(0.60+0.25*sin((p.x+p.y)*48.0+TIME*1.7));
 float dark=clamp(0.35+wave*0.19+ripples,0.0,1.0);
 vec3 water=mix(clear_color.rgb,deep_color.rgb,dark);
 vec3 taint=mix(vec3(0.18,0.06,0.25),vec3(0.40,0.09,0.45),dark);
 water=mix(water,taint,clamp(corrupted,0.0,1.0)*0.78);
 water=mix(water,vec3(0.66,0.83,0.90),foam*0.32);
 ALBEDO=water;
 ROUGHNESS=0.20;
 METALLIC=0.08;
 EMISSION=water*night_glow*0.12;
 ALPHA=0.91;
}"""
 var fall_shader:=Shader.new()
 fall_shader.code="""shader_type spatial;
render_mode blend_mix, cull_disabled, shadows_disabled;
void fragment() {
 float streak=sin(UV.x*34.0+TIME*1.1)*0.22+sin(UV.y*54.0-TIME*4.1)*0.21;
 float foam=clamp(0.55+streak,0.2,0.95);
 ALBEDO=mix(vec3(0.14,0.45,0.66),vec3(0.72,0.89,0.97),foam);
 ROUGHNESS=0.16;
 ALPHA=0.74;
}"""
 var fall_mat:=ShaderMaterial.new()
 fall_mat.shader=fall_shader
 var fall_plane:=PlaneMesh.new()
 fall_plane.size=Vector2(1.0,1.0)
 var petal_mesh:=SphereMesh.new()
 petal_mesh.radius=0.33
 petal_mesh.height=0.42
 petal_mesh.radial_segments=8
 petal_mesh.rings=4
 var stem_mesh:=CylinderMesh.new()
 stem_mesh.top_radius=0.045
 stem_mesh.bottom_radius=0.09
 stem_mesh.height=0.95
 stem_mesh.radial_segments=5
 var fish_mesh:=SphereMesh.new()
 fish_mesh.radius=0.22
 fish_mesh.height=0.44
 fish_mesh.radial_segments=7
 fish_mesh.rings=4
 var stalk_mat:=_v361_material(Color("476b52"))
 var fish_mat:=_v361_material(Color("81c4d6"),true)
 for zone in range(V363_RULES.WATER_ZONE_COUNT):
  var water_root:Node3D=v362_water_roots[zone]
  var lake:MeshInstance3D=water_root.get_node_or_null("V362_AnimatedWaterSurface") as MeshInstance3D
  if not is_instance_valid(lake):return
  var previous:ShaderMaterial=lake.material_override as ShaderMaterial
  var original_tint:Color=Color("5295aa")
  if is_instance_valid(previous):original_tint=previous.get_shader_parameter("shallow_color")
  var material:=ShaderMaterial.new()
  material.shader=shader
  material.set_shader_parameter("clear_color",original_tint)
  material.set_shader_parameter("deep_color",original_tint.darkened(0.65))
  material.set_shader_parameter("corrupted",0.0)
  material.set_shader_parameter("night_glow",0.0)
  lake.material_override=material
  v363_water_materials.append(material)
  v363_taint.append(false)
  var midpoint:Vector2=REGIONS[zone]["at"]
  var offset:Vector2=V362_JOBS.initial_water_offset(zone)
  var cx:float=midpoint.x+offset.x
  var cz:float=midpoint.y+offset.y
  # Two gentle cascades at the marsh and quarry; GPU animation only.
  # Neither changes the terrain or creates physics blockers.
  if zone==2 or zone==3:
   var fall:=_v361_primitive(water_root,fall_plane,fall_mat,Vector3(cx,1.18,cz-4.1),Vector3(2.3,1.0,2.2))
   fall.rotation_degrees.x=90.0
   fall.name="V363_MoonlitCascade"
   v363_cascades.append(fall)
  var flora_root:=Node3D.new()
  flora_root.name="V363_ReactiveShoreFlora"
  water_root.add_child(flora_root)
  var normal_mat:=_v361_material(Color("719c83"))
  var bloom_mat:=_v361_material(V361_ART.COLORS[zone],true)
  var taint_mat:=_v361_material(Color("85428d"),true)
  for index in range(V363_RULES.FLORA_PER_ZONE):
   var angle:float=TAU*(float(index)+0.25)/float(V363_RULES.FLORA_PER_ZONE)
   var spot:=Node3D.new()
   spot.name="Moonbloom_%d_%d"%[zone,index]
   spot.position=Vector3(cx+cos(angle)*7.5,0.0,cz+sin(angle)*5.8)
   flora_root.add_child(spot)
   _v361_primitive(spot,stem_mesh,stalk_mat,Vector3(0.0,0.47,0.0))
   var blossom:=_v361_primitive(spot,petal_mesh,bloom_mat,Vector3(0.0,1.02,0.0),Vector3(1.0,0.55,1.0))
   _v361_primitive(spot,petal_mesh,normal_mat,Vector3(-0.26,0.45,0.0),Vector3(0.75,0.28,0.45))
   v363_flora.append({"zone":zone,"node":spot,"flower":blossom,"bright":bloom_mat,"tainted":taint_mat,"origin":spot.position,"phase":angle})
  var fish_root:=Node3D.new()
  fish_root.name="V363_ShoreFish"
  water_root.add_child(fish_root)
  for index in range(V363_RULES.FISH_PER_ZONE):
   var swimmer:=_v361_primitive(fish_root,fish_mesh,fish_mat,Vector3(cx,0.1,cz),Vector3(0.7,0.18,1.15))
   swimmer.name="MoonwaterFish_%d_%d"%[zone,index]
   v363_fish.append({"zone":zone,"node":swimmer,"center":Vector3(cx,0.1,cz),"phase":float(index)*PI,"speed":0.66+float(zone%3)*0.12})

func _v363_refresh_quality()->void:
 if not v363_ready:return
 var details:bool=V361_ART.details_enabled(gfx_settings,graphics_quality)
 for item in v363_flora:
  var node:Node3D=item["node"]
  if is_instance_valid(node):node.visible=details
 for item in v363_fish:
  var node:Node3D=item["node"]
  if is_instance_valid(node):node.visible=details

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 _v363_refresh_quality()

func _process(delta:float)->void:
 super._process(delta)
 if not v363_ready:return
 # 4Hz visual refresh, only player-region decor animates. Shader waves stay on GPU.
 v363_update_elapsed+=minf(delta,0.15)
 if v363_update_elapsed<V363_RULES.UPDATE_SECONDS:return
 var elapsed:float=v363_update_elapsed
 v363_update_elapsed=0.0
 v363_visual_time+=elapsed
 if hero_playing and not death_active and not menu_open:
  v363_ward_seconds=maxf(0.0,v363_ward_seconds-elapsed)
 _v363_tick(elapsed)

func _v363_tick(_elapsed:float)->void:
 if not v363_ready:return
 var night:float=1.0 if _day_phase()=="Night" else 0.24
 var night_changed:bool=not is_equal_approx(night,v363_applied_night_glow)
 if night_changed:v363_applied_night_glow=night
 for zone in range(V363_RULES.WATER_ZONE_COUNT):
  var tainted:bool=V363_RULES.is_tainted(zone,rifts,v341_active_site,v341_sites)
  if tainted!=v363_taint[zone]:
   v363_taint[zone]=tainted
   v363_water_materials[zone].set_shader_parameter("corrupted",1.0 if tainted else 0.0)
  if night_changed:v363_water_materials[zone].set_shader_parameter("night_glow",night)
 if not hero_playing or not is_instance_valid(player):return
 if not V361_ART.details_enabled(gfx_settings,graphics_quality):return
 var player_at:Vector3=player.global_position
 for item in v363_flora:
  var zone:int=int(item["zone"])
  if zone!=region_index:continue
  var stalk:Node3D=item["node"]
  var flower:MeshInstance3D=item["flower"]
  if not is_instance_valid(stalk) or not is_instance_valid(flower):continue
  var dist_sq:float=player_at.distance_squared_to(stalk.global_position)
  var response:float=V363_RULES.flora_response(dist_sq,v363_taint[zone])
  stalk.rotation_degrees.z=7.0*response*sin(v363_visual_time*2.0+float(item["phase"]))
  var flower_material:Material=item["tainted"] if v363_taint[zone] else item["bright"]
  if flower.material_override!=flower_material:flower.material_override=flower_material
  stalk.scale=Vector3.ONE*(1.0+response*0.10)
  v363_flora_updates+=1
 for item in v363_fish:
  if int(item["zone"])!=region_index:continue
  var fish:MeshInstance3D=item["node"]
  if not is_instance_valid(fish):continue
  var center:Vector3=item["center"]
  var dist_sq:float=player_at.distance_squared_to(center)
  var radius:float=V363_RULES.fish_escape_radius(dist_sq)
  var angle:float=v363_visual_time*float(item["speed"])+float(item["phase"])
  fish.position=center+Vector3(cos(angle)*radius,0.0,sin(angle)*radius*0.66)
  fish.rotation.y=angle+PI*0.5
  v363_fish_updates+=1

func _apply_incoming_damage(raw:float)->void:
 super._apply_incoming_damage(V363_RULES.ward_damage(raw,v363_ward_seconds,_depth_near_corruption()>=0))

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="World":
  menu_body.text+="\n\n[b]MOONFALL'S LIVING WORLD[/b]\nWater develops violet corruption near unsealed rifts and active lunar invasions. Seal a rift to restore its region's shoreline. Fish retreat and moonblooms respond when approached. Water, wildlife and flora are decorations, not new obstacles or survival mechanics."
  menu_body.text+="\nMoonbloom ward: "+("ACTIVE (corruption damage -20%)" if v363_ward_seconds>0.0 else "Inactive")
 if which=="Professions":
  menu_body.text+="\n\n[b]LIVING WORLD RECIPES[/b]\nGather from your existing sites, then cook, brew or forge useful rewards."
  for index in range(V363_RULES.RECIPES.size()):
   var recipe:Dictionary=V363_RULES.RECIPES[index]
   var issue:String=V363_RULES.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
   menu_body.text+="\n"+str(recipe["name"])+" • "+str(recipe["cost"])+(" [READY]" if issue=="" else " ["+issue+"]")
   _atlas_action("CRAFT: "+str(recipe["name"]),"v363:craft:"+str(index))
  _atlas_action("EAT TRAIL RATIONS (+35% HP)","v363:use:food")
  _atlas_action("DRINK WARD TONIC (-20% CORRUPTION DAMAGE)","v363:use:ward")

func _atlas_execute(action:String)->void:
 if action.begins_with("v363:craft:"):
  _v363_craft(int(action.trim_prefix("v363:craft:")))
  _menu_tab("Professions")
  return
 if action.begins_with("v363:use:"):
  _v363_consume(action.trim_prefix("v363:use:"))
  _menu_tab("Professions")
  return
 super._atlas_execute(action)

func _v363_craft(index:int)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Craft safely while on foot.",1.8)
  return
 if index<0 or index>=V363_RULES.RECIPES.size():return
 var recipe:Dictionary=V363_RULES.RECIPES[index]
 var issue:String=V363_RULES.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
 if issue!="":
  _show_message(issue,1.8)
  return
 V363_RULES.spend(recipe,materials)
 if int(recipe["power"])>0:
  gear_items.append({"name":recipe["result"],"rarity":"Crafted","power":recipe["power"]})
 else:
  _gain_material(str(recipe["result"]),1)
 var job:String=str(recipe["job"])
 water_professions[job]=mini(300,int(water_professions.get(job,1))+1)
 _show_message("Crafted "+str(recipe["name"])+" • "+job+" "+str(water_professions[job]),2.0)
 _save_game()

func _v363_consume(kind:String)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Use field supplies when not fighting.",1.8)
  return
 var item:String="Witchwood Trail Rations" if kind=="food" else ("Moonbloom Ward Tonic" if kind=="ward" else "")
 if item=="":return
 if kind=="food" and hp>=max_hp:
  _show_message("Health is already full.",1.6)
  return
 if not INVENTORY_RULES.consume_one(materials,item):
  _show_message("No "+item+" in materials.",1.8)
  return
 if kind=="food":hp=minf(max_hp,hp+max_hp*0.35)
 else:v363_ward_seconds=V363_RULES.MAX_WARD_SECONDS
 _show_message("Used "+item+".",1.8)
 _save_game()

func _start_game(selected:String)->void:
 super._start_game(selected)
 v363_ward_seconds=0.0

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 # The ward is a temporary consumable effect, not a permanent buff.
 v363_ward_seconds=0.0

func _death_return_to_sanctuary()->void:
 super._death_return_to_sanctuary()
 v363_ward_seconds=0.0

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["living_world"]="v36.3 living shorelines and rift restoration"
 report["living_flora"]=v363_flora.size()
 report["water_cascades"]=v363_cascades.size()
 report["decorative_fish"]=v363_fish.size()
 report["floral_visual_ticks"]=v363_flora_updates
 report["fish_visual_ticks"]=v363_fish_updates
 report["ward_active"]=v363_ward_seconds>0.0
 return report
