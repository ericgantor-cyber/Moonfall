extends "res://scripts/native_v38_3.gd"
## Lightweight in-world companion presence. No new mode or character-art changes.
## This is following/navigation only; combat assistance requires separate testing.
var realm_follower:Node3D
var realm_follower_visual:MeshInstance3D
var realm_follower_last_ethic:String = ""
const FOLLOW_DISTANCE:float = 2.7
const FOLLOW_SPEED:float = 5.0
const FOLLOW_CATCHUP_SPEED:float = 14.0
const FOLLOW_CATCHUP_START:float = 5.0
const FOLLOW_TELEPORT_DISTANCE:float = 35.0

func _ready()->void:
 super._ready()
 _create_realm_follower()

func _create_realm_follower()->void:
 realm_follower = Node3D.new()
 realm_follower.name = "RealmCompanionFollower"
 add_child(realm_follower)
 realm_follower_visual = MeshInstance3D.new()
 realm_follower_visual.name = "CompanionMarker"
 var mesh:SphereMesh = SphereMesh.new()
 mesh.radius = 0.24
 mesh.height = 0.48
 realm_follower_visual.mesh = mesh
 var material:StandardMaterial3D = StandardMaterial3D.new()
 material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
 material.albedo_color = Color(0.35, 0.8, 1.0)
 realm_follower_visual.material_override = material
 realm_follower.add_child(realm_follower_visual)
 realm_follower.visible = false

func _process(delta:float)->void:
 super._process(delta)
 if not is_instance_valid(realm_follower):return
 var active:bool = hero_playing and not death_active and not menu_open and is_instance_valid(player)
 realm_follower.visible = active
 if not active:return
 var state:Dictionary = REALM_ETHICS.normalize(realm_ethics)
 var loyalty:int = REALM_ETHICS.companion_loyalty(state,realm_companion_ethic)
 var following:bool = loyalty >= 40
 realm_follower.visible = following
 if not following:return
 if realm_follower_last_ethic != realm_companion_ethic:
  realm_follower_last_ethic = realm_companion_ethic
  var mat:StandardMaterial3D = realm_follower_visual.material_override as StandardMaterial3D
  if is_instance_valid(mat):mat.albedo_color = Color(0.95,0.35,0.55) if realm_companion_ethic=="Evil" else Color(0.35,0.8,1.0)
 var target:Vector3 = player.global_position + Vector3(1.7, 1.1, 1.7)
 var distance:float = realm_follower.global_position.distance_to(target)
 if distance>FOLLOW_TELEPORT_DISTANCE:
  realm_follower.global_position = target
 else:
  # Catch up with a moving player instead of falling permanently behind.
  # Keep normal following speed close to the player and cap catch-up velocity.
  var catchup:float = maxf(0.0,distance-FOLLOW_CATCHUP_START)
  var speed:float = minf(FOLLOW_CATCHUP_SPEED,FOLLOW_SPEED+catchup)
  realm_follower.global_position = realm_follower.global_position.move_toward(target,speed*maxf(0.0,delta))
