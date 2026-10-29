extends RefCounted
const V385_ASSETS:=preload("res://scripts/systems/witchwood_assets_v385.gd")
## Witchwood Wilds visual-only vertical slice. Deterministic geometry, no collision,
## gameplay RNG, character texture changes, world modes, or per-frame mesh creation.
const TREE_COUNT:int=48
const STONE_COUNT:int=32
const MOTE_COUNT:int=28
const SEED:int=2026384

static func _material(tint:Color,metallic:float=0.0,glow:bool=false)->StandardMaterial3D:
 var mat:=StandardMaterial3D.new()
 mat.albedo_color=tint
 mat.roughness=0.91 if not glow else 0.45
 mat.metallic=metallic
 if glow:
  mat.emission_enabled=true
  mat.emission=tint
  mat.emission_energy_multiplier=1.65
 return mat

static func _batch(root:Node3D,name:String,mesh:Mesh,material:Material,count:int)->MultiMeshInstance3D:
 var multi:=MultiMesh.new()
 multi.transform_format=MultiMesh.TRANSFORM_3D
 multi.use_colors=true
 multi.mesh=mesh
 multi.instance_count=count
 var result:=MultiMeshInstance3D.new()
 result.name=name
 result.multimesh=multi
 result.material_override=material
 result.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 root.add_child(result)
 return result

static func _stone(root:Node3D,name:String,at:Vector3,size:Vector3,mat:Material,tilt:float=0.0)->MeshInstance3D:
 var slab:=BoxMesh.new()
 slab.size=size
 var item:=MeshInstance3D.new()
 item.name=name
 item.mesh=slab
 item.material_override=mat
 item.position=at
 item.rotation_degrees.z=tilt
 item.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 root.add_child(item)
 return item

static func build(region:Node3D,center:Vector2)->Dictionary:
 var root:=Node3D.new()
 root.name="WitchwoodCinematicSlice"
 root.position=Vector3(center.x,0.0,center.y)
 region.add_child(root)
 var rng:=RandomNumberGenerator.new()
 rng.seed=SEED
 # Original handcrafted-by-code 3D silhouettes, shared by all 48 trees.
 var bark_mesh:Mesh=V385_ASSETS.make_moonwood_trunk()
 var crown_mesh:Mesh=V385_ASSETS.make_moonwood_canopy()
 var stone_mesh:Mesh=V385_ASSETS.make_mossy_rock()
 var mote_mesh:=SphereMesh.new()
 mote_mesh.radius=0.07
 mote_mesh.height=0.14
 mote_mesh.radial_segments=6
 mote_mesh.rings=3
 var bark:=_batch(root,"MoonwoodTrunks",bark_mesh,V385_ASSETS.make_surface_material("bark",Color("353037")),TREE_COUNT)
 var crowns:=_batch(root,"MoonwoodCanopies",crown_mesh,V385_ASSETS.make_surface_material("foliage",Color("345c50")),TREE_COUNT*2)
 var stones:=_batch(root,"WeatheredMossStones",stone_mesh,V385_ASSETS.make_surface_material("stone",Color("63676b")),STONE_COUNT)
 var motes:=_batch(root,"WitchwoodFireflies",mote_mesh,_material(Color("8dc5bd"),0.0,true),MOTE_COUNT)
 var placed:int=0
 var attempts:int=0
 while placed<TREE_COUNT and attempts<800:
  attempts+=1
  var x:float=rng.randf_range(-27.0,27.0)
  var z:float=rng.randf_range(-27.0,27.0)
  # Paths, shrine, encounter space and existing landmark stay visually clear.
  if absf(x)<8.0 or absf(z)<7.0:continue
  if Vector2(x-16.0,z-16.0).length_squared()<72.0:continue
  if Vector2(x-3.0,z+7.5).length_squared()<60.0:continue
  var scale_factor:float=rng.randf_range(0.82,1.45)
  var orientation:Basis=Basis(Vector3.UP,rng.randf_range(-PI,PI))
  var trunk_basis:Basis=orientation.scaled(Vector3(scale_factor,scale_factor,scale_factor))
  bark.multimesh.set_instance_transform(placed,Transform3D(trunk_basis,Vector3(x,1.90*scale_factor,z)))
  bark.multimesh.set_instance_color(placed,Color(rng.randf_range(0.78,1.0),rng.randf_range(0.85,1.0),rng.randf_range(0.84,0.98)))
  for crown_index in range(2):
   var cscale:float=scale_factor*(1.0 if crown_index==0 else 0.72)
   var crown_basis:Basis=orientation.scaled(Vector3(cscale,cscale,cscale))
   var height:float=(3.85 if crown_index==0 else 5.10)*scale_factor
   var entry:int=placed*2+crown_index
   crowns.multimesh.set_instance_transform(entry,Transform3D(crown_basis,Vector3(x,height,z)))
   crowns.multimesh.set_instance_color(entry,Color(rng.randf_range(0.7,0.95),rng.randf_range(0.77,1.0),rng.randf_range(0.8,1.0)))
  placed+=1
 # Bounds are fixed; scene construction never consumes gameplay's RNG.
 for i in range(STONE_COUNT):
  var x:float=rng.randf_range(-26.5,26.5)
  var z:float=rng.randf_range(-25.5,25.5)
  if absf(z)<4.0:z=4.5 if z>=0.0 else -4.5
  var factor:float=rng.randf_range(0.3,1.1)
  var pose:=Basis(Vector3.UP,rng.randf_range(-PI,PI)).scaled(Vector3(factor,0.34*factor,factor))
  stones.multimesh.set_instance_transform(i,Transform3D(pose,Vector3(x,0.17*factor,z)))
  stones.multimesh.set_instance_color(i,Color(rng.randf_range(0.73,0.93),rng.randf_range(0.78,1.0),rng.randf_range(0.71,0.9)))
 for i in range(MOTE_COUNT):
  var x:float=rng.randf_range(-22.0,22.0)
  var z:float=rng.randf_range(-20.0,20.0)
  var s:float=rng.randf_range(0.65,1.3)
  motes.multimesh.set_instance_transform(i,Transform3D(Basis.IDENTITY.scaled(Vector3.ONE*s),Vector3(x,rng.randf_range(1.7,4.5),z)))
  motes.multimesh.set_instance_color(i,Color(rng.randf_range(0.65,0.9),1.0,rng.randf_range(0.81,1.0)))
 # Leaning, fractured shrine ruins share the region's existing world geometry.
 var ruins:=Node3D.new()
 ruins.name="MoonwoodRuinGate"
 ruins.position=Vector3(15.0,0.0,16.0)
 root.add_child(ruins)
 var stone_mat:=V385_ASSETS.make_surface_material("stone",Color("616777"))
 _stone(ruins,"LeftMoonPillar",Vector3(-3.6,1.5,0.0),Vector3(1.0,3.0,1.2),stone_mat,-7.0)
 _stone(ruins,"RightMoonPillar",Vector3(3.6,1.45,0.0),Vector3(1.0,2.9,1.2),stone_mat,9.0)
 _stone(ruins,"BrokenMoonLintel",Vector3(0.0,3.05,0.0),Vector3(6.0,0.8,0.95),stone_mat,3.0)
 _stone(ruins,"AncientThreshold",Vector3(0.0,0.20,-0.35),Vector3(7.8,0.40,2.4),stone_mat)
 var rune_mat:=_material(Color("62b6ba"),0.05,true)
 var rune_mesh:=TorusMesh.new()
 rune_mesh.inner_radius=0.84
 rune_mesh.outer_radius=1.01
 rune_mesh.rings=12
 rune_mesh.ring_segments=18
 var rune:=MeshInstance3D.new()
 rune.name="UnbrokenMoonRune"
 rune.mesh=rune_mesh
 rune.material_override=rune_mat
 rune.position=Vector3(0.0,2.15,-0.72)
 rune.rotation_degrees.x=90.0
 rune.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 ruins.add_child(rune)
 var moon_glow:=OmniLight3D.new()
 moon_glow.name="WitchwoodMoonLight"
 moon_glow.position=Vector3(0.0,2.4,-0.6)
 moon_glow.light_color=Color("7aaec6")
 moon_glow.light_energy=0.48
 moon_glow.omni_range=8.0
 moon_glow.shadow_enabled=false
 ruins.add_child(moon_glow)
 return {"root":root,"trunks":bark,"crowns":crowns,"stones":stones,"motes":motes,"ruins":ruins,"light":moon_glow,"tree_count":placed}

