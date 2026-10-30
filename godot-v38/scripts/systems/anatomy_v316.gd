extends RefCounted
## Reusable, purely cosmetic anatomy over the v31.5 organic ArrayMesh.
## Actual 3D nodes/meshes, no colliders, AI actors, or saved state.
const QUADRUPEDS:Array[String]=["wolf","stag","boar","fox","panther","elk"]
static var mesh_cache:Dictionary={}
static var material_cache:Dictionary={}

static func _sphere(radius:float)->SphereMesh:
 var key:String="sphere:%.3f"%radius
 if not mesh_cache.has(key):
  var mesh:=SphereMesh.new()
  mesh.radius=radius
  mesh.height=radius*2.0
  mesh.radial_segments=10
  mesh.rings=5
  mesh_cache[key]=mesh
 return mesh_cache[key]

static func _limb_mesh(radius:float)->CapsuleMesh:
 var key:String="limb:%.3f"%radius
 if not mesh_cache.has(key):
  var mesh:=CapsuleMesh.new()
  mesh.radius=radius
  mesh.height=0.54
  mesh.radial_segments=8
  mesh.rings=3
  mesh_cache[key]=mesh
 return mesh_cache[key]

static func _material(key:String,color:Color,emissive:bool=false)->StandardMaterial3D:
 if not material_cache.has(key):
  var mat:=StandardMaterial3D.new()
  mat.albedo_color=color
  mat.roughness=0.80
  if emissive:
   mat.emission_enabled=true
   mat.emission=color
   mat.emission_energy_multiplier=1.1
  material_cache[key]=mat
 return material_cache[key]

static func _piece(parent:Node3D,name:String,mesh:Mesh,material:Material,at:Vector3,scale3d:Vector3=Vector3.ONE)->MeshInstance3D:
 var node:=MeshInstance3D.new()
 node.name=name
 node.mesh=mesh
 node.material_override=material
 node.position=at
 node.scale=scale3d
 node.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 parent.add_child(node)
 return node

static func decorate(geometry:Dictionary,species:String,boss:bool=false)->Dictionary:
 var root:Node3D=geometry["root"]
 var head:Node3D=geometry["head"]
 var coat:Color=Color(str({"wolf":"656771","stag":"89694d","boar":"694e43","fox":"be744f","serpent":"437f72","panther":"373b4b","elk":"76614c","owl":"9992a1"}.get(species,"656771")))
 var fur:Material=_material(species+":limb",coat.darkened(0.10))
 var feet:Material=_material(species+":feet",coat.darkened(0.28))
 var eyes:Material=_material(species+":eyes",Color("f5bf72") if species not in ["serpent","owl"] else Color("ecab66"),boss)
 var muzzle:Material=_material(species+":muzzle",coat.lightened(0.12))
 var legs:Array[Node3D]=[]
 if species in QUADRUPEDS:
  for front in [-1.0,1.0]:
   for side in [-1.0,1.0]:
    var anchor:=Node3D.new()
    anchor.name="V316_LegHinge"
    anchor.position=Vector3(front*0.48,0.02,side*0.28)
    root.add_child(anchor)
    _piece(anchor,"V316_ArticulatedLeg",_limb_mesh(0.105 if species in ["boar","elk"] else 0.078),fur,Vector3(0,-0.24,0))
    _piece(anchor,"V316_Paw",_sphere(0.13),feet,Vector3(0.12,-0.50,0),Vector3(1.25,0.6,1.0))
    legs.append(anchor)
 elif species=="owl":
  for side in [-1.0,1.0]:
   var perch:=Node3D.new()
   perch.name="V316_OwlTalons"
   perch.position=Vector3(-0.05,-0.17,side*0.18)
   root.add_child(perch)
   _piece(perch,"V316_Talon",_limb_mesh(0.052),feet,Vector3(0,-0.19,0),Vector3(0.78,0.65,0.78))
   legs.append(perch)
 else:
  # Serpents move by body undulation rather than implausible mammal legs.
  var ridge_mesh:Mesh=_sphere(0.10)
  for i in range(7):
   var offset:float=float(i)/6.0
   _piece(root,"V316_SerpentScales",ridge_mesh,fur,Vector3(-0.75+offset*1.5,0.48-0.08*absf(offset-0.5),0.0),Vector3(1.15,0.55,1.5))
 # Head-specific anatomy stays on the original animated head pivot.
 for side in [-1.0,1.0]:
  _piece(head,"V316_Eye",_sphere(0.090 if species!="owl" else 0.13),eyes,Vector3(0.28,0.12,side*(0.22 if species!="owl" else 0.27)))
 _piece(head,"V316_Muzzle",_sphere(0.18 if species not in ["owl","serpent"] else 0.11),muzzle,Vector3(0.43,-0.08,0),Vector3(1.4,0.72,1.1))
 if species=="boar":
  var ivory:Material=_material("ivory",Color("d3be9e"))
  for side in [-1.0,1.0]:
   _piece(head,"V316_Tusk",_sphere(0.12),ivory,Vector3(0.33,-0.14,side*0.24),Vector3(0.75,1.25,0.65))
 if species=="owl":
  _piece(head,"V316_Beak",_sphere(0.11),_material("beak",Color("c6aa65")),Vector3(0.49,-0.06,0),Vector3(1.8,0.75,0.75))
 var boss_aura:MeshInstance3D=null
 if boss:
  boss_aura=_piece(root,"V316_BossCore",_sphere(0.32),_material("boss_aura",Color("ef7354"),true),Vector3(-0.15,0.53,0.0),Vector3(0.95,0.45,1.1))
  boss_aura.visible=false
 return {"legs":legs,"aura":boss_aura,"gait_count":0,"species":species}
