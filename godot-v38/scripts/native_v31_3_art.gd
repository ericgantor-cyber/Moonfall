extends "res://scripts/native_v31_3.gd"
## Moonfall v31.3 Art Edition: game-integrated original glyphs for gameplay cues.
## No modified hero artwork, collision, damage, saved data or encounter timing.
const ART_RIFT:Texture2D=preload("res://assets/v31_3_art/rift_warning.png")
const ART_SURGE:Texture2D=preload("res://assets/v31_3_art/moon_surge.png")
const ART_CACHE:Texture2D=preload("res://assets/v31_3_art/treasure_cache.png")
const ART_CARRIER:Texture2D=preload("res://assets/v31_3_art/treasure_bearer.png")
const ART_BOSS:Texture2D=preload("res://assets/v31_3_art/boss_phase.png")
const ART_FORGE:Texture2D=preload("res://assets/v31_3_art/forge.png")
const ART_LOOT:Texture2D=preload("res://assets/v31_3_art/rare_loot.png")
const ART_DUNGEON:Texture2D=preload("res://assets/v31_3_art/dungeon_seal.png")
var v313_art_chests:Array[Sprite3D]=[]
var v313_art_traps:Array[Sprite3D]=[]
var v313_art_surge:TextureRect
var v313_art_boss_count:int=0
var v313_art_carrier_count:int=0
var v313_art_forge:Sprite3D
var v313_art_dungeon:Array[Sprite3D]=[]
var v313_art_rare_loot:int=0

func _v313_art_sprite(texture:Texture2D,scale_pixels:float=0.004)->Sprite3D:
 var sprite:=Sprite3D.new()
 sprite.texture=texture
 sprite.pixel_size=scale_pixels
 sprite.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 sprite.shaded=false
 sprite.transparent=true
 sprite.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 sprite.no_depth_test=false
 return sprite

func _build_scene()->void:
 super._build_scene()
 _v313_art_decorate_chests()
 _v313_art_decorate_traps()
 _v313_art_decorate_landmarks()

func _v313_art_decorate_chests()->void:
 # Only six markers, reused for the life of the world.
 for chest in treasure_chests:
  var node:=_v313_art_sprite(ART_CACHE,0.0028)
  node.name="Art_RareCacheSigil"
  node.position=Vector3(0.0,2.50,0.0)
  node.visible=not (int(chest["zone"]) in opened_treasure_chests)
  (chest["node"] as Node3D).add_child(node)
  v313_art_chests.append(node)

func _v313_art_decorate_traps()->void:
 for entry in v313_traps:
  var rune:=entry["node"] as MeshInstance3D
  var art:=_v313_art_sprite(ART_RIFT,0.005)
  art.name="Art_RiftWarningGlyph"
  art.billboard=BaseMaterial3D.BILLBOARD_DISABLED
  art.rotation.x=-PI/2.0
  art.position=Vector3(0.0,0.04,0.0)
  rune.add_child(art)
  v313_art_traps.append(art)

func _v313_art_decorate_landmarks()->void:
 # The outpost shrine already acts as the merchant trigger; this is only a sign.
 if region_shrines.size()>0:
  var sign:=_v313_art_sprite(ART_FORGE,0.003)
  sign.name="Art_HearthstoneForgeSign"
  sign.position=Vector3(0.0,4.2,0.0)
  region_shrines[0].add_child(sign)
  v313_art_forge=sign
 for index in range(rifts.size()):
  var sigil:=_v313_art_sprite(ART_DUNGEON,0.003)
  sigil.name="Art_DungeonSeal_"+str(index)
  var place:Vector3=rifts[index]["anchor"]
  sigil.position=place+Vector3(0.0,3.5,0.0)
  world.add_child(sigil)
  v313_art_dungeon.append(sigil)

func _ready()->void:
 super._ready()
 _v313_art_build_surge_hud()

func _v313_art_build_surge_hud()->void:
 var canvas:=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if canvas==null or canvas.get_child_count()==0:return
 var screen:=canvas.get_child(0) as Control
 if screen==null:return
 var emblem:=TextureRect.new()
 emblem.name="Art_MoonSurgeIndicator"
 emblem.texture=ART_SURGE
 emblem.custom_minimum_size=Vector2(52,52)
 emblem.size=Vector2(52,52)
 emblem.position=Vector2(6,147)
 emblem.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
 emblem.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
 emblem.mouse_filter=Control.MOUSE_FILTER_IGNORE
 emblem.visible=false
 screen.add_child(emblem)
 v313_art_surge=emblem

func _refresh_treasure_chests()->void:
 super._refresh_treasure_chests()
 for index in range(mini(v313_art_chests.size(),treasure_chests.size())):
  v313_art_chests[index].visible=not (int(treasure_chests[index]["zone"]) in opened_treasure_chests)

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var count_before:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.size()<=count_before:return
 var creature:Dictionary=beasts[-1]
 var container:=creature.get("body") as CharacterBody3D
 if container==null:return
 if boss:
  var marker:=_v313_art_sprite(ART_BOSS,0.003)
  marker.name="Art_BossPhaseCrest"
  marker.position=Vector3(0.0,3.20,0.0)
  container.add_child(marker)
  v313_art_boss_count+=1
 elif bool(creature.get("treasure_carrier",false)):
  var marker:=_v313_art_sprite(ART_CARRIER,0.0025)
  marker.name="Art_TreasureBearerCrest"
  marker.position=Vector3(0.0,2.75,0.0)
  container.add_child(marker)
  v313_art_carrier_count+=1

func _create_gear_drop(at:Vector3,kind:String,variant:String,zone:int,existing:Dictionary={})->void:
 var before:int=gear_drops.size()
 super._create_gear_drop(at,kind,variant,zone,existing)
 if gear_drops.size()<=before:return
 var drop:Dictionary=gear_drops[-1]
 var rarity:String=str((drop["data"] as Dictionary).get("rarity",""))
 if rarity not in ["Legendary","Mythic"]:return
 var marker:=_v313_art_sprite(ART_LOOT,0.0025)
 marker.name="Art_RareLootGlyph"
 marker.position=Vector3(0.0,2.30,0.0)
 (drop["node"] as Node3D).add_child(marker)
 v313_art_rare_loot+=1

func _v313_tick_surge()->void:
 super._v313_tick_surge()
 if is_instance_valid(v313_art_surge):
  v313_art_surge.visible=hero_playing and v313_surge_zone>=0 and not menu_open

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 if is_instance_valid(v313_art_surge):v313_art_surge.visible=false

func _perf_report()->Dictionary:
 var result:Dictionary=super._perf_report()
 result["art_edition"]="v31.3"
 result["art_chest_icons"]=v313_art_chests.size()
 result["art_rift_glyphs"]=v313_art_traps.size()
 result["art_boss_icons_created"]=v313_art_boss_count
 result["art_treasure_carrier_icons_created"]=v313_art_carrier_count
 result["art_dungeon_seals"]=v313_art_dungeon.size()
 result["art_rare_loot_emblems_created"]=v313_art_rare_loot
 return result
