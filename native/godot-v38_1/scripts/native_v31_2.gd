extends "res://scripts/native_v31_1.gd"
## v31.2: shared-resource foliage, pooled combat highlights, environmental lighting,
## camera distance LOD. Character art, saves, physics and navigation are unchanged.
const V312_FOLIAGE_PER_REGION := 96
const V312_HIT_POOL := 8
var v312_ready:bool=false
var v312_foliage:Array[MultiMeshInstance3D]=[]
var v312_hits:Array[MeshInstance3D]=[]
var v312_hit_time:Array[float]=[]
var v312_hit_cursor:int=0
var v312_hit_mesh:SphereMesh
var v312_hit_materials:Array[StandardMaterial3D]=[]
var v312_fx_clock:float=0.0
var v312_light_clock:float=0.0
var v312_lod_limit:float=45.0
var v312_visual_stats:Dictionary={}

func _release_label()->String:return "v31.2"

func _ready()->void:
 super._ready()
 _v312_build_foliage()
 _v312_build_hit_pool()
 v312_ready=true
 _v312_update_quality()
 _v312_update_lighting()

func _v312_build_foliage()->void:
 # One opaque MultiMesh per biome, not a scene node per grass blade.
 # Deterministic positions avoid save-game and gameplay RNG changes.
 var tuft:=PrismMesh.new()
 tuft.size=Vector3(0.17,0.38,0.09)
 for zone in range(REGIONS.size()):
  var material:=StandardMaterial3D.new()
  var color:Color=[Color("66865e"),Color("68926d"),Color("9aa0a2"),Color("756c83"),Color("998a63"),Color("85657d")][zone%6]
  material.albedo_color=color
  material.roughness=0.96
  material.cull_mode=BaseMaterial3D.CULL_DISABLED
  material.vertex_color_use_as_albedo=true
  var mesh_batch:=MultiMesh.new()
  mesh_batch.transform_format=MultiMesh.TRANSFORM_3D
  mesh_batch.use_colors=true
  mesh_batch.mesh=tuft
  mesh_batch.instance_count=V312_FOLIAGE_PER_REGION
  var random:=RandomNumberGenerator.new()
  random.seed=2026312+zone*4001
  for i in range(V312_FOLIAGE_PER_REGION):
   var xpos:float=random.randf_range(-23.0,23.0)
   var zpos:float=random.randf_range(-22.0,22.0)
   # Roads and the main approach lane stay clear. The blades have no collisions.
   if absf(zpos)<4.5:zpos=4.5*signf(zpos) if zpos!=0.0 else 4.5
   var basis:=Basis(Vector3.UP,random.randf_range(-PI,PI))
   basis=basis.scaled(Vector3(random.randf_range(0.7,1.4),random.randf_range(0.6,1.55),1.0))
   mesh_batch.set_instance_transform(i,Transform3D(basis,Vector3(xpos,0.24,zpos)))
   mesh_batch.set_instance_color(i,Color(0.86+random.randf_range(0,0.14),0.92+random.randf_range(0,0.08),0.80+random.randf_range(0,0.16)))
  var scenery:=MultiMeshInstance3D.new()
  scenery.name="V312_BatchedFoliage_"+str(zone)
  scenery.multimesh=mesh_batch
  scenery.material_override=material
  var center:Vector2=REGIONS[zone]["at"]
  scenery.position=Vector3(center.x,0.0,center.y)
  scenery.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  scenery.visibility_range_end=45.0
  world.add_child(scenery)
  v312_foliage.append(scenery)

func _v312_build_hit_pool()->void:
 v312_hit_mesh=SphereMesh.new()
 v312_hit_mesh.radius=0.24
 v312_hit_mesh.height=0.32
 for tint in [Color("ffa665"),Color("f15e6d")]:
  var material:=StandardMaterial3D.new()
  material.albedo_color=tint
  material.emission_enabled=true
  material.emission=tint
  material.emission_energy_multiplier=1.45
  material.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
  material.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
  material.no_depth_test=false
  v312_hit_materials.append(material)
 for i in range(V312_HIT_POOL):
  var orb:=MeshInstance3D.new()
  orb.name="V312_RecycledImpact_"+str(i)
  orb.mesh=v312_hit_mesh
  orb.material_override=v312_hit_materials[0]
  orb.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  orb.visible=false
  world.add_child(orb)
  v312_hits.append(orb)
  v312_hit_time.append(0.0)