static func apply_quality(slice:Dictionary,settings:Dictionary)->void:
 if slice.is_empty():return
 var detail:int=clampi(int(settings.get("environment_detail","7")),1,10)
 var foliage:String=str(settings.get("foliage_quality","High"))
 var clutter:String=str(settings.get("ground_clutter","High"))
 var visible_forest:bool=detail>=3 and foliage!="Off" and clutter!="Off"
 var factor:float=0.0 if not visible_forest else 0.33 if foliage=="Low" or detail<5 else 0.67 if foliage=="Medium" or detail<8 else 1.0
 var tree_limit:int=int(floor(float(TREE_COUNT)*factor))
 var trunks:MultiMeshInstance3D=slice["trunks"]
 var canopies:MultiMeshInstance3D=slice["crowns"]
 trunks.visible=tree_limit>0
 canopies.visible=tree_limit>0
 trunks.multimesh.visible_instance_count=tree_limit
 canopies.multimesh.visible_instance_count=tree_limit*2
 var shadows:bool=detail>=8 and str(settings.get("shadow_quality","Off")) in ["High","Ultra"]
 trunks.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_ON if shadows else GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 canopies.cast_shadow=trunks.cast_shadow
 var stone_count:int=STONE_COUNT if detail>=7 else 12 if detail>=4 else 0
 var stones:MultiMeshInstance3D=slice["stones"]
 # Normal maps are a near-field detail feature; low presets skip the extra samples.
 for batch in [trunks,canopies,stones]:
  var mat:StandardMaterial3D=batch.material_override as StandardMaterial3D
  if is_instance_valid(mat):mat.normal_enabled=detail>=6
 stones.visible=stone_count>0 and clutter!="Off"
 stones.multimesh.visible_instance_count=stone_count if stones.visible else 0
 var fireflies:MultiMeshInstance3D=slice["motes"]
 fireflies.visible=detail>=6 and str(settings.get("particle_density","Low"))!="Low"
 fireflies.multimesh.visible_instance_count=MOTE_COUNT if fireflies.visible else 0
 var ruins:Node3D=slice["ruins"]
 ruins.visible=detail>=3
 var light:OmniLight3D=slice["light"]
 light.visible=ruins.visible and detail>=7
 var limit:float=25.0 if str(settings.get("scenery_lod","Balanced"))=="Near" else 75.0 if str(settings.get("scenery_lod","Balanced"))=="Far" else 48.0
 for batch in [trunks,canopies,stones,fireflies]:
  batch.visibility_range_end=limit
