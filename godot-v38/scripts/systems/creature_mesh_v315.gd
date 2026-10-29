extends RefCounted
## Moonfall v31.5: reusable organic ArrayMesh profiles and PBR materials.
## These are actual Godot 3D surface meshes, not billboard portraits.
## No physics shapes, gameplay identifiers, or save data are created here.

const PROFILES:Dictionary={
 "wolf":Vector3(0.84,0.40,0.35),"stag":Vector3(0.92,0.43,0.35),
 "boar":Vector3(0.82,0.52,0.46),"fox":Vector3(0.73,0.32,0.29),
 "serpent":Vector3(1.06,0.30,0.30),"panther":Vector3(0.90,0.37,0.33),
 "elk":Vector3(1.06,0.55,0.43),"owl":Vector3(0.47,0.51,0.44)}
const COATS:Dictionary={
 "wolf":Color("60616a"),"stag":Color("8c6649"),"boar":Color("65473c"),
 "fox":Color("bd613e"),"serpent":Color("39786a"),"panther":Color("34394b"),
 "elk":Color("755942"),"owl":Color("8c8792")}
static var cached_meshes:Dictionary={}
static var cached_materials:Dictionary={}

static func sculpt_mesh(species:String,part:String)->ArrayMesh:
 var key:String=species+":"+part
 if cached_meshes.has(key):return cached_meshes[key]
 var dims:Vector3=PROFILES.get(species,PROFILES["wolf"])
 var half_len:float=dims.x if part=="body" else dims.x*0.46
 var radius_y:float=dims.y if part=="body" else dims.y*0.66
 var radius_z:float=dims.z if part=="body" else dims.z*0.68
 if part=="tail":
  half_len=dims.x*0.67
  radius_y=dims.y*0.29
  radius_z=dims.z*0.32
 var sections:int=16
 var facets:int=16
 var surface:=SurfaceTool.new()
 surface.begin(Mesh.PRIMITIVE_TRIANGLES)
 # Indexed quad strips share vertices, generating smooth (not faceted) PBR normals.
 for section in range(sections+1):
  for face in range(facets+1):
   var along:float=float(section)/float(sections)
   var theta:float=TAU*float(face)/float(facets)
   var rounded:float=sqrt(maxf(0.0005,1.0-pow(2.0*along-1.0,2)))
   var shape:float=1.0+0.035*sin(theta*7.0+along*17.0+float(species.length()))
   var x:float=(along*2.0-1.0)*half_len
   var y:float=sin(theta)*radius_y*rounded*shape
   var z:float=cos(theta)*radius_z*rounded*shape
   if part=="tail":y+=0.15*sin(along*PI)
   elif part=="body":y+=0.06*sin(along*PI)
   var shade:float=0.85+0.14*sin(along*15.0+theta*4.0)
   surface.set_uv(Vector2(along,theta/TAU))
   surface.set_color(Color(shade,shade,shade))
   surface.add_vertex(Vector3(x,y,z))
 for section in range(sections):
  for face in range(facets):
   var a:int=section*(facets+1)+face
   var b:int=a+1
   var c:int=a+facets+1
   var d:int=c+1
   # Outside-facing triangles: normal roughly +Z at theta zero.
   for index in [a,d,b,a,c,d]:surface.add_index(index)
 surface.generate_normals()
 var result:ArrayMesh=surface.commit()
 cached_meshes[key]=result
 return result

static func coat(species:String,accent:bool=false)->StandardMaterial3D:
 var key:String=species+(":accent" if accent else ":skin")
 if cached_materials.has(key):return cached_materials[key]
 var mat:=StandardMaterial3D.new()
 var base:Color=COATS.get(species,COATS["wolf"])
 mat.albedo_color=base.lightened(0.18) if accent else base
 mat.vertex_color_use_as_albedo=true
 mat.roughness=0.78 if species in ["wolf","fox","owl","elk","stag"] else 0.52
 mat.metallic=0.05 if species=="serpent" else 0.0
 mat.cull_mode=BaseMaterial3D.CULL_DISABLED
 cached_materials[key]=mat
 return mat

static func part(parent:Node3D,species:String,section:String,position:Vector3,accent:bool=false)->MeshInstance3D:
 var item:=MeshInstance3D.new()
 item.name="V315_"+section.capitalize()
 item.mesh=sculpt_mesh(species,section)
 item.material_override=coat(species,accent)
 item.position=position
 parent.add_child(item)
 return item