func _v312_impact(at:Vector3,player_damage:bool=false)->void:
 if not v312_ready or str(gfx_settings.get("combat_effects","Medium"))=="Off":return
 if not is_instance_valid(player) or at.distance_squared_to(player.global_position)>900.0:return
 var n:int=v312_hit_cursor%V312_HIT_POOL
 v312_hit_cursor+=1
 var orb:MeshInstance3D=v312_hits[n]
 orb.global_position=at+Vector3(0,1.1,0)
 orb.material_override=v312_hit_materials[1 if player_damage else 0]
 orb.scale=Vector3.ONE
 orb.visible=true
 v312_hit_time[n]=0.27 if str(gfx_settings.get("combat_effects","Medium"))=="High" else 0.16

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var old:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if float(target.get("hp",0.0))<old:
  var body:Node3D=target.get("body")
  if is_instance_valid(body):_v312_impact(body.global_position)

func _apply_incoming_damage(raw:float)->void:
 var old:float=hp
 super._apply_incoming_damage(raw)
 if hp<old and is_instance_valid(player):_v312_impact(player.global_position,true)

func _v312_update_lighting()->void:
 if not v312_ready or not gfx_ready:return
 var moon:=get_node_or_null("TwinMoonlight") as DirectionalLight3D
 if not is_instance_valid(moon) or living_environment==null:return
 # Parent lighting owns time/weather; base refresh prevents accumulating multipliers.
 var choice:String=str(gfx_settings.get("cinematic_lighting","Natural"))
 if choice=="Classic":return
 var phase:String=_day_phase()
 var night:bool=phase.to_lower().contains("night") or phase.to_lower().contains("dusk")
 moon.light_color=moon.light_color.lerp(Color("aab9ea") if night else Color("ffdda8"),0.14 if choice=="Natural" else 0.32)
 moon.light_energy*=0.97 if choice=="Natural" else 0.91
 var fog_tint:Color=Color("7181a0") if night else Color("a4a4a3")
 living_environment.fog_light_color=living_environment.fog_light_color.lerp(fog_tint,0.09 if choice=="Natural" else 0.21)

func _v312_update_quality()->void:
 if not v312_ready or not gfx_ready:return
 var detail:int=int(gfx_settings.get("environment_detail","7"))
 var foliage:String=str(gfx_settings.get("foliage_quality","High"))
 var enabled:bool=detail>2 and foliage!="Off" and str(gfx_settings.get("ground_clutter","Medium"))!="Off"
 var range_factor:float={"Near":0.6,"Balanced":1.0,"Far":1.45}.get(str(gfx_settings.get("scenery_lod","Balanced")),1.0)
 v312_lod_limit=(20.0+float(detail)*3.5)*range_factor
 for item in v312_foliage:
  if not is_instance_valid(item):continue
  item.visible=enabled
  item.visibility_range_end=v312_lod_limit
  item.multimesh.visible_instance_count=0 if not enabled else 24 if foliage=="Low" else 56 if foliage=="Medium" else V312_FOLIAGE_PER_REGION
 # Existing static dioramas retain the inherited proximity check; geometry
 # visibility ranges provide an additional GPU-side cull when far away.
 for diorama in v24_diorama_cache:
  if not is_instance_valid(diorama):continue
  for child in diorama.get_children():
   if child is GeometryInstance3D:
    child.visibility_range_end=v312_lod_limit
 v312_visual_stats={"foliage_batches":v312_foliage.size(),"max_instances":v312_foliage.size()*V312_FOLIAGE_PER_REGION,"impact_pool":v312_hits.size(),"cull_distance":v312_lod_limit}

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 if v312_ready:
  _v312_update_quality()

func _refresh_world_lighting()->void:
 super._refresh_world_lighting()
 if v312_ready:_v312_update_lighting()

func _process(delta:float)->void:
 super._process(delta)
 if not v312_ready:return
 v312_fx_clock+=delta
 if v312_fx_clock>=0.04:
  var elapsed:float=v312_fx_clock
  v312_fx_clock=0.0
  for i in range(V312_HIT_POOL):
   if v312_hit_time[i]<=0.0:continue
   v312_hit_time[i]=maxf(0.0,v312_hit_time[i]-elapsed)
   var orb:MeshInstance3D=v312_hits[i]
   orb.visible=v312_hit_time[i]>0.0
   if orb.visible:orb.scale+=Vector3.ONE*elapsed*2.6
 # Lighting and culling are not recomputed every frame.
 v312_light_clock+=delta
 if v312_light_clock>=2.0:
  v312_light_clock=0.0
  _refresh_world_lighting()
