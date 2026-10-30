extends "res://scripts/native_v17.gd"
## v18: invisible logical terrain grid. No GridMesh, decals, lines, materials,
## tile nodes, or collision bodies are created for the grid.
## Godot's existing StaticBody3D colliders stay authoritative for physical hits.
## The grid is a compact spatial index for region lookups and AI obstacle steering.
const GRID_STEP := 3.0
const GRID_ORIGIN := Vector2(-90.0, -60.0)
const GRID_COLUMNS := 60
const GRID_ROWS := 40
const GRID_TILES := GRID_COLUMNS * GRID_ROWS
const GRID_AVOID_LOOKAHEAD := 1.8

var grid_regions := PackedByteArray()
var grid_blocked := PackedByteArray()
var grid_blocked_count := 0
var grid_avoided_steps := 0
var grid_queries := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall Native v18: seamless invisible terrain grid + release checks.",4.2)

func _build_scene() -> void:
 super._build_scene()
 _build_invisible_grid()

func _build_invisible_grid() -> void:
 grid_blocked_count=0
 grid_regions.resize(GRID_TILES)
 grid_blocked.resize(GRID_TILES)
 grid_blocked.fill(0)
 # Top row is the three northern regions, bottom row the southern regions.
 # These are spatial logical tiles, not rendered terrain replacements.
 for row in range(GRID_ROWS):
  for col in range(GRID_COLUMNS):
   var ix:=row * GRID_COLUMNS + col
   grid_regions[ix] = int(col / 20) + (3 if row >= 20 else 0)
 for body in world.find_children("*", "StaticBody3D", true, false):
  if not (body is StaticBody3D):continue
  for child in body.get_children():
   if not (child is CollisionShape3D):continue
   var collision:CollisionShape3D=child
   if collision.disabled or collision.shape==null:continue
   var half_extents:=Vector2.ZERO
   if collision.shape is BoxShape3D:
    var box:BoxShape3D=collision.shape
    # The continuous floor is walkable, never a giant blocked cell region.
    if box.size.x > 12.0 or box.size.z > 12.0:continue
    half_extents = Vector2(box.size.x*0.5,box.size.z*0.5)
   elif collision.shape is CylinderShape3D:
    var cylinder:CylinderShape3D=collision.shape
    half_extents = Vector2.ONE * cylinder.radius
   elif collision.shape is SphereShape3D:
    var sphere:SphereShape3D=collision.shape
    half_extents = Vector2.ONE * sphere.radius
   else:continue
   var world_scale:Vector3=collision.global_transform.basis.get_scale().abs()
   half_extents*=Vector2(world_scale.x,world_scale.z)
   _grid_mark_footprint(collision.global_position,half_extents)
 for blocked in grid_blocked:
  if blocked == 1:grid_blocked_count+=1

func _grid_mark_footprint(at:Vector3, half_extent:Vector2) -> void:
 # Mark only tile centers within a physical footprint and a small agent margin.
 # Conservative: Godot collision still decides exact clipping.
 var margin:=0.45
 var from_x:=maxi(0, int(ceil((at.x - half_extent.x - margin - GRID_ORIGIN.x)/GRID_STEP - 0.5)))
 var to_x:=mini(GRID_COLUMNS-1,int(floor((at.x + half_extent.x + margin - GRID_ORIGIN.x)/GRID_STEP - 0.5)))
 var from_z:=maxi(0, int(ceil((at.z - half_extent.y - margin - GRID_ORIGIN.y)/GRID_STEP - 0.5)))
 var to_z:=mini(GRID_ROWS-1,int(floor((at.z + half_extent.y + margin - GRID_ORIGIN.y)/GRID_STEP - 0.5)))
 for row in range(from_z,to_z+1):
  for col in range(from_x,to_x+1):
   grid_blocked[row*GRID_COLUMNS+col]=1

func _grid_index(at:Vector3) -> int:
 var col:=int(floor((at.x-GRID_ORIGIN.x)/GRID_STEP))
 var row:=int(floor((at.z-GRID_ORIGIN.y)/GRID_STEP))
 if col<0 or col>=GRID_COLUMNS or row<0 or row>=GRID_ROWS:return -1
 return row*GRID_COLUMNS+col

func _grid_is_open(at:Vector3) -> bool:
 var index:=_grid_index(at)
 return index>=0 and index<grid_blocked.size() and grid_blocked[index]==0

func _grid_region_at(at:Vector3) -> int:
 var index:=_grid_index(at)
 return int(grid_regions[index]) if index>=0 and index<grid_regions.size() else -1

func _region_here() -> int:
 if not is_instance_valid(player):return 0
 var zone:=_grid_region_at(player.global_position)
 return zone if zone>=0 else super._region_here()

func _terrain_grid_steer(body:CharacterBody3D,desired:Vector3) -> Vector3:
 # A cheap predictive nudge for existing beasts when approaching solid props.
 # Never position-snap the creature: physics handles actual collision response.
 if desired.length_squared()<0.01 or not is_instance_valid(body):return desired
 grid_queries+=1
 var position:=body.global_position
 if not _grid_is_open(position):return desired # Don't trap a body already by a prop.
 var ahead:=position+desired.normalized()*GRID_AVOID_LOOKAHEAD
 if _grid_is_open(ahead):return desired
 var left:=desired.rotated(Vector3.UP,PI/3.0)
 var right:=desired.rotated(Vector3.UP,-PI/3.0)
 var left_open:=_grid_is_open(position+left.normalized()*GRID_AVOID_LOOKAHEAD)
 var right_open:=_grid_is_open(position+right.normalized()*GRID_AVOID_LOOKAHEAD)
 if left_open or right_open:
  grid_avoided_steps+=1
  return left if left_open and not right_open else right if right_open and not left_open else (left if body.get_instance_id()%2==0 else right)
 # At blocked corners, probe a wider turn before relying on physics sliding.
 # Keep all probes bounded and deterministic, with no new nodes or pathfinding allocations.
 var wide_left:=desired.rotated(Vector3.UP,2.0*PI/3.0)
 var wide_right:=desired.rotated(Vector3.UP,-2.0*PI/3.0)
 var wide_left_open:=_grid_is_open(position+wide_left.normalized()*GRID_AVOID_LOOKAHEAD)
 var wide_right_open:=_grid_is_open(position+wide_right.normalized()*GRID_AVOID_LOOKAHEAD)
 if wide_left_open or wide_right_open:
  grid_avoided_steps+=1
  return wide_left if wide_left_open and not wide_right_open else wide_right if wide_right_open and not wide_left_open else (wide_left if body.get_instance_id()%2==0 else wide_right)
 # If every probe is blocked, let Godot's physical sliding resolve it.
 return desired
