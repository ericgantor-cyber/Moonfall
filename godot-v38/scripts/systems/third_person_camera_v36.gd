extends RefCounted
## Deterministic third-person camera geometry for Moonfall's real 3D world.
const MIN_PITCH:float=0.21
const MAX_PITCH:float=1.13
const MIN_DISTANCE:float=6.5
const MAX_DISTANCE:float=22.0
const DEFAULT_DISTANCE:float=12.0
const DEFAULT_PITCH:float=0.56
const DEFAULT_YAW:float=0.78539816339

static func camera_offset(yaw:float,pitch:float,distance:float)->Vector3:
 var flat:float=cos(clampf(pitch,MIN_PITCH,MAX_PITCH))*clampf(distance,MIN_DISTANCE,MAX_DISTANCE)
 return Vector3(sin(yaw)*flat,sin(clampf(pitch,MIN_PITCH,MAX_PITCH))*clampf(distance,MIN_DISTANCE,MAX_DISTANCE),cos(yaw)*flat)

static func move_relative(raw:Vector3,yaw:float)->Vector3:
 # W/S align to the camera's projected forward/back, A/D to its right.
 var toward_camera:=Vector3(sin(yaw),0.0,cos(yaw))
 var right:=Vector3(cos(yaw),0.0,-sin(yaw))
 return (right*raw.x+toward_camera*raw.z).limit_length(1.0)

static func orbit_pitch(current:float,change:float)->float:
 return clampf(current+change,MIN_PITCH,MAX_PITCH)

static func zoom_from_legacy(value:float)->float:
 # v24 passes 19 as default, +/-1.5 for one mouse wheel step.
 return clampf(DEFAULT_DISTANCE+(clampf(value,12.0,31.0)-19.0)*0.8,MIN_DISTANCE,MAX_DISTANCE)

static func safe_distance(hit_distance:float)->float:
 return maxf(2.4,hit_distance-0.55)

static func collision_constrained_position(current:Vector3,desired:Vector3,focus:Vector3,safe_radius:float,blocked:bool,blend:float)->Vector3:
 # A wall requires immediate retraction; easing from the old long distance
 # would leave the camera beyond the detected obstacle for several frames.
 # When the path is clear, preserve the usual smooth outward camera follow.
 var next_position:Vector3=current.lerp(desired,clampf(blend,0.0,1.0))
 if blocked and next_position.distance_to(focus)>safe_radius:
  return desired
 return next_position
