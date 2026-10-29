extends RefCounted
## Visual-only Witchwood floor detailing. Four bounded GPU batches, one seeded
## generator, no physics, no save keys and no allocations during gameplay.
const MOSS_COUNT:int=96
const FERN_COUNT:int=72
const LOG_COUNT:int=16
const ROOT_COUNT:int=24
const SEED:int=2026386

static func _batch(root:Node3D,label:String,mesh:Mesh,mat:Material,count:int)->MultiMeshInstance3D:
 var mm:=MultiMesh.new()
 mm.transform_format=MultiMesh.TRANSFORM_3D
 mm.use_colors=true
 mm.mesh=mesh
 mm.instance_count=count
 var node:=MultiMeshInstance3D.new()
 node.name=label
 node.multimesh=mm
 node.material_override=mat
 node.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 root.add_child(node)
 return node

static func _moss_texture()->ImageTexture:
 # A radial RGBA patch, generated once per session with a soft irregular edge.
 var size:int=64
 var img:=Image.create(size,size,false,Image.FORMAT_RGBA8)
 for y in range(size):
  for x in range(size):
   var uv:Vector2=Vector2((float(x)+0.5)/float(size),(float(y)+0.5)/float(size))
   var radius:float=(uv-Vector2(0.5,0.5)).length()*2.0
   var noise:float=0.5+0.5*sin(float(x)*0.43+sin(float(y)*0.19)*1.9)*cos(float(y)*0.32)
   var edge:float=clampf((1.04-radius+0.13*(noise-0.5))*3.1,0.0,1.0)
   var tint:float=lerpf(0.78,1.0,noise)
   img.set_pixel(x,y,Color(0.68*tint,0.92*tint,0.73*tint,edge*0.56))
 img.generate_mipmaps()
 return ImageTexture.create_from_image(img)

static func _moss_mesh()->QuadMesh:
 var mesh:=QuadMesh.new()
 mesh.size=Vector2(2.2,1.8)
 return mesh

static func _fern_mesh()->ArrayMesh:
 var st:=SurfaceTool.new()
 st.begin(Mesh.PRIMITIVE_TRIANGLES)
 # Eight asymmetric fronds. Each makes a small three-dimensional curved leaf
 # around the central crown rather than an intersecting flat billboard cross.
 for i in range(8):
  var angle:float=float(i)*TAU/8.0
  var direction:Vector3=Vector3(cos(angle),0.0,sin(angle))
  var across:Vector3=Vector3(-direction.z,0.0,direction.x)
  var stem:Vector3=Vector3(0.0,0.12,0.0)
  var middle:Vector3=direction*0.26+Vector3(0.0,0.50,0.0)
  var tip:Vector3=direction*0.66+Vector3(0.0,0.27,0.0)
  var left:Vector3=middle+across*0.16
  var right:Vector3=middle-across*0.16
  var n:Vector3=(across.cross(direction)).normalized()
  st.set_normal(n)
  st.set_uv(Vector2(0.5,1.0))
  st.add_vertex(stem)
  st.set_normal(n)
  st.set_uv(Vector2(0.0,0.52))
  st.add_vertex(left)
  st.set_normal(n)
  st.set_uv(Vector2(0.5,0.0))
  st.add_vertex(tip)
  st.set_normal(n)
  st.set_uv(Vector2(0.5,1.0))
  st.add_vertex(stem)
  st.set_normal(n)
  st.set_uv(Vector2(0.5,0.0))
  st.add_vertex(tip)
  st.set_normal(n)
  st.set_uv(Vector2(1.0,0.52))
  st.add_vertex(right)
 st.generate_tangents()
 return st.commit()

static func _fern_material()->StandardMaterial3D:
 var mat:=StandardMaterial3D.new()
 mat.albedo_color=Color("426c56")
 mat.roughness=0.95
 mat.cull_mode=BaseMaterial3D.CULL_DISABLED
 mat.vertex_color_use_as_albedo=true
 return mat

static func _moss_material()->StandardMaterial3D:
 var mat:=StandardMaterial3D.new()
 mat.albedo_texture=_moss_texture()
 mat.albedo_color=Color("567c64")
 mat.roughness=0.97
 mat.vertex_color_use_as_albedo=true
 mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 mat.cull_mode=BaseMaterial3D.CULL_DISABLED
 mat.depth_draw_mode=BaseMaterial3D.DEPTH_DRAW_OPAQUE_ONLY
 return mat

static func _wood_material()->StandardMaterial3D:
 var mat:=StandardMaterial3D.new()
 mat.albedo_color=Color("393436")
 mat.roughness=0.92
 mat.vertex_color_use_as_albedo=true
 return mat

static func _valid_spot(x:float,z:float)->bool:
 # Important: no clutter over the established connected roads, travel shrine,
 # the existing moonwood monument, or the combat arena center.
 if absf(x)<8.0 or absf(z)<7.0:return false
 if Vector2(x-3.0,z+7.5).length_squared()<50.0:return false
 if Vector2(x-16.0,z-16.0).length_squared()<75.0:return false
 return true

