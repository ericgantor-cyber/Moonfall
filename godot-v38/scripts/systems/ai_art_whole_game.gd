extends RefCounted
## Optional, local approved art for the whole native game.
## No HTTP/API, no custom collisions, no restyling the original two heroes.
const REGION_COUNT:int=6
const CHAMBER_COUNT:int=14
const GLYPH_KEYS:Array[String]=[
 "rift_warning","moon_surge","treasure_cache","treasure_bearer",
 "boss_phase","forge","rare_loot","dungeon_seal","orb_hud"
]
const SPELL_KEYS:Array[String]=[
 "nova","dash","fire","moonbound","emberborn","venomheart",
 "eclipsed","dodge","parry","block"
]
const GEAR_KEYS:Array[String]=[
 "charm","band","cowl","boots","mantle","crest","relic",
 "weapon","potion","herb","ore","food"
]
const SCENERY_KEYS:Array[String]=[
 "shrine","moon_tree","stone_pillar","forge_anvil","marsh_reeds",
 "ruin_gate","citadel_banner","campfire","crystal","library_books"
]
static var _textures:Dictionary={}

static func image_path(category:String,key:String)->String:
 match category:
  "regions":
   if not key.is_valid_int():return ""
   var id:int=int(key)
   if id<0 or id>=REGION_COUNT or key!="%02d"%id:return ""
   return "res://assets/ai_generated/regions/region_%02d.png"%id
  "chambers":
   if not key.is_valid_int():return ""
   var stage:int=int(key)
   if stage<1 or stage>CHAMBER_COUNT or key!="%02d"%stage:return ""
   return "res://assets/ai_generated/chambers/chamber_%02d.png"%stage
  "glyphs":
   if GLYPH_KEYS.has(key):return "res://assets/ai_generated/glyphs/glyph_"+key+".png"
  "spells":
   if SPELL_KEYS.has(key):return "res://assets/ai_generated/spells/spell_"+key+".png"
  "gear":
   if GEAR_KEYS.has(key):return "res://assets/ai_generated/gear/gear_"+key+".png"
  "scenery":
   if SCENERY_KEYS.has(key):return "res://assets/ai_generated/scenery/scenery_"+key+".png"
 return ""

static func optional_texture(category:String,key:String)->Texture2D:
 var path:String=image_path(category,key)
 if path.is_empty():return null
 if _textures.has(path):return _textures[path] as Texture2D
 if not ResourceLoader.exists(path):return null
 var resource:Resource=load(path)
 if not resource is Texture2D:return null
 var texture:Texture2D=resource as Texture2D
 _textures[path]=texture
 return texture

static func override_sprite(sprite:Sprite3D,category:String,key:String)->bool:
 if not is_instance_valid(sprite):return false
 var texture:Texture2D=optional_texture(category,key)
 if texture==null:return false
 sprite.texture=texture
 return true

static func override_rect(rect:TextureRect,category:String,key:String)->bool:
 if not is_instance_valid(rect):return false
 var texture:Texture2D=optional_texture(category,key)
 if texture==null:return false
 rect.texture=texture
 return true

static func add_billboard(parent:Node3D,category:String,key:String,local_at:Vector3,height:float)->Sprite3D:
 if not is_instance_valid(parent) or height<=0.0:return null
 var texture:Texture2D=optional_texture(category,key)
 if texture==null:return null
 var sprite:=Sprite3D.new()
 sprite.name="ApprovedGameArt_"+category+"_"+key
 sprite.texture=texture
 sprite.pixel_size=height/float(maxi(texture.get_width(),texture.get_height()))
 sprite.position=local_at
 sprite.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 sprite.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 sprite.transparent=true
 sprite.shaded=false
 sprite.no_depth_test=false
 parent.add_child(sprite)
 return sprite
