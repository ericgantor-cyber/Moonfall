extends "res://scripts/native_v37_3.gd"
## v37.4: solid underwater grottos and artwork-preserving swim presentation.
## Inherits six lakes, actual depth movement, oxygen, caches and one world.
const V374_SOLID_LAYERS:int=1
var v374_collision_bodies:Array[StaticBody3D]=[]
var v374_swim_pose_elapsed:float=0.0
var v374_swim_pose_anchor:Node3D
var v374_wall_material:StandardMaterial3D

func _ready()->void:
 super._ready()
 _v374_build_solid_boundaries()
 _v374_make_pose_anchor()

func _v374_make_pose_anchor()->void:
 # Dedicated pose anchor keeps the v16 attack/dodge animation untouched.
 # Its transform is absolute and resets when the hero leaves the water.
 if not is_instance_valid(hero_sprite):return
 var original_parent:Node3D=hero_sprite.get_parent() as Node3D
 if not is_instance_valid(original_parent):return
 v374_swim_pose_anchor=Node3D.new()
 v374_swim_pose_anchor.name="V380_SwimPoseAnchor"
 original_parent.add_child(v374_swim_pose_anchor)
 hero_sprite.reparent(v374_swim_pose_anchor,true)

func _v374_add_boundary(parent:Node3D,label:String,at:Vector3,dimensions:Vector3)->void:
 var body:=StaticBody3D.new()
 body.name=label
 body.collision_layer=V374_SOLID_LAYERS
 body.collision_mask=0
 body.position=at
 parent.add_child(body)
 var shape:=BoxShape3D.new()
 shape.size=dimensions
 var collision:=CollisionShape3D.new()
 collision.shape=shape
 body.add_child(collision)
 if label=="SolidFarWall" or label.begins_with("SolidSideWall"):
  var visible_wall:=MeshInstance3D.new()
  visible_wall.name="VisibleCaveStone"
  var wall_mesh:=BoxMesh.new()
  wall_mesh.size=dimensions
  visible_wall.mesh=wall_mesh
  visible_wall.material_override=v374_wall_material
  body.add_child(visible_wall)
 v374_collision_bodies.append(body)

func _v374_build_solid_boundaries()->void:
 if v373_caves.size()!=V373_RULES.REGION_COUNT:return
 v374_wall_material=_v361_material(Color("2d4c57"))
 for zone in range(V373_RULES.REGION_COUNT):
  var cave:Node3D=v373_caves[zone]
  if not is_instance_valid(cave):continue
  var mid:Vector2=REGIONS[zone]["at"]
  var offset:Vector2=V362_JOBS.initial_water_offset(zone)
  var cx:float=mid.x+offset.x
  var cz:float=mid.y+offset.y
  # Colliders use the SAME dimensions as the existing rendered lake floor
  # and lowered cave roof. No physics objects are created per frame.
  _v374_add_boundary(cave,"SolidLakeBed",Vector3(cx+1.6,-3.72,cz),Vector3(12.2,0.28,6.7))
  _v374_add_boundary(cave,"SolidCaveCeiling",Vector3(cx+4.35,-0.90,cz),Vector3(4.75,0.42,3.2))
  # Close the passage's far end and two long sides while keeping the western
  # mouth open to the swimming lake. Walls stay below the surface.
  _v374_add_boundary(cave,"SolidFarWall",Vector3(cx+7.18,-2.35,cz),Vector3(0.40,2.65,3.5))
  for side in [-1,1]:
   _v374_add_boundary(cave,"SolidSideWall_%d"%side,Vector3(cx+4.6,-2.35,cz+float(side)*1.73),Vector3(5.5,2.65,0.40))
  # Existing decorative posts also become obstacles at their exact locations.
  for post in range(4):
   var px:float=cx+2.3+float(post)*1.13
   for side in [-1,1]:
    _v374_add_boundary(cave,"SolidPillar_%d_%d"%[post,side],Vector3(px,-2.60,cz+float(side)*1.38),Vector3(0.40,2.2,0.40))

func _process(delta:float)->void:
 super._process(delta)
 if not is_instance_valid(v374_swim_pose_anchor):return
 var swimming:bool=hero_playing and not death_active and v37_swimming and v373_depth>V373_RULES.SUBMERGED_DEPTH
 if not swimming:
  v374_swim_pose_anchor.position=Vector3.ZERO
  v374_swim_pose_anchor.rotation=Vector3.ZERO
  v374_swim_pose_anchor.scale=Vector3.ONE
  return
 # Only the parent anchor moves. The original Sprite3D texture AND inherited
 # attack/dodge transforms remain independent; the scale cannot accumulate.
 v374_swim_pose_elapsed+=minf(delta,0.05)
 var phase:float=v374_swim_pose_elapsed*5.2
 v374_swim_pose_anchor.rotation.z=0.055*sin(phase)
 v374_swim_pose_anchor.position.y=0.08*sin(phase*1.15)
 v374_swim_pose_anchor.scale=Vector3(1.0+0.035*sin(phase),1.0-0.035*sin(phase),1.0)

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["solid_underwater_colliders"]=v374_collision_bodies.size()
 return report

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\nSolid deep-water grotto walls and floors now block the hero. Swimming uses gentle original-art posing; new 3D character rigs are not part of this update."
