extends Node3D
## Reusable visual response to successful hero action animations in Witchwood.
## Zero new projectiles, hitboxes, gameplay rewards, save fields or light sources.
const FERN_RESPONSE:=preload("res://shaders/witchwood_fern_response_v388.gdshader")
const MAX_RIPPLES:int=2
const RIPPLE_LIFETIME:float=0.42
var fern_material:ShaderMaterial
var rings:Array[MeshInstance3D]=[]
var ages:Array[float]=[]
var colors:Array[Color]=[]
var cursor:int=0
var active_quality:bool=true
var impact_power:float=0.0
var triggers:int=0

func setup(fern_batch:MultiMeshInstance3D)->void:
 if is_instance_valid(fern_batch):
  fern_material=ShaderMaterial.new()
  fern_material.shader=FERN_RESPONSE
  fern_batch.material_override=fern_material
 var mesh:=TorusMesh.new()
 mesh.inner_radius=0.48
 mesh.outer_radius=0.56
 mesh.rings=8
 mesh.ring_segments=32
 for i in range(MAX_RIPPLES):
  var ring:=MeshInstance3D.new()
  ring.name="CombatGroundRipple_"+str(i)
  ring.mesh=mesh
  ring.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  var mat:=StandardMaterial3D.new()
  mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
  mat.cull_mode=BaseMaterial3D.CULL_DISABLED
  mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
  mat.albedo_color=Color(0.55,0.78,0.84,0.0)
  ring.material_override=mat
  ring.visible=false
  add_child(ring)
  rings.append(ring)
  ages.append(RIPPLE_LIFETIME)
  colors.append(Color("88bdc7"))
 set_process(false)

func configure(settings:Dictionary)->void:
 var detail:int=clampi(int(settings.get("environment_detail","7")),1,10)
 active_quality=detail>=5 and str(settings.get("ground_clutter","High"))!="Off"
 if not active_quality:
  impact_power=0.0
  if is_instance_valid(fern_material):fern_material.set_shader_parameter("impact_strength",0.0)
  for i in range(rings.size()):
   rings[i].visible=false
   ages[i]=RIPPLE_LIFETIME
  set_process(false)

func trigger(action:String,world_position:Vector3)->void:
 if not active_quality or rings.is_empty():return
 var tint:Color=Color("8bbdc7")
 match action:
  "attack":tint=Color("e1a36b")
  "cast":tint=Color("8a9fe5")
  "dash":tint=Color("a6c9c0")
  "hurt":tint=Color("c783a3")
 var index:int=cursor%MAX_RIPPLES
 cursor+=1
 var ring:MeshInstance3D=rings[index]
 ring.global_position=Vector3(world_position.x,0.105,world_position.z)
 ring.scale=Vector3.ONE*0.60
 ring.visible=true
 ages[index]=0.0
 colors[index]=tint
 impact_power=0.75 if action=="attack" or action=="cast" else 0.55
 if is_instance_valid(fern_material):
  fern_material.set_shader_parameter("impact_center",world_position)
  fern_material.set_shader_parameter("impact_strength",impact_power)
 triggers+=1
 set_process(true)

func _process(delta:float)->void:
 var dt:float=clampf(delta,0.0,0.1)
 var any_live:bool=false
 for i in range(rings.size()):
  if ages[i]>=RIPPLE_LIFETIME:continue
  ages[i]=minf(RIPPLE_LIFETIME,ages[i]+dt)
  var fraction:float=ages[i]/RIPPLE_LIFETIME
  var ring:MeshInstance3D=rings[i]
  ring.scale=Vector3.ONE*(0.60+1.55*fraction)
  var tint:Color=colors[i]
  var mat:StandardMaterial3D=ring.material_override as StandardMaterial3D
  mat.albedo_color=Color(tint.r,tint.g,tint.b,0.24*(1.0-fraction))
  ring.visible=fraction<1.0
  any_live=any_live or ring.visible
 impact_power=maxf(0.0,impact_power-dt*2.3)
 if is_instance_valid(fern_material):fern_material.set_shader_parameter("impact_strength",impact_power)
 if not any_live and impact_power<=0.001:set_process(false)

func active_ripple_count()->int:
 var result:int=0
 for ring in rings:
  if ring.visible:result+=1
 return result