static func create(rig:Node3D,species:String,boss:bool=false)->Dictionary:
 var root:=Node3D.new()
 root.name="V315_OrganicCreatureModel"
 rig.add_child(root)
 var body:=part(root,species,"body",Vector3(0,0.23,0))
 var head_pivot:=Node3D.new()
 head_pivot.name="V315_AnimatedHead"
 head_pivot.position=Vector3(0.64,0.44,0.0) if species!="owl" else Vector3(0.22,0.62,0)
 root.add_child(head_pivot)
 var head:=part(head_pivot,species,"head",Vector3(0.17,0,0),true)
 var tail_pivot:=Node3D.new()
 tail_pivot.name="V315_AnimatedTail"
 tail_pivot.position=Vector3(-0.76,0.15,0)
 root.add_child(tail_pivot)
 var tail:=part(tail_pivot,species,"tail",Vector3(-0.34,0.0,0.0))
 if species=="serpent":
  body.scale=Vector3(1.4,0.7,1.0)
  tail.scale=Vector3(1.2,0.85,0.9)
 elif species=="owl":
  body.scale=Vector3(0.86,1.40,1.0)
 elif species in ["boar","elk"]:
  body.scale=Vector3(1.10,1.12,1.05)
 var trim:=StandardMaterial3D.new()
 trim.albedo_color=Color("a69c86")
 trim.roughness=0.78
 var features:Array[Node3D]=[]
 if species in ["wolf","fox","panther","owl"]:
  for side in [-1.0,1.0]:
   var ear:=MeshInstance3D.new()
   var cone:=CylinderMesh.new()
   cone.bottom_radius=0.16 if species!="owl" else 0.12
   cone.top_radius=0.005
   cone.height=0.34 if species!="owl" else 0.18
   cone.radial_segments=7
   ear.mesh=cone;ear.material_override=coat(species,true)
   ear.position=Vector3(0.01,0.29,side*0.24)
   ear.name="V315_Ear"
   head_pivot.add_child(ear)
   features.append(ear)
 if species in ["stag","elk"]:
  for side in [-1.0,1.0]:
   var horn:=MeshInstance3D.new()
   var cylinder:=CylinderMesh.new()
   cylinder.height=0.85 if species=="elk" else 0.63
   cylinder.top_radius=0.018;cylinder.bottom_radius=0.075
   cylinder.radial_segments=7
   horn.mesh=cylinder;horn.material_override=trim
   horn.position=Vector3(0.02,0.50,side*0.24)
   horn.rotation_degrees.z=side*21.0
   horn.name="V315_Antler"
   head_pivot.add_child(horn)
   features.append(horn)
 var wings:Array[Node3D]=[]
 if species=="owl":
  for side in [-1.0,1.0]:
   var pivot:=Node3D.new()
   pivot.name="V315_FeatherWing"
   pivot.position=Vector3(-0.12,0.37,side*0.34)
   root.add_child(pivot)
   var wing:=part(pivot,"owl","tail",Vector3(-0.12,0,side*0.25),true)
   wing.scale=Vector3(0.62,0.65,1.7)
   wings.append(pivot)
 var weakpoint:MeshInstance3D=null
 var crowns:Array[Node3D]=[]
 if boss:
  # Bosses gain actual additional 3D heads, plus an illuminated phase weak point.
  for side in [-1.0,1.0]:
   var extra:=Node3D.new()
   extra.name="V315_BossExtraHead"
   extra.position=Vector3(0.20,0.70,side*0.71)
   root.add_child(extra)
   var extra_head:=part(extra,species,"head",Vector3(0.30,0,0),true)
   extra_head.scale=Vector3(0.84,0.84,0.84)
   crowns.append(extra)
  weakpoint=MeshInstance3D.new()
  weakpoint.name="V315_BossWeakpoint"
  var sphere:=SphereMesh.new()
  sphere.radius=0.23;sphere.height=0.46
  weakpoint.mesh=sphere
  var emissive:=StandardMaterial3D.new()
  emissive.albedo_color=Color("ff985e")
  emissive.emission_enabled=true
  emissive.emission=Color("ff492e")
  emissive.emission_energy_multiplier=1.8
  weakpoint.material_override=emissive
  weakpoint.position=Vector3(0.15,0.70,0.0)
  root.add_child(weakpoint)
  weakpoint.visible=false
 return {"root":root,"body":body,"head":head_pivot,"tail":tail_pivot,"wings":wings,"crowns":crowns,"weakpoint":weakpoint,"features":features}
