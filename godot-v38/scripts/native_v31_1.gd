extends "res://scripts/native_v31_0.gd"
## v31.1: physically based region surfaces and more natural atmospheric rendering.
## No changes to existing character art, collision, gameplay, or save schema.
const REAL_SURFACES:=7
var realism_materials:Array[StandardMaterial3D]=[]
var realism_flat:Array[Material]=[]
var realism_meshes:Array[MeshInstance3D]=[]
var realism_surface_ids:Array[int]=[]
var realism_ready:bool=false

func _release_label()->String:return "v31.1"

func _ready()->void:
 super._ready()
 _realism_collect_surfaces()
 realism_ready=true
 _gfx_apply(false)

func _realism_collect_surfaces()->void:
 for i in range(REAL_SURFACES):
  var material:=StandardMaterial3D.new()
  material.albedo_texture=load("res://assets/v31_realism/surface_%d.png" % i)
  material.normal_texture=load("res://assets/v31_realism/normal_%d.png" % i)
  material.normal_enabled=true
  material.normal_scale=0.65 if i in [0,1,2,6] else 0.85
  material.roughness=0.88 if i in [0,1,3,4,6] else 0.76
  material.metallic=0.0
  material.uv1_triplanar=true
  material.uv1_world_triplanar=true
  material.uv1_scale=Vector3(0.55,0.55,0.55)
  material.texture_filter=BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
  realism_materials.append(material)
 for zone in range(6):
  var ground:Node3D=region_nodes[zone].get_node_or_null("RegionGround")
  if ground==null:continue
  for child in ground.get_children():
   if child is MeshInstance3D:
    realism_meshes.append(child)
    realism_flat.append(child.material_override)
    realism_surface_ids.append(zone)
 # Roads are neutral stone and share ONE PBR material for efficient batching.
 for child in world.get_children():
  # Godot auto-renames repeated road nodes; identify their known box dimensions
  # instead of relying on a duplicated name.
  if not child is Node3D or absf(child.position.y-0.015)>0.001:continue
  for surface in child.get_children():
   if surface is MeshInstance3D and surface.mesh is BoxMesh and (
    is_equal_approx((surface.mesh as BoxMesh).size.x,3.8) or
    is_equal_approx((surface.mesh as BoxMesh).size.x,2.8)):
    realism_meshes.append(surface)
    realism_flat.append(surface.material_override)
    realism_surface_ids.append(6)

func _realism_set_materials()->void:
 if not realism_ready:return
 var quality:String=str(gfx_settings.get("terrain_surface","PBR Detail"))
 var filter:String=str(gfx_settings.get("texture_filter","Trilinear"))
 var filter_mode:BaseMaterial3D.TextureFilter=BaseMaterial3D.TEXTURE_FILTER_NEAREST if filter=="Nearest" else BaseMaterial3D.TEXTURE_FILTER_LINEAR if filter=="Linear" else BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS if filter=="Trilinear" else BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
 for surface in realism_materials:
  surface.normal_enabled=quality=="PBR Detail"
  surface.texture_filter=filter_mode
 for j in range(realism_meshes.size()):
  var mesh:=realism_meshes[j]
  if is_instance_valid(mesh):mesh.material_override=realism_flat[j] if quality=="Off" else realism_materials[realism_surface_ids[j]]
 _realism_weather_materials()

func _realism_weather_materials()->void:
 if not realism_ready:return
 var wet:bool=str(gfx_settings.get("wet_surfaces","On"))=="On" and weather_name in ["Rain","Mist"]
 for i in range(realism_materials.size()):
  var mat:=realism_materials[i]
  mat.roughness=(0.51 if i==2 else 0.62) if wet else (0.76 if i in [2,5] else 0.88)
  mat.albedo_color=Color(0.80,0.83,0.87) if wet else Color.WHITE

func _realism_atmosphere()->void:
 if not gfx_ready or living_environment==null:return
 var style:String=str(gfx_settings.get("realism_grade","Natural"))
 living_environment.tonemap_mode=Environment.TONE_MAPPER_FILMIC if style=="Classic" else Environment.TONE_MAPPER_ACES if style=="Natural" else Environment.TONE_MAPPER_AGX
 living_environment.tonemap_white=6.0
 living_environment.tonemap_exposure=1.0 if style=="Classic" else 0.93 if style=="Natural" else 0.86
 living_environment.adjustment_saturation=1.0 if style=="Classic" else 0.94 if style=="Natural" else 0.90
 var light:=get_node_or_null("TwinMoonlight") as DirectionalLight3D
 if is_instance_valid(light):
  # Wider source light softens shadows without disabling combat readability.
  light.light_angular_distance=0.0 if str(gfx_settings.get("soft_shadows","On"))=="Off" else 0.7

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 if not gfx_ready:return
 _realism_set_materials()
 _realism_atmosphere()

func _refresh_world_lighting()->void:
 super._refresh_world_lighting()
 if realism_ready:_realism_weather_materials()
