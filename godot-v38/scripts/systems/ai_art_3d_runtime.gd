extends RefCounted
## Approved real GLB meshes, automatically imported by Godot on startup.
## No API access, no runtime mesh generation, no changes to collision/hero art.
const GAME_ART=preload("res://scripts/systems/ai_art_whole_game.gd")
const WILDLIFE_ART=preload("res://scripts/systems/ai_art_overrides.gd")
const MODEL_ROOT:String="res://assets/ai_generated/models/"
const RIGGED_ROOT:String="res://assets/ai_generated/rigged/"
const MAX_VISIBLE_MESHES:int=160
const MAX_CACHED_MODEL_SCENES:int=12
static var _models:Dictionary={}
static var _cache_keys:Array[String]=[]

static func approved_model_path(category:String,asset_id:String)->String:
 if asset_id.length()<4 or asset_id.length()>90:return ""
 for i in range(asset_id.length()):
  var c:String=asset_id[i]
  if not ((c>="a" and c<="z") or (c>="0" and c<="9") or c=="_" or c=="-"):return ""
 match category:
  "animals":
   if not asset_id.begins_with("animal_"):return ""
  "guardians":
   if not asset_id.begins_with("guardian_"):return ""
   var suffix:String=asset_id.trim_prefix("guardian_")
   if suffix.length()!=2 or not suffix.is_valid_int() or suffix!="%02d"%int(suffix) or int(suffix)>=55:return ""
  "scenery":
   if not asset_id.begins_with("scenery_") or not GAME_ART.SCENERY_KEYS.has(asset_id.trim_prefix("scenery_")):return ""
  "gear":
   if not asset_id.begins_with("gear_") or not GAME_ART.GEAR_KEYS.has(asset_id.trim_prefix("gear_")):return ""
  "chambers":
   var suffix:String=asset_id.trim_prefix("chamber_")
   if not asset_id.begins_with("chamber_") or suffix!=("%02d"%int(suffix)) or not suffix.is_valid_int() or int(suffix)<1 or int(suffix)>14:return ""
  _:return ""
 # A separately approved animated rig takes priority. The original reviewed
 # static mesh is retained, so failed/absent rig imports can still fall back.
 if category in ["animals","guardians"]:
  var rigged_path:String=RIGGED_ROOT+category+"/"+asset_id+".glb"
  if ResourceLoader.exists(rigged_path):return rigged_path
 return MODEL_ROOT+category+"/"+asset_id+".glb"

static func _approved_scene(category:String,asset_id:String)->PackedScene:
 var path:String=approved_model_path(category,asset_id)
 if path.is_empty():return null
 if _models.has(path):
  _cache_keys.erase(path)
  _cache_keys.append(path)
  return _models[path] as PackedScene
 if not ResourceLoader.exists(path):return null
 var asset:Resource=load(path)
 if not asset is PackedScene:return null
 _models[path]=asset
 _cache_keys.append(path)
 while _cache_keys.size()>MAX_CACHED_MODEL_SCENES:
  var old_key:String=_cache_keys.pop_front()
  _models.erase(old_key)
 return asset as PackedScene

static func _mesh_height_and_count(root:Node3D)->Vector2:
 # Only render geometry determines scale. Ignore physics and empty scene nodes.
 var lo:float=INF
 var hi:float=-INF
 var visible_meshes:int=0
 var stack:Array[Node]=[root]
 while not stack.is_empty():
  var node:Node=stack.pop_back()
  if node is MeshInstance3D:
   var mesh:MeshInstance3D=node as MeshInstance3D
   if mesh.mesh!=null:
    visible_meshes+=1
    if visible_meshes>MAX_VISIBLE_MESHES:return Vector2(0,visible_meshes)
    var aabb:AABB=mesh.get_aabb()
    for x in [0,1]:
     for y in [0,1]:
      for z in [0,1]:
       var corner:Vector3=aabb.position+Vector3(aabb.size.x*x,aabb.size.y*y,aabb.size.z*z)
       var point:Vector3=root.to_local(mesh.to_global(corner))
       lo=minf(lo,point.y)
       hi=maxf(hi,point.y)
  if node is CollisionShape3D:
   # Imported art may carry authoring collisions; native gameplay hitboxes win.
   (node as CollisionShape3D).set_deferred("disabled",true)
  for child in node.get_children():stack.append(child)
 return Vector2(maxf(0.0,hi-lo) if is_finite(lo) and is_finite(hi) else 0.0,visible_meshes)

static func add_approved_model(parent:Node3D,category:String,asset_id:String,at:Vector3,height:float,replace_native:bool=false)->Node3D:
 if not is_instance_valid(parent) or height<=0.0:return null
 var packed:PackedScene=_approved_scene(category,asset_id)
 if packed==null:return null
 var instance:Node=packed.instantiate()
 if not instance is Node3D:
  instance.free()
  return null
 var root:Node3D=instance as Node3D
 root.name="MoonfallApproved3D_"+asset_id
 parent.add_child(root)
 root.position=at
 var info:Vector2=_mesh_height_and_count(root)
 if info.y<1.0 or info.y>float(MAX_VISIBLE_MESHES) or info.x<0.001 or not is_finite(info.x):
  root.queue_free()
  return null
 var factor:float=height/info.x
 if not is_finite(factor) or factor>200.0 or factor<0.001:
  root.queue_free()
  return null
 root.scale=Vector3.ONE*factor
 if replace_native:
  for child in parent.get_children():
   if child==root:continue
   if child is MeshInstance3D:(child as MeshInstance3D).visible=false
  var silhouette:Node3D=parent.get_node_or_null("AmericasWildlifeSilhouette") as Node3D
  if is_instance_valid(silhouette):silhouette.visible=false
  var sprite:Sprite3D=parent.get_node_or_null("MoonfallApprovedAIArt") as Sprite3D
  if is_instance_valid(sprite):sprite.visible=false
 return root
