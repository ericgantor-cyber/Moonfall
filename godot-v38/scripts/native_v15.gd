extends "res://scripts/native_v14.gd"
## v15 Living Art: deterministic generated coat patterns, creature gait/flap/slither,
## guardian coronas, regional wisps and distance-bounded cosmetic updates.
## Visuals do not alter collisions, hero art, gameplay RNG or save schema.

const ANIMATION_DISTANCE_SQUARED := 36.0 * 36.0
const MOTION_TICK_SECONDS := 0.06
const PELT_RESOLUTION := 64
var pelt_textures: Dictionary = {}
var motion_tracks: Array[Dictionary] = []
var guardian_coronas: Dictionary = {}
var regional_wisps: Array[Dictionary] = []
var motion_accumulator := 0.0
var animated_this_tick := 0
var far_animation_skips := 0
var animation_ticks := 0
var phase_bursts := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall Native v15: Living Art animations and patterned wildlife.",4.5)

func _pattern_image(kind: String, base_color: Color) -> ImageTexture:
 var img := Image.create(PELT_RESOLUTION, PELT_RESOLUTION, false, Image.FORMAT_RGBA8)
 var seed := float(kind.length())
 for y in range(PELT_RESOLUTION):
  for x in range(PELT_RESOLUTION):
   var fx := float(x)
   var fy := float(y)
   var grain := sin(fx * 15.723 + fy * 38.416 + seed * 52.12) * 43758.5453
   grain -= floorf(grain)
   var shade := 0.05 * (grain - 0.5)
   match kind:
    "wolf":shade += 0.19 * sin(fx * 0.20 + fy * 0.27)
    "fox":shade += 0.19 * sin(fy * 0.15 + fx * 0.07)
    "boar":shade += 0.14 * sin(fx * 0.43 + fy * 0.11)
    "panther":shade += 0.12 * cos(fx * 0.33) * sin(fy * 0.39)
    "serpent":shade += 0.20 * sin(fy * 0.51 + sin(fx * 0.34))
    "owl":shade += 0.17 * cos(fy * 0.42 + absf(sin(fx * 0.31)) * 2.8)
    "stag","elk":shade += 0.13 * cos(fx * 0.27 + sin(fy * 0.33) * 1.8)
   var color := base_color.lightened(maxf(shade, 0.0)) if shade >= 0.0 else base_color.darkened(-shade)
   img.set_pixel(x,y,color)
 return ImageTexture.create_from_image(img)

func _coat_material(kind: String, variant: String, base_color: Color) -> StandardMaterial3D:
 var key := kind + ":" + variant
 if not pelt_textures.has(key):
  pelt_textures[key] = _pattern_image(kind,base_color)
 var material := StandardMaterial3D.new()
 material.albedo_texture = pelt_textures[key]
 material.roughness = 0.87
 return material

func _build_region(zone: int) -> void:
 super._build_region(zone)
 # Five shared-mesh ambient wisps per region. They are hidden in Smooth mode
 # along with the landmarks, and update only near the player.
 var parent:Node3D = decorative_groups[-1]
 var accent := Color(str(REGIONS[zone]["light"]))
 var center:Vector2 = REGIONS[zone]["at"]
 for i in range(5):
  var node := Node3D.new()
  node.name = "V15_RegionalWisp"
  parent.add_child(node)
  var angle := TAU * float(i) / 5.0
  var anchor := Vector3(center.x-15.0+cos(angle)*5.8,1.4+float(i%3)*0.3,center.y+12.0+sin(angle)*5.8)
  node.position = anchor
  _form(node,"sphere",Vector3.ZERO,Vector3(0.21,0.21,0.21),accent,true)
  regional_wisps.append({"node":node,"at":anchor,"phase":angle,"zone":zone})

func _spawn_beast(at: Vector3, kind: String, zone: int, variant: String="normal", boss: bool=false) -> void:
 super._spawn_beast(at,kind,zone,variant,boss)
 var rig:Node3D = visual_rigs[-1]
 var beast:Dictionary = beasts[-1]
 var parts:Array[Node] = rig.get_children()
 var leg_parts:Array[Dictionary] = []
 var wing_parts:Array[MeshInstance3D] = []
 var serpent_parts:Array[MeshInstance3D] = []
 var primary:Array[MeshInstance3D] = []
 for part in parts:
  if not part is MeshInstance3D:continue
  var mesh:MeshInstance3D=part
  if mesh.mesh is CylinderMesh and absf(mesh.position.y+0.42)<0.03 and absf(mesh.scale.y-0.72)<0.04:
   leg_parts.append({"mesh":mesh,"start":mesh.position})
  if kind=="owl" and mesh.mesh is SphereMesh and absf(absf(mesh.position.z)-0.46)<0.03 and absf(mesh.position.y+0.05)<0.03:
   wing_parts.append(mesh)
  if kind=="serpent" and mesh.mesh is SphereMesh and serpent_parts.size()<5:
   serpent_parts.append(mesh)
  if mesh.mesh is SphereMesh and primary.size()<(6 if kind=="serpent" else 2 if kind=="owl" else 1):
   primary.append(mesh)
 var sample:StandardMaterial3D=primary[0].material_override if not primary.is_empty() else null
 if sample!=null:
  var painted:=_coat_material(kind,variant,sample.albedo_color)
  for part in primary:part.material_override=painted
 # Existing feet remain visible: they move with their nearby leg cylinders.
 var feet:Array[MeshInstance3D] = []
 if kind not in ["owl","serpent"]:
  for part in parts:
   if part is MeshInstance3D and part.mesh is SphereMesh and absf(part.position.y+0.75)<0.03 and absf(part.scale.y-0.17)<0.04:
    feet.append(part)
 var corona:Node3D
 if boss:
  corona=Node3D.new()
  corona.name="V15_GuardianCorona"
  rig.add_child(corona)
  corona.position.y=1.3
  _form(corona,"ring",Vector3(0,0.0,0),Vector3(1.2,0.07,1.2),Color("f1a0ff"),true,Vector3(20,0,0))
  _form(corona,"ring",Vector3(0,0.35,0),Vector3(0.95,0.06,0.95),Color("ff775f"),true,Vector3(-17,0,0))
  corona.visible=false
  guardian_coronas[int(beast["boss_id"])] = corona
 motion_tracks.append({"rig":rig,"kind":kind,"zone":zone,"legs":leg_parts,"feet":feet,"wings":wing_parts,"segments":serpent_parts,"phase":float(motion_tracks.size())*0.73,"base_scale":rig.scale,"corona":corona})

