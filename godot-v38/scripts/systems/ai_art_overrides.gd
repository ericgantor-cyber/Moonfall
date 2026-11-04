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

static func attach_approved(body:CharacterBody3D,resource_path:String)->bool:
 if not is_instance_valid(body) or resource_path.is_empty():return false
 if not (resource_path.begins_with(ANIMAL_FOLDER) or resource_path.begins_with(GUARDIAN_FOLDER)):return false
 if not resource_path.ends_with(".png") or not ResourceLoader.exists(resource_path):return false
 var image:Resource=load(resource_path)
 if not image is Texture2D:return false
 var existing:Sprite3D=body.get_node_or_null("MoonfallApprovedAIArt") as Sprite3D
 if is_instance_valid(existing):
  existing.texture=image as Texture2D
  return true
 var sprite:=Sprite3D.new()
 sprite.name="MoonfallApprovedAIArt"
 sprite.texture=image as Texture2D
 sprite.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 sprite.pixel_size=0.0024
 sprite.position=Vector3(0,0.63,0)
 sprite.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 sprite.transparent=true
 body.add_child(sprite)
 return true
