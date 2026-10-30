extends "res://scripts/native_v31_3_art.gd"
## v31.4 Living Bestiary: new illustrated creature/boss portraits and region landmark
## art, near-only animation LOD, impact reactions, limited pack awareness.
## No new actors/collisions, no save schema or inherited art changes.
const V314_ART_PATH:String="res://assets/v31_4_bestiary/"
const V314_SPECIES:Array[String]=["wolf","stag","boar","fox","serpent","panther","elk","owl"]
const V314_TICK:float=0.14
var v314_textures:Dictionary={}
var v314_portraits:Array[Dictionary]=[]
var v314_region_signs:Array[Sprite3D]=[]
var v314_clock:float=0.0
var v314_animated_count:int=0
var v314_culled_count:int=0
var v314_shared_alerts:int=0
var v314_reaction_count:int=0
var v314_visual_usec:int=0
var v314_visual_peak_usec:int=0

func _release_label()->String:return "v31.4"

func _v314_texture(key:String)->Texture2D:
 if not v314_textures.has(key):
  v314_textures[key]=load(V314_ART_PATH+key+".png")
 return v314_textures[key]

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var previous:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.size()<=previous:return
 var creature:Dictionary=beasts[-1]
 var host:CharacterBody3D=creature["body"]
 var picture_key:String="boss_"+str(posmod(zone,4)) if boss else kind
 if kind not in V314_SPECIES and not boss:picture_key="wolf"
 var sprite:=_v313_art_sprite(_v314_texture(picture_key),0.0027 if not boss else 0.0037)
 sprite.name="V314_BestiaryPortrait"
 sprite.position=Vector3(0.0,1.95 if not boss else 3.95,0.0)
 sprite.visible=false
 host.add_child(sprite)
 v314_portraits.append({"node":sprite,"creature":creature,"zone":zone,"boss":boss,"base_y":sprite.position.y,"hit_until":0.0,"phase":1,"picture_key":picture_key})

func _build_scene()->void:
 super._build_scene()
 for zone in range(mini(6,region_shrines.size())):
  var mark:=_v313_art_sprite(_v314_texture("region_"+str(zone)),0.0028)
  mark.name="V314_RegionalBestiaryBanner_"+str(zone)
  mark.position=Vector3(2.4,3.1,0.0)
  mark.visible=false
  region_shrines[zone].add_child(mark)
  v314_region_signs.append(mark)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var hp_before:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if hp_before<=float(target.get("hp",0.0)):return
 var hit_body:Node3D=target.get("body")
 for entry in v314_portraits:
  if (entry["creature"] as Dictionary).get("body")==hit_body:
   entry["hit_until"]=map_time+0.30
   v314_reaction_count+=1
   break
 # Nearby hostile packs recognize an ally being struck; bounded and cool-down-free,
 # using the existing perception timer, not new pathfinding or extra agents.
 if not bool(target.get("hostile",false)):return
 var count:int=0
 for neighbor in zone_beast_lists[int(target.get("zone",0))]:
  if neighbor==target or not bool(neighbor.get("alive",false)) or not bool(neighbor.get("hostile",false)):continue
  var other:CharacterBody3D=neighbor.get("body")
  if not is_instance_valid(other):continue
  if hit_body.global_position.distance_squared_to(other.global_position)>64.0:continue
  neighbor["alert_until"]=maxf(float(neighbor.get("alert_until",0.0)),map_time+2.4)
  v314_shared_alerts+=1
  count+=1
  if count>=6:break

func _v314_update_visuals()->void:
 if not is_instance_valid(player):return
 var started:int=Time.get_ticks_usec()
 v314_animated_count=0
 v314_culled_count=0
 var near:Vector3=player.global_position
 var distance_sq:float=196.0 if graphics_quality=="Smooth" else 784.0
 for index in range(v314_portraits.size()):
  var entry:Dictionary=v314_portraits[index]
  var sprite:Sprite3D=entry["node"]
  var creature:Dictionary=entry["creature"]
  var body:CharacterBody3D=creature["body"]
  var show_it:bool=hero_playing and bool(creature.get("alive",false)) and int(entry["zone"])==region_index and is_instance_valid(body) and body.visible and body.global_position.distance_squared_to(near)<distance_sq*(1.65 if bool(entry["boss"]) else 1.0)
  sprite.visible=show_it
  if not show_it:
   v314_culled_count+=1
   continue
  v314_animated_count+=1
  var hit:bool=map_time<float(entry["hit_until"])
  sprite.modulate=Color(1,0.50,0.55) if hit else Color.WHITE
  sprite.position.y=float(entry["base_y"])+0.07*sin(map_time*2.1+float(index)*0.31)
  if bool(entry["boss"]):
   var phase:int=int(creature.get("boss_phase",1))
   if phase!=int(entry["phase"]):
    entry["phase"]=phase
    sprite.texture=_v314_texture("boss_"+str(posmod(int(entry["zone"])+phase-1,4)))
   sprite.scale=Vector3.ONE*(1.0+0.08*sin(map_time*3.1))
 for j in range(v314_region_signs.size()):
  var banner:Sprite3D=v314_region_signs[j]
  banner.visible=hero_playing and region_index==j and banner.global_position.distance_squared_to(near)<900.0 and graphics_quality!="Smooth"
 v314_visual_usec=Time.get_ticks_usec()-started
 v314_visual_peak_usec=maxi(v314_visual_peak_usec,v314_visual_usec)

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing:return
 v314_clock+=minf(maxf(delta,0.0),0.2)
 if v314_clock>=V314_TICK:
  v314_clock=fmod(v314_clock,V314_TICK)
  _v314_update_visuals()

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v314_clock=0.0
 for entry in v314_portraits:
  (entry["node"] as Sprite3D).visible=false
  entry["hit_until"]=0.0
  entry["phase"]=1

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v31.4"
 report["bestiary_portraits_created"]=v314_portraits.size()
 report["bestiary_portraits_animated"]=v314_animated_count
 report["bestiary_portraits_culled"]=v314_culled_count
 report["bestiary_visual_update_usec"]=v314_visual_usec
 report["bestiary_visual_peak_usec"]=v314_visual_peak_usec
 report["bestiary_pack_alerts"]=v314_shared_alerts
 report["bestiary_hit_reactions"]=v314_reaction_count
 return report
