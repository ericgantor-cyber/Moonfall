extends RefCounted
## Lightweight taxon silhouettes on inherited native wildlife actors.
## Decorations are noncolliding and cannot alter AI, combat or hero art.
## These are recognizable generic silhouettes, not bespoke 3D species models.
static var _mesh_cache:Dictionary={}
static var _material_cache:Dictionary={}

static func _sphere()->SphereMesh:
 if not _mesh_cache.has("sphere"):
  var mesh:=SphereMesh.new()
  mesh.radius=0.25
  mesh.height=0.5
  mesh.radial_segments=8
  mesh.rings=4
  _mesh_cache["sphere"]=mesh
 return _mesh_cache["sphere"] as SphereMesh

static func _fin()->BoxMesh:
 if not _mesh_cache.has("fin"):
  var mesh:=BoxMesh.new()
  mesh.size=Vector3(0.06,0.48,0.33)
  _mesh_cache["fin"]=mesh
 return _mesh_cache["fin"] as BoxMesh

static func _beak()->CylinderMesh:
 if not _mesh_cache.has("beak"):
  var mesh:=CylinderMesh.new()
  mesh.top_radius=0.015
  mesh.bottom_radius=0.14
  mesh.height=0.4
  mesh.radial_segments=6
  _mesh_cache["beak"]=mesh
 return _mesh_cache["beak"] as CylinderMesh

static func _mat(taxon:String)->StandardMaterial3D:
 if not _material_cache.has(taxon):
  var palette:Dictionary={"M":"ad987b","B":"b9b4d8","R":"4aa68d","A":"75b3c4"}
  var material:=StandardMaterial3D.new()
  material.albedo_color=Color(str(palette.get(taxon,"ad987b")))
  material.roughness=0.89
  _material_cache[taxon]=material
 return _material_cache[taxon] as StandardMaterial3D

static func _part(parent:Node3D,name:String,mesh:Mesh,position:Vector3,scale:Vector3,taxon:String)->MeshInstance3D:
 var part:=MeshInstance3D.new()
 part.name=name
 part.mesh=mesh
 part.position=position
 part.scale=scale
 part.material_override=_mat(taxon)
 part.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 parent.add_child(part)
 return part

static func attach(body:CharacterBody3D,taxon:String,species_name:String)->Node3D:
 if not is_instance_valid(body):return null
 var prior:Node3D=body.get_node_or_null("AmericasWildlifeSilhouette") as Node3D
 if is_instance_valid(prior):prior.queue_free()
 var root:=Node3D.new()
 root.name="AmericasWildlifeSilhouette"
 body.add_child(root)
 match taxon:
  "B":
   # Wings, chest and a projecting beak around the original body rig.
   _part(root,"LeftWing",_fin(),Vector3(0,0.15,-0.42),Vector3(1,1.35,2.5),"B")
   _part(root,"RightWing",_fin(),Vector3(0,0.15,0.42),Vector3(1,1.35,2.5),"B")
   var beak:=_part(root,"Beak",_beak(),Vector3(0.78,0.18,0),Vector3.ONE,"B")
   beak.rotation.z=PI*0.5
  "R":
   # Dorsal plates and tail ridge mark reptilian profiles.
   _part(root,"DorsalRidge",_fin(),Vector3(-0.28,0.29,0),Vector3(1,0.76,1.3),"R")
   _part(root,"Tail",_beak(),Vector3(-0.77,-0.08,0),Vector3(1.0,1.45,1.0),"R").rotation.z=-PI*0.5
  "A":
   # Broad eye nodes and low shoulder pads for amphibian profiles.
   _part(root,"LeftEye",_sphere(),Vector3(0.42,0.39,-0.23),Vector3(0.6,0.64,0.6),"A")
   _part(root,"RightEye",_sphere(),Vector3(0.42,0.39,0.23),Vector3(0.6,0.64,0.6),"A")
  _:
   # Mammals retain their original inherited art, gaining ears or horns.
   var antlered:bool=("deer" in species_name.to_lower() or "elk" in species_name.to_lower() or "moose" in species_name.to_lower() or "sheep" in species_name.to_lower() or "bison" in species_name.to_lower())
   var side_scale:Vector3=Vector3(0.4,1.5 if antlered else 0.72,0.48)
   _part(root,"LeftEarOrHorn",_sphere(),Vector3(0.47,0.43,-0.26),side_scale,"M")
   _part(root,"RightEarOrHorn",_sphere(),Vector3(0.47,0.43,0.26),side_scale,"M")
 return root

static func animate(root:Node3D,taxon:String,elapsed:float,phase:float)->void:
 if not is_instance_valid(root):return
 # Visual-only movement, bounded and sparse enough for slow scan intervals.
 match taxon:
  "B":root.rotation.z=sin(elapsed*2.5+phase)*0.10
  "R":root.rotation.y=sin(elapsed*1.3+phase)*0.09
  "A":root.position.y=sin(elapsed*1.7+phase)*0.035
  _:root.rotation.z=sin(elapsed*1.0+phase)*0.035