static func build(witchwood_root:Node3D)->Dictionary:
 var root:=Node3D.new()
 root.name="ForestFloorDetail"
 witchwood_root.add_child(root)
 var moss:=_batch(root,"LeafMossPatches",_moss_mesh(),_moss_material(),MOSS_COUNT)
 # Fix the transparent moss layer above the existing flat ground as a whole.
 # This position is independent of GPU MultiMesh per-instance readback.
 moss.position.y=0.045
 var ferns:=_batch(root,"MoonFernClumps",_fern_mesh(),_fern_material(),FERN_COUNT)
 var log_mesh:=CylinderMesh.new()
 log_mesh.top_radius=0.25
 log_mesh.bottom_radius=0.30
 log_mesh.height=2.7
 log_mesh.radial_segments=8
 var logs:=_batch(root,"FallenMoonwood",log_mesh,_wood_material(),LOG_COUNT)
 var root_mesh:=CylinderMesh.new()
 root_mesh.top_radius=0.075
 root_mesh.bottom_radius=0.11
 root_mesh.height=1.5
 root_mesh.radial_segments=6
 var roots:=_batch(root,"ExposedMoonwoodRoots",root_mesh,_wood_material(),ROOT_COUNT)
 var rng:=RandomNumberGenerator.new()
 rng.seed=SEED
 var batches:Array=[moss,ferns,logs,roots]
 var limits:Array[int]=[MOSS_COUNT,FERN_COUNT,LOG_COUNT,ROOT_COUNT]
 for kind in range(batches.size()):
  var batch:MultiMeshInstance3D=batches[kind]
  var placed:int=0
  var trials:int=0
  while placed<limits[kind] and trials<2400:
   trials+=1
   var x:float=rng.randf_range(-27.0,27.0)
   var z:float=rng.randf_range(-26.0,26.0)
   if not _valid_spot(x,z):continue
   var angle:float=rng.randf_range(-PI,PI)
   var scale:float=rng.randf_range(0.72,1.34)
   var transform:Transform3D=Transform3D.IDENTITY
   match kind:
    0:
     var ground_basis:Basis=Basis(Vector3.UP,angle)*Basis(Vector3.RIGHT,-PI*0.5)
     transform=Transform3D(ground_basis.scaled(Vector3(scale,scale,scale)),Vector3(x,0.0,z))
    1:
     transform=Transform3D(Basis(Vector3.UP,angle).scaled(Vector3.ONE*scale),Vector3(x,0.015,z))
    2:
     var log_basis:Basis=Basis(Vector3.UP,angle)*Basis(Vector3.FORWARD,PI*0.5)
     transform=Transform3D(log_basis.scaled(Vector3(0.70,scale,0.70)),Vector3(x,0.23,z))
    3:
     var root_basis:Basis=Basis(Vector3.UP,angle)*Basis(Vector3.FORWARD,PI*0.5)
     transform=Transform3D(root_basis.scaled(Vector3(0.72,scale,0.72)),Vector3(x,0.08,z))
   batch.multimesh.set_instance_transform(placed,transform)
   batch.multimesh.set_instance_color(placed,Color(rng.randf_range(0.80,0.99),rng.randf_range(0.79,0.99),rng.randf_range(0.83,1.0),1.0))
   placed+=1
 return {"root":root,"moss":moss,"ferns":ferns,"logs":logs,"roots":roots,"batch_count":4}

static func apply_quality(floor:Dictionary,settings:Dictionary)->void:
 if floor.is_empty():return
 var detail:int=clampi(int(settings.get("environment_detail","7")),1,10)
 var clutter:String=str(settings.get("ground_clutter","High"))
 var foliage:String=str(settings.get("foliage_quality","High"))
 var density:float=0.0 if detail<4 or clutter=="Off" else 0.33 if clutter=="Low" or detail<6 else 0.66 if clutter=="Medium" or detail<8 else 1.0
 var counts:Array[int]=[MOSS_COUNT,FERN_COUNT,LOG_COUNT,ROOT_COUNT]
 var nodes:Array=[floor["moss"],floor["ferns"],floor["logs"],floor["roots"]]
 var lod:String=str(settings.get("scenery_lod","Balanced"))
 var limit:float=26.0 if lod=="Near" else 72.0 if lod=="Far" else 48.0
 for i in range(nodes.size()):
  var node:MultiMeshInstance3D=nodes[i] as MultiMeshInstance3D
  var factor:float=density
  if i==1 and foliage=="Off":factor=0.0
  if i>=2 and detail<5:factor=0.0
  var count:int=clampi(int(floorf(float(counts[i])*factor)),0,counts[i])
  node.visible=count>0
  node.multimesh.visible_instance_count=count
  node.visibility_range_end=limit