func _refresh_boss_phase(beast: Dictionary) -> void:
 var before := int(beast.get("boss_phase",1))
 super._refresh_boss_phase(beast)
 var after := int(beast.get("boss_phase",1))
 if after <= before:return
 var id := int(beast.get("boss_id",-1))
 if guardian_coronas.has(id):
  var corona:Node3D=guardian_coronas[id]
  if is_instance_valid(corona):corona.visible=true
 var body:CharacterBody3D=beast.get("body")
 if is_instance_valid(body) and is_instance_valid(player) and body.global_position.distance_squared_to(player.global_position)<2500.0:
  _impact_ring(body.global_position + Vector3(0,0.1,0),Color("ff6a96") if after==3 else Color("ffc476"))
  phase_bursts += 1

func _process(delta:float) -> void:
 super._process(delta)
 motion_accumulator += minf(maxf(delta,0.0),0.2)
 if motion_accumulator < MOTION_TICK_SECONDS:return
 motion_accumulator=fmod(motion_accumulator,MOTION_TICK_SECONDS)
 animation_ticks += 1
 animated_this_tick=0
 if not is_instance_valid(player):return
 var p:Vector3=player.global_position
 for i in range(motion_tracks.size()):
  var track:Dictionary=motion_tracks[i]
  var creature:Dictionary=beasts[i]
  if not bool(creature.get("alive",false)) or int(track["zone"])!=region_index:continue
  var body:CharacterBody3D=creature["body"]
  if not is_instance_valid(body) or not body.visible:continue
  if body.global_position.distance_squared_to(p)>ANIMATION_DISTANCE_SQUARED:
   far_animation_skips += 1
   continue
  var rig:Node3D=track["rig"]
  var kind:String=track["kind"]
  var phase:float=map_time*10.0+float(track["phase"])
  var moving:float=clampf(body.velocity.length()/3.5,0.0,1.0)
  var staggered:bool=map_time<float(creature.get("stagger_until",0.0))
  if staggered:moving=0.0
  var strength:float=0.22*moving
  rig.rotation.z=lerpf(rig.rotation.z,(0.12 if staggered else sin(phase*0.7)*0.035*moving),0.35)
  for leg_index in range(track["legs"].size()):
   var limb:Dictionary=track["legs"][leg_index]
   var mesh:MeshInstance3D=limb["mesh"]
   var gait:float=sin(phase+(PI if leg_index%3==0 else 0.0))
   mesh.rotation.z=lerpf(mesh.rotation.z,gait*strength,0.4)
  for foot_index in range(track["feet"].size()):
   var foot:MeshInstance3D=track["feet"][foot_index]
   var lift:float=maxf(0.0,sin(phase+(PI if foot_index%3==0 else 0.0)))
   foot.position.y=lerpf(foot.position.y,-0.75+lift*0.15*moving,0.45)
  for wing_index in range(track["wings"].size()):
   var wing:MeshInstance3D=track["wings"][wing_index]
   wing.rotation.x=sin(phase*1.25)*0.37*(1.0 if moving>0.15 else 0.46)*(1.0 if wing_index%2==0 else -1.0)
  for segment_index in range(track["segments"].size()):
   var section:MeshInstance3D=track["segments"][segment_index]
   section.position.z=sin(phase*0.70+float(segment_index)*0.8)*0.11*(0.3+moving)
  var crown:Node3D=track["corona"]
  if is_instance_valid(crown):
   crown.visible=int(creature.get("boss_phase",1))>=2
   if crown.visible:
    crown.rotation.y=fposmod(crown.rotation.y+0.095,TAU)
    var pulse:float=1.0+0.12*sin(map_time*3.1+float(i))
    crown.scale=Vector3.ONE*pulse
  animated_this_tick += 1
 if graphics_quality!="Smooth":
  for mote in regional_wisps:
   if int(mote["zone"])!=region_index:continue
   var node:Node3D=mote["node"]
   if not is_instance_valid(node) or not node.visible:continue
   var base:Vector3=mote["at"]
   if p.distance_squared_to(base)>ANIMATION_DISTANCE_SQUARED:continue
   var angle:float=map_time*1.25+float(mote["phase"])
   node.position=base+Vector3(cos(angle)*0.35,sin(angle*0.73)*0.24,sin(angle)*0.35)
