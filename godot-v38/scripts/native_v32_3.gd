extends "res://scripts/native_v32_2.gd"
## v32.3: lightweight chamber floor detail and champion arena readability.
## Visual-only: no new collision, enemy AI, save data, or asset modification.
var v323_floor_markers:int=0
var v323_champion_sigils:int=0

func _release_label()->String:return "v32.3"

func _build_scene()->void:
 super._build_scene()
 _v323_decorate_chambers()

func _v323_decorate_chambers()->void:
 var line_mesh:=BoxMesh.new()
 line_mesh.size=Vector3(1.0,0.025,0.10)
 var dot_mesh:=CylinderMesh.new()
 dot_mesh.top_radius=0.14
 dot_mesh.bottom_radius=0.14
 dot_mesh.height=0.035
 dot_mesh.radial_segments=8
 var ring_mesh:=CylinderMesh.new()
 ring_mesh.top_radius=2.65
 ring_mesh.bottom_radius=2.65
 ring_mesh.height=0.018
 ring_mesh.radial_segments=32
 var center_mesh:=CylinderMesh.new()
 center_mesh.top_radius=2.22
 center_mesh.bottom_radius=2.22
 center_mesh.height=0.021
 center_mesh.radial_segments=32
 for chamber in v321_chambers:
  var root:Node3D=chamber["node"]
  if not is_instance_valid(root):continue
  var stage:int=int(chamber["stage"])
  var site:int=int(chamber["site"])
  if site<0 or site>=rifts.size():continue
  var zone:int=int(rifts[site]["zone"])
  var tint:Color=Color(str(REGIONS[zone]["light"]))
  var mat:=StandardMaterial3D.new()
  mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
  mat.albedo_color=tint.darkened(0.25)
  mat.emission_enabled=true
  mat.emission=tint.darkened(0.45)
  mat.no_depth_test=false
  # Distinct stage silhouettes: radial waypoints, crossing paths, boss arena ring.
  if stage==1:
   for i in range(6):
    var theta:float=float(i)*TAU/6.0
    var marker:=MeshInstance3D.new()
    marker.mesh=dot_mesh
    marker.material_override=mat
    marker.position=Vector3(cos(theta)*2.5,0.092,sin(theta)*2.5)
    marker.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    root.add_child(marker)
    v323_floor_markers+=1
  elif stage==2:
   for i in range(4):
    var stripe:=MeshInstance3D.new()
    stripe.mesh=line_mesh
    stripe.material_override=mat
    stripe.scale=Vector3(4.8,1,1)
    stripe.position=Vector3(0,0.095,(float(i)-1.5)*1.1)
    stripe.rotation.y=PI/4.0 if i%2==0 else -PI/4.0
    stripe.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    root.add_child(stripe)
    v323_floor_markers+=1
  else:
   var rim:=MeshInstance3D.new()
   rim.mesh=ring_mesh
   rim.material_override=mat
   rim.position.y=0.087
   rim.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   root.add_child(rim)
   # Recess the center slightly to create a visible circular arena outline.
   var center:=MeshInstance3D.new()
   center.mesh=center_mesh
   var center_mat:=StandardMaterial3D.new()
   center_mat.albedo_color=tint.darkened(0.84)
   center_mat.roughness=1.0
   center.material_override=center_mat
   center.position.y=0.101
   center.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   root.add_child(center)
   v323_champion_sigils+=1

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.3"
 report["chamber_floor_markers"]=v323_floor_markers
 report["champion_arena_sigils"]=v323_champion_sigils
 return report
