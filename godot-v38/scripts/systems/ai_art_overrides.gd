extends RefCounted
## Optional approved AI art overlays. No image API use in the running game.
## Missing assets always preserve existing procedural creature visuals.
const ANIMAL_FOLDER:String="res://assets/ai_generated/animals/"
const GUARDIAN_FOLDER:String="res://assets/ai_generated/guardians/"

static func slugify(name:String)->String:
 var result:String=""
 for i in range(name.length()):
  var c:String=name[i].to_lower()
  if c>="a" and c<="z" or c>="0" and c<="9":
   result+=c
  elif c=="-" or c=="_":
   result+=c
  elif result!="" and not result.ends_with("_"):
   result+="_"
 return result.strip_edges().trim_suffix("_")

static func animal_path(species_name:String)->String:
 return ANIMAL_FOLDER+"animal_"+slugify(species_name)+".png"

static func guardian_path(profile_id:int)->String:
 if profile_id<0 or profile_id>=55:return ""
 return GUARDIAN_FOLDER+"guardian_%02d.png"%profile_id

# Only visuals are hidden. Hitboxes, AI bodies, lights, and labels remain.
static func _show_native_visuals(body:CharacterBody3D,show:bool)->void:
 for child in body.get_children():
  if child is MeshInstance3D:child.visible=show
 var generic_silhouette:Node3D=body.get_node_or_null("AmericasWildlifeSilhouette") as Node3D
 if is_instance_valid(generic_silhouette):generic_silhouette.visible=show

static func clear_approved(body:CharacterBody3D)->void:
 if not is_instance_valid(body):return
 var existing:Sprite3D=body.get_node_or_null("MoonfallApprovedAIArt") as Sprite3D
 if is_instance_valid(existing):existing.queue_free()
 _show_native_visuals(body,true)

static func attach_approved(body:CharacterBody3D,resource_path:String)->bool:
 if not is_instance_valid(body):return false
 if not (resource_path.begins_with(ANIMAL_FOLDER) or resource_path.begins_with(GUARDIAN_FOLDER)):
  clear_approved(body)
  return false
 if not resource_path.ends_with(".png") or not ResourceLoader.exists(resource_path):
  clear_approved(body)
  return false
 var image:Resource=load(resource_path)
 if not image is Texture2D:
  clear_approved(body)
  return false
 var existing:Sprite3D=body.get_node_or_null("MoonfallApprovedAIArt") as Sprite3D
 var sprite:Sprite3D=existing
 if not is_instance_valid(sprite):
  sprite=Sprite3D.new()
  sprite.name="MoonfallApprovedAIArt"
  sprite.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  sprite.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  sprite.transparent=true
  body.add_child(sprite)
 var texture:Texture2D=image as Texture2D
 var guardian:bool=resource_path.begins_with(GUARDIAN_FOLDER)
 sprite.texture=texture
 # Keep original world scale when the concept is a large square PNG.
 # A guardian is intentionally larger; wildlife remains near original height.
 sprite.pixel_size=(3.0 if guardian else 1.9)/float(maxi(texture.get_width(),texture.get_height()))
 sprite.position=Vector3(0,0.8 if guardian else 0.33,0)
 _show_native_visuals(body,false)
 return true
