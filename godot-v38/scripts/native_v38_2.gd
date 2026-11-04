extends "res://scripts/native_v38_1.gd"
## Coordinate-accurate atlas overlay; existing connected world geometry unchanged.
const NAVIGATION_MAP=preload("res://scripts/systems/navigation_map.gd")
const AMERICAS_WILDLIFE=preload("res://scripts/systems/americas_wildlife_catalog.gd")
const APPROVED_AI_ART=preload("res://scripts/systems/ai_art_overrides.gd")
const AI_WHOLE_GAME_ART=preload("res://scripts/systems/ai_art_whole_game.gd")
const AI_APPROVED_3D=preload("res://scripts/systems/ai_art_3d_runtime.gd")
const AI_ART_AUTO_QUALITY=preload("res://scripts/systems/ai_art_auto_quality.gd")
const AI_ART_ANIMATION=preload("res://scripts/systems/ai_art_animation_runtime.gd")
const AMERICAS_VISUALS=preload("res://scripts/systems/americas_wildlife_visuals.gd")
var atlas_panel:PanelContainer
var atlas_chart:Control
var atlas_readout:Label
var atlas_waypoint:Vector3=Vector3.ZERO
var atlas_has_waypoint:bool=false
var atlas_elapsed:float=0.0
var atlas_chamber_scan_elapsed:float=0.0
var atlas_open_button:Button
var atlas_zoom_radius:float=120.0
var atlas_quest_positions:Array[Vector3]=[]
var atlas_legend:Label
var atlas_discovered:Array[Vector3]=[]
var atlas_chambers_seen:Array[int]=[]
var atlas_dungeon_paths:Dictionary={}
var atlas_dungeon_site:int=-1
var atlas_regional_dungeon:int=-1
var atlas_regional_seen:Array[int]=[]
var atlas_regional_relics_claimed:Array[int]=[]
var atlas_regional_guardians_cleared:Array[int]=[]
var atlas_regional_guardian_actors:Dictionary={}
var atlas_regional_guardian_rolls:Array[int]=[]
const DUNGEON_GUARDIAN_TRIGGER_RADIUS_SQ:float=16.0*16.0
var atlas_dungeon_zoom_radius:float=35.0
var atlas_fast_travel_select:OptionButton
var atlas_fast_travel_button:Button
var atlas_fast_travel_status:Label
var atlas_fast_travel_unlocked_cache:Array[int]=[]
var american_wildlife_rolls:Array[int]=[]
var american_wildlife_eligible:int=0
var ai_art_game_decorations:int=0
var ai_art_game_glyphs:int=0
const AI_ART_NEAR_ANIMAL_LIMIT:int=3
const AI_ART_NEAR_CHAMBER_LIMIT:int=1
const AI_ART_ANIMAL_DISTANCE_SQ:float=22.0*22.0
const AI_ART_CHAMBER_DISTANCE_SQ:float=30.0*30.0
var ai_art_live_animal_models:Array[Dictionary]=[]
var ai_art_chamber_models:Array[Dictionary]=[]
var ai_art_quality_tier:int=AI_ART_AUTO_QUALITY.HIGH
var ai_art_frame_average_ms:float=16.7
var ai_art_slow_seconds:float=0.0
var ai_art_fast_seconds:float=0.0
var ai_art_next_model_load_ms:int=0
var ai_art_model_load_attempts:int=0
var ai_art_anim_clock:float=0.0
var ai_art_anim_tick_count:int=0
var american_wildlife_visual_ticks:int=0
var american_wildlife_visual_clock:float=0.0
var atlas_heading:Label
const ATLAS_DISCOVERY_SPACING:float=55.0
const ATLAS_MAX_DISCOVERED:int=2048
func _atlas_room_id(site_id:int,stage:int)->int:
 if stage<=3:return site_id*3+stage-1
 return rifts.size()*3+site_id*11+stage-4

func _atlas_all_site_chambers()->Array[Dictionary]:
 var rooms:Array[Dictionary]=[]
 rooms.append_array(v321_chambers)
 rooms.append_array(v321_annexes)
 return rooms

# Separate room IDs and trail keys preserve all historical rift checkpoint data.
func _atlas_regional_room_id(zone:int,dungeon_id:int,stage:int)->int:
 return (zone*V321_RULES.REGIONAL_DUNGEON_COUNT+dungeon_id)*V321_RULES.REGIONAL_CHAMBERS_PER_DUNGEON+stage-1

func _atlas_regional_trail_key(regional_id:int)->int:
 return rifts.size()+regional_id

func _atlas_regional_rooms()->Array[Dictionary]:
 var result:Array[Dictionary]=[]
 if atlas_regional_dungeon<0:return result
 var zone:int=int(atlas_regional_dungeon / V321_RULES.REGIONAL_DUNGEON_COUNT)
 var dungeon_id:int=atlas_regional_dungeon % V321_RULES.REGIONAL_DUNGEON_COUNT
 for entry in v321_region_rooms:
  if int(entry.get("zone",-1))==zone and int(entry.get("dungeon",-1))==dungeon_id:result.append(entry)
 return result

# Reuse native wildlife actors. Only original non-boss, non-rift wildlife can
# receive a geographical identity: no extra AI, physics bodies or spawn waves.
func _build_scene()->void:
 super._build_scene()
 american_wildlife_eligible=0
 for animal in beasts:
  if bool(animal.get("boss",false)) or int(animal.get("dungeon_site",-1))>=0:continue
  animal["americas_wildlife_eligible"]=true
  american_wildlife_eligible+=1

func _americas_roll_profiles()->void:
 american_wildlife_rolls.clear()
 for zone in range(REGIONS.size()):american_wildlife_rolls.append(rng.randi_range(0,1000000))

func _americas_apply_wildlife_profiles()->void:
 if american_wildlife_rolls.size()!=REGIONS.size():return
 for actor in ai_art_live_animal_models:_ai_art_release_animal_model(actor)
 ai_art_live_animal_models.clear()
 for zone in range(REGIONS.size()):
  var slot_by_kind:Dictionary={}
  for animal in zone_beast_lists[zone]:
   if not bool(animal.get("americas_wildlife_eligible",false)) or bool(animal.get("boss",false)):continue
   var archetype:String=str(animal.get("kind",""))
   var slot:int=int(slot_by_kind.get(archetype,0))
   slot_by_kind[archetype]=slot+1
   var picked:Dictionary=AMERICAS_WILDLIFE.select_for_archetype(zone,archetype,american_wildlife_rolls[zone]+slot)
   if picked.is_empty():continue
   animal["americas_species"]=str(picked["name"])
   animal["americas_taxon"]=str(picked["taxon"])
   var native_body:CharacterBody3D=animal.get("body") as CharacterBody3D
   if is_instance_valid(native_body):
    animal["americas_visual_root"]=AMERICAS_VISUALS.attach(native_body,str(picked["taxon"]),str(picked["name"]))
    # Start with the approved 2D sprite or native silhouette.
    # Only creatures near the player receive their actual 4K GLB later.
    animal["americas_3d_asset_id"]="animal_"+APPROVED_AI_ART.slugify(str(picked["name"]))
    APPROVED_AI_ART.attach_approved(native_body,APPROVED_AI_ART.animal_path(str(picked["name"])))
   var nameplate:Label3D=animal.get("nameplate") as Label3D
   if is_instance_valid(nameplate) and bool(animal.get("alive",false)) and not bool(animal.get("tamed",false)):
    nameplate.text=str(picked["name"])
  # Original wildlife still uses its native skeleton, AI, combat and loot.

func _ai_art_release_animal_model(actor:Dictionary)->void:
 actor.erase("ai_art_animation_binding")
 var current:Node3D=actor.get("americas_3d_node") as Node3D
 if is_instance_valid(current):current.queue_free()
 actor.erase("americas_3d_node")
 var body:CharacterBody3D=actor.get("body") as CharacterBody3D
 if is_instance_valid(body):
  APPROVED_AI_ART.attach_approved(body,APPROVED_AI_ART.animal_path(str(actor.get("americas_species",""))))

# Art detail changes only after sustained poor or good frame times.
# 3s poor degrades by one tier; 16s stable recovers by one tier.
# Imported GLB animations are visual. Native AI decides all actions and hits.
# Only the bounded active creature GLBs and any loaded dungeon guardians tick.
func _ai_art_update_animations(delta:float)->void:
 ai_art_anim_clock+=clampf(delta,0.0,0.2)
 if ai_art_anim_clock<0.08:return
 ai_art_anim_clock=fmod(ai_art_anim_clock,0.08)
 if not hero_playing or death_active or menu_open or not is_instance_valid(player):return
 for animal in ai_art_live_animal_models:
  var animated:Dictionary=animal.get("ai_art_animation_binding",{})
  if animated.is_empty():continue
  AI_ART_ANIMATION.apply_state(animated,AI_ART_ANIMATION.state_for_actor(animal,map_time))
  ai_art_anim_tick_count+=1
 for guardian in atlas_regional_guardian_actors.values():
  if not bool(guardian.get("alive",false)):continue
  var animated:Dictionary=guardian.get("ai_art_animation_binding",{})
  if animated.is_empty():continue
  var guard_body:CharacterBody3D=guardian.get("body") as CharacterBody3D
  if not is_instance_valid(guard_body) or not guard_body.visible:continue
  if guard_body.global_position.distance_squared_to(player.global_position)>45.0*45.0:continue
  AI_ART_ANIMATION.apply_state(animated,AI_ART_ANIMATION.state_for_actor(guardian,map_time))
  ai_art_anim_tick_count+=1

func _ai_art_track_frame_performance(delta:float)->void:
 if not hero_playing or death_active or not is_finite(delta) or delta<=0.0:return
 var sample:float=clampf(delta*1000.0,0.0,160.0)
 ai_art_frame_average_ms=lerpf(ai_art_frame_average_ms,sample,0.045)
 var frame_seconds:float=minf(delta,0.15)
 if AI_ART_AUTO_QUALITY.should_degrade(ai_art_frame_average_ms):
  ai_art_slow_seconds+=frame_seconds
  ai_art_fast_seconds=0.0
 elif AI_ART_AUTO_QUALITY.should_upgrade(ai_art_frame_average_ms):
  ai_art_fast_seconds+=frame_seconds
  ai_art_slow_seconds=0.0
 else:
  ai_art_slow_seconds=maxf(0.0,ai_art_slow_seconds-frame_seconds)
  ai_art_fast_seconds=0.0
 var new_tier:int=AI_ART_AUTO_QUALITY.next_tier(ai_art_quality_tier,ai_art_slow_seconds,ai_art_fast_seconds)
 if new_tier!=ai_art_quality_tier:
  ai_art_quality_tier=new_tier
  ai_art_slow_seconds=0.0
  ai_art_fast_seconds=0.0

# Streaming only reserves the nearest 0,1 or 3 approved animal meshes
# (depending on adaptive frame budget), and at most one dungeon monument.
# Hysteresis prevents model flicker and one load/900ms prevents GPU spikes.
func _ai_art_refresh_nearby_models()->void:
 if not is_instance_valid(player):return
 var here:Vector3=player.global_position
 var now_ms:int=Time.get_ticks_msec()
 var limit:int=mini(AI_ART_NEAR_ANIMAL_LIMIT,AI_ART_AUTO_QUALITY.animal_limit(ai_art_quality_tier))
 var candidates:Array[Dictionary]=[]
 if limit>0:
  for animal in zone_beast_lists[region_index]:
   if not bool(animal.get("alive",false)) or not bool(animal.get("americas_wildlife_eligible",false)):continue
   var body:CharacterBody3D=animal.get("body") as CharacterBody3D
   if not is_instance_valid(body) or not body.visible:continue
   var current:Node3D=animal.get("americas_3d_node") as Node3D
   var active:bool=is_instance_valid(current)
   var distance:float=body.global_position.distance_squared_to(here)
   var max_range:float=AI_ART_AUTO_QUALITY.ANIMAL_EXIT_RADIUS_SQ if active else AI_ART_ANIMAL_DISTANCE_SQ
   if distance>max_range:continue
   var identifier:String=str(animal.get("americas_3d_asset_id",""))
   if identifier.is_empty():continue
   var path:String=AI_APPROVED_3D.approved_model_path("animals",identifier)
   if path.is_empty() or not ResourceLoader.exists(path):continue
   # Slight active-model priority prevents visible swapping at equal distances.
   var rank:float=distance-(AI_ART_AUTO_QUALITY.ANIMAL_ENTER_RADIUS_SQ if active else 0.0)
   candidates.append({"actor":animal,"rank":rank})
 candidates.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return float(a["rank"])<float(b["rank"]))
 var selected:Dictionary={}
 var loaded_this_tick:bool=false
 for i in range(mini(limit,candidates.size())):
  var actor:Dictionary=candidates[i]["actor"]
  var body:CharacterBody3D=actor.get("body") as CharacterBody3D
  if not is_instance_valid(body):continue
  var key:int=body.get_instance_id()
  selected[key]=true
  var current:Node3D=actor.get("americas_3d_node") as Node3D
  if not is_instance_valid(current) and not loaded_this_tick and now_ms>=ai_art_next_model_load_ms:
   ai_art_model_load_attempts+=1
   ai_art_next_model_load_ms=now_ms+AI_ART_AUTO_QUALITY.MIN_LOAD_INTERVAL_MS
   loaded_this_tick=true
   var identifier:String=str(actor["americas_3d_asset_id"])
   current=AI_APPROVED_3D.add_approved_model(body,"animals",identifier,Vector3.ZERO,2.0,true)
   if is_instance_valid(current):
    actor["americas_3d_node"]=current
    actor["ai_art_animation_binding"]=AI_ART_ANIMATION.bind_imported(current)
 for actor in ai_art_live_animal_models:
  var body:CharacterBody3D=actor.get("body") as CharacterBody3D
  if not is_instance_valid(body) or not selected.has(body.get_instance_id()):
   _ai_art_release_animal_model(actor)
 ai_art_live_animal_models.clear()
 for entry in candidates:
  var actor:Dictionary=entry["actor"]
  var body:CharacterBody3D=actor.get("body") as CharacterBody3D
  if is_instance_valid(body) and selected.has(body.get_instance_id()) and is_instance_valid(actor.get("americas_3d_node") as Node3D):
   ai_art_live_animal_models.append(actor)
 # The room model budget shares the same 900ms load throttle.
 var nearest:Dictionary={}
 var best:float=INF
 if AI_ART_AUTO_QUALITY.chamber_limit(ai_art_quality_tier)>0:
  for entry in ai_art_chamber_models:
   var node:Node3D=entry.get("node") as Node3D
   if not is_instance_valid(node) or not node.visible:continue
   var existing:Node3D=entry.get("model") as Node3D
   var allowed:float=AI_ART_AUTO_QUALITY.ROOM_EXIT_RADIUS_SQ if is_instance_valid(existing) else AI_ART_CHAMBER_DISTANCE_SQ
   var distance:float=node.global_position.distance_squared_to(here)
   if distance<allowed and distance<best:
    nearest=entry
    best=distance
 for entry in ai_art_chamber_models:
  var node:Node3D=entry.get("node") as Node3D
  if not is_instance_valid(node):continue
  var existing:Node3D=entry.get("model") as Node3D
  var banner:Sprite3D=entry.get("banner") as Sprite3D
  if entry==nearest and not nearest.is_empty():
   if not is_instance_valid(existing) and not loaded_this_tick and now_ms>=ai_art_next_model_load_ms:
    ai_art_model_load_attempts+=1
    ai_art_next_model_load_ms=now_ms+AI_ART_AUTO_QUALITY.MIN_LOAD_INTERVAL_MS
    loaded_this_tick=true
    var stage:int=int(entry["stage"])
    existing=AI_APPROVED_3D.add_approved_model(node,"chambers","chamber_%02d"%stage,Vector3.ZERO,2.3)
    if is_instance_valid(existing):entry["model"]=existing
   if is_instance_valid(existing) and is_instance_valid(banner):banner.visible=false
  else:
   if is_instance_valid(existing):existing.queue_free()
   entry.erase("model")
   if is_instance_valid(banner):banner.visible=true

# Animate only the visible nearby animals at a low cadence.
# No movement is applied to CharacterBody3D, so collision and native AI remain intact.
func _americas_update_wildlife_visuals(delta:float)->void:
 american_wildlife_visual_clock+=clampf(delta,0.0,0.2)
 if american_wildlife_visual_clock<0.28:return
 american_wildlife_visual_clock=fmod(american_wildlife_visual_clock,0.28)
 if not hero_playing or not is_instance_valid(player):return
 _ai_art_refresh_nearby_models()
 var now:float=float(Time.get_ticks_msec())/1000.0
 var updates:int=0
 for animal in zone_beast_lists[region_index]:
  if updates>=20:break
  if not bool(animal.get("alive",false)) or not bool(animal.get("americas_wildlife_eligible",false)):continue
  var body:CharacterBody3D=animal.get("body") as CharacterBody3D
  var visual:Node3D=animal.get("americas_visual_root") as Node3D
  if not is_instance_valid(body) or not is_instance_valid(visual) or not body.visible or not visual.visible:continue
  if body.global_position.distance_squared_to(player.global_position)>45.0*45.0:continue
  AMERICAS_VISUALS.animate(visual,str(animal.get("americas_taxon","M")),now,float(animal.get("boss_id",0))*0.47)
  updates+=1
 american_wildlife_visual_ticks+=updates

# Existing v31.3 glyphs and original HUD can display approved artwork,
# with no new widgets when an optional PNG has not been supplied.
func _ai_art_upgrade_existing_glyphs()->void:
 for icon in v313_art_chests:
  if AI_WHOLE_GAME_ART.override_sprite(icon,"glyphs","treasure_cache"):ai_art_game_glyphs+=1
 for icon in v313_art_traps:
  if AI_WHOLE_GAME_ART.override_sprite(icon,"glyphs","rift_warning"):ai_art_game_glyphs+=1
 for icon in v313_art_dungeon:
  if AI_WHOLE_GAME_ART.override_sprite(icon,"glyphs","dungeon_seal"):ai_art_game_glyphs+=1
 if AI_WHOLE_GAME_ART.override_sprite(v313_art_forge,"glyphs","forge"):ai_art_game_glyphs+=1
 if AI_WHOLE_GAME_ART.override_rect(v313_art_surge,"glyphs","moon_surge"):ai_art_game_glyphs+=1
 if AI_WHOLE_GAME_ART.override_rect(v386_orb_image,"glyphs","orb_hud"):ai_art_game_glyphs+=1

# Optional scenery attaches to existing shrine nodes, and 14 chamber art
# templates attach to the rooms themselves. Visibility inherits room culling;
# NO extra collision shapes or new actual room/chamber geometry.
func _ai_art_decorate_regions_and_chambers()->void:
 for zone in range(mini(region_shrines.size(),REGIONS.size())):
  var shrine:Node3D=region_shrines[zone] as Node3D
  if is_instance_valid(AI_WHOLE_GAME_ART.add_billboard(shrine,"regions","%02d"%zone,Vector3(0,4.5,0),2.6)):
   ai_art_game_decorations+=1
  if zone==0:
   var scene_model:Node3D=AI_APPROVED_3D.add_approved_model(shrine,"scenery","scenery_shrine",Vector3(2.2,0,0),2.8)
   if is_instance_valid(scene_model):
    ai_art_game_decorations+=1
   elif is_instance_valid(AI_WHOLE_GAME_ART.add_billboard(shrine,"scenery","shrine",Vector3(2.2,2.2,0),1.3)):
    ai_art_game_decorations+=1
 # Every room is already independently culled by the native dungeon renderer.
 # A maximum of 420 optional sprites, but zero when chamber art is absent.
 for chamber in v321_region_rooms:
  var stage:int=int(chamber.get("stage",0))
  if stage<1 or stage>14:continue
  var room_node:Node3D=chamber.get("node") as Node3D
  # 4K GLB props are loaded only when the player approaches this room.
  var banner:Sprite3D=AI_WHOLE_GAME_ART.add_billboard(room_node,"chambers","%02d"%stage,Vector3(0,1.8,0),1.4)
  if is_instance_valid(banner):ai_art_game_decorations+=1
  if stage in [7,14]:
   var path:String=AI_APPROVED_3D.approved_model_path("chambers","chamber_%02d"%stage)
   if not path.is_empty() and ResourceLoader.exists(path):
    ai_art_chamber_models.append({"node":room_node,"stage":stage,"banner":banner})

func _ai_art_build_spell_icons()->void:
 # Only real native Q/E/R spells, and only if the player provided PNG icons.
 if not is_instance_valid(v386_orb_image):return
 var anchor:Control=v386_orb_image.get_parent() as Control
 if not is_instance_valid(anchor):return
 var strip:=HBoxContainer.new()
 strip.name="AIArt_ApprovedSpellIcons"
 strip.position=Vector2(290.0,238.0)
 strip.add_theme_constant_override("separation",3)
 strip.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var count:int=0
 for spell in ["nova","dash","fire"]:
  var art:Texture2D=AI_WHOLE_GAME_ART.optional_texture("spells",spell)
  if art==null:continue
  var icon:=TextureRect.new()
  icon.texture=art
  icon.custom_minimum_size=Vector2(35,35)
  icon.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
  icon.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
  icon.mouse_filter=Control.MOUSE_FILTER_IGNORE
  icon.tooltip_text=spell.capitalize()
  strip.add_child(icon)
  count+=1
 if count>0:
  anchor.add_child(strip)
  ai_art_game_glyphs+=count
 else:strip.queue_free()

func _ready()->void:
 super._ready()
 _create_atlas()
 _ai_art_upgrade_existing_glyphs()
 _ai_art_decorate_regions_and_chambers()
 _ai_art_build_spell_icons()

# Roll each guardian once per adventure. Saving the rolled species prevents
# checkpoint reloads from re-randomizing an unfinished boss encounter.
func _atlas_roll_regional_guardians()->void:
 atlas_regional_guardian_rolls.clear()
 var pool_size:int=V321_RULES.REGIONAL_GUARDIAN_SPECIES.size()
 if pool_size==0:return
 for zone in range(REGIONS.size()):
  # Shuffle the four distinct species in a zone, then choose one wildcard.
  # Guarantee all native guardian types appear per region, without adjacency repeats.
  var available:Array[int]=[]
  for species_id in range(pool_size):available.append(species_id)
  for dungeon_id in range(V321_RULES.REGIONAL_DUNGEON_COUNT):
   if available.is_empty():
    for species_id in range(pool_size):available.append(species_id)
   var pick:int=rng.randi_range(0,available.size()-1)
   if dungeon_id>0 and available.size()>1 and available[pick]==atlas_regional_guardian_rolls.back():
    pick=(pick+1)%available.size()
   atlas_regional_guardian_rolls.append(available[pick])
   available.remove_at(pick)

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 atlas_has_waypoint=false
 atlas_zoom_radius=120.0
 atlas_discovered.clear()
 atlas_chambers_seen.clear()
 atlas_regional_seen.clear()
 atlas_regional_relics_claimed.clear()
 atlas_regional_guardians_cleared.clear()
 atlas_regional_guardian_actors.clear()
 _atlas_roll_regional_guardians()
 _americas_roll_profiles()
 _americas_apply_wildlife_profiles()
 atlas_fast_travel_unlocked_cache.clear()
 atlas_regional_dungeon=-1
 atlas_dungeon_paths.clear()
 atlas_chamber_scan_elapsed=0.0
 atlas_dungeon_site=-1
 atlas_dungeon_zoom_radius=NAVIGATION_MAP.DUNGEON_DEFAULT_ZOOM
 _refresh_atlas()

func _checkpoint_extra_data()->Dictionary:
 var data:Dictionary=super._checkpoint_extra_data()
 data["atlas_quest_positions"]=atlas_quest_positions.map(func(p:Vector3):return [p.x,p.y,p.z])
 data["atlas_discovered"]=NAVIGATION_MAP.encode_history(atlas_discovered)
 data["atlas_chambers_seen"]=atlas_chambers_seen.duplicate()
 data["atlas_regional_seen"]=atlas_regional_seen.duplicate()
 data["atlas_regional_relics_claimed"]=atlas_regional_relics_claimed.duplicate()
 data["atlas_regional_guardians_cleared"]=atlas_regional_guardians_cleared.duplicate()
 data["atlas_regional_guardian_rolls"]=atlas_regional_guardian_rolls.duplicate()
 data["americas_wildlife_rolls"]=american_wildlife_rolls.duplicate()
 var saved_paths:Dictionary={}
 for site_key in atlas_dungeon_paths.keys():
  var points:Array[Vector3]=atlas_dungeon_paths[site_key]
  saved_paths[str(site_key)]=NAVIGATION_MAP.encode_history(points)
 data["atlas_dungeon_paths"]=saved_paths
 data["atlas_zoom_radius"]=atlas_zoom_radius
 data["atlas_has_waypoint"]=atlas_has_waypoint
 data["atlas_waypoint"]=[atlas_waypoint.x,atlas_waypoint.y,atlas_waypoint.z]
 return data

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 var extras:Variant=data.get("optional_resources",{})
 atlas_has_waypoint=false
 atlas_discovered.clear()
 atlas_chambers_seen.clear()
 atlas_regional_seen.clear()
 atlas_regional_relics_claimed.clear()
 atlas_regional_guardians_cleared.clear()
 atlas_regional_guardian_actors.clear()
 atlas_regional_guardian_rolls.clear()
 american_wildlife_rolls.clear()
 atlas_fast_travel_unlocked_cache.clear()
 atlas_dungeon_paths.clear()
 atlas_quest_positions.clear()
 atlas_regional_dungeon=-1
 atlas_dungeon_site=-1
 atlas_dungeon_zoom_radius=NAVIGATION_MAP.DUNGEON_DEFAULT_ZOOM
 atlas_zoom_radius=clampf(float(extras.get("atlas_zoom_radius",120.0)),25.0,2000.0) if extras is Dictionary else 120.0
 if not is_finite(atlas_zoom_radius):atlas_zoom_radius=120.0
 if extras is Dictionary:
  atlas_discovered=NAVIGATION_MAP.decode_history(extras.get("atlas_discovered",[]))
  atlas_chambers_seen=NAVIGATION_MAP.decode_chambers(extras.get("atlas_chambers_seen",[]),v321_chambers.size()+v321_annexes.size())
  atlas_regional_seen=NAVIGATION_MAP.decode_chambers(extras.get("atlas_regional_seen",[]),REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT*V321_RULES.REGIONAL_CHAMBERS_PER_DUNGEON)
  atlas_regional_relics_claimed=NAVIGATION_MAP.decode_chambers(extras.get("atlas_regional_relics_claimed",[]),REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT*V321_RULES.REGIONAL_CACHE_STAGES.size())
  atlas_regional_guardians_cleared=NAVIGATION_MAP.decode_chambers(extras.get("atlas_regional_guardians_cleared",[]),REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT)
  var saved_guardians:Variant=extras.get("atlas_regional_guardian_rolls",[])
  var expected_rolls:int=REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT
  if saved_guardians is Array and saved_guardians.size()==expected_rolls:
   var valid_rolls:bool=true
   for choice in saved_guardians:
    if not (choice is int) or int(choice)<0 or int(choice)>=V321_RULES.REGIONAL_GUARDIAN_SPECIES.size():
     valid_rolls=false
     break
   if valid_rolls:
    for choice in saved_guardians:atlas_regional_guardian_rolls.append(int(choice))
  var saved_american_rolls:Variant=extras.get("americas_wildlife_rolls",[])
  if saved_american_rolls is Array and saved_american_rolls.size()==REGIONS.size():
   var valid_wildlife:bool=true
   for offset in saved_american_rolls:
    if not (offset is int) or int(offset)<0 or int(offset)>1000000:
     valid_wildlife=false
     break
   if valid_wildlife:
    for offset in saved_american_rolls:american_wildlife_rolls.append(int(offset))
  var loaded_paths:Variant=extras.get("atlas_dungeon_paths",{})
  if loaded_paths is Dictionary:
   for site_id in range(mini(rifts.size()+REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT,64)):
    var points:Array[Vector3]=NAVIGATION_MAP.decode_history(loaded_paths.get(str(site_id),[]))
    while points.size()>NAVIGATION_MAP.MAX_DUNGEON_TRAIL_POINTS:points.pop_front()
    if not points.is_empty():atlas_dungeon_paths[site_id]=points
  var quests:Variant=extras.get("atlas_quest_positions",[])
  if quests is Array:
   for item in quests.slice(0,24):
    if item is Array and item.size()==3:
     var qx:float=float(item[0])
     var qy:float=float(item[1])
     var qz:float=float(item[2])
     if is_finite(qx) and is_finite(qy) and is_finite(qz):atlas_quest_positions.append(Vector3(qx,qy,qz))
  var point:Variant=extras.get("atlas_waypoint",[])
  if point is Array and point.size()==3:
   var x:float=float(point[0])
   var y:float=float(point[1])
   var z:float=float(point[2])
   if is_finite(x) and is_finite(y) and is_finite(z):
    atlas_waypoint=Vector3(x,y,z)
    atlas_has_waypoint=bool(extras.get("atlas_has_waypoint",false))
 if atlas_regional_guardian_rolls.size()!=REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT:
  _atlas_roll_regional_guardians()
 if american_wildlife_rolls.size()!=REGIONS.size():_americas_roll_profiles()
 _americas_apply_wildlife_profiles()
 _refresh_atlas()

func _create_atlas()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 atlas_panel=PanelContainer.new()
 atlas_panel.name="NavigationAtlas"
 atlas_panel.set_anchors_preset(Control.PRESET_TOP_LEFT)
 atlas_panel.position=Vector2(18,175)
 atlas_panel.custom_minimum_size=Vector2(310,340)
 layer.add_child(atlas_panel)
 atlas_open_button=Button.new()
 atlas_open_button.name="OpenAtlasButton"
 atlas_open_button.text="MAP (M)"
 atlas_open_button.position=Vector2(18,125)
 atlas_open_button.custom_minimum_size=Vector2(115,42)
 atlas_open_button.pressed.connect(_toggle_atlas)
 layer.add_child(atlas_open_button)
 var layout:VBoxContainer=VBoxContainer.new()
 atlas_panel.add_child(layout)
 var heading:Label=Label.new()
 heading.text="MOONFALL ATLAS | N ↑ | CLICK TO MARK"
 atlas_heading=heading
 layout.add_child(heading)
 atlas_chart=NAVIGATION_MAP.new()
 atlas_chart.custom_minimum_size=Vector2(300,240)
 atlas_chart.set("metres_per_half_width",atlas_zoom_radius)
 layout.add_child(atlas_chart)
 atlas_chart.connect("waypoint_selected",_atlas_select_waypoint)
 atlas_chart.connect("zoom_requested",_atlas_zoom)
 atlas_readout=Label.new()
 atlas_readout.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(atlas_readout)
 atlas_legend=Label.new()
 atlas_legend.text="BLUE: YOU  |  GOLD: WAYPOINT / VISITED CHAMBER  |  PURPLE: QUEST  |  TEAL: EXPLORED TRAIL"
 atlas_legend.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(atlas_legend)
 var buttons:HBoxContainer=HBoxContainer.new()
 layout.add_child(buttons)
 var mark:Button=Button.new()
 mark.text="MARK HERE"
 mark.pressed.connect(_atlas_mark_here)
 buttons.add_child(mark)
 var clear:Button=Button.new()
 clear.text="CLEAR"
 clear.pressed.connect(_atlas_clear)
 buttons.add_child(clear)
 # Fast travel is enabled only for previously visited dungeon entrances.
 var travel_row:=HBoxContainer.new()
 layout.add_child(travel_row)
 atlas_fast_travel_select=OptionButton.new()
 atlas_fast_travel_select.name="DiscoveredDungeonDestinations"
 atlas_fast_travel_select.size_flags_horizontal=Control.SIZE_EXPAND_FILL
 atlas_fast_travel_select.tooltip_text="Visit a dungeon entrance to unlock it for fast travel."
 atlas_fast_travel_select.focus_mode=Control.FOCUS_ALL
 travel_row.add_child(atlas_fast_travel_select)
 atlas_fast_travel_button=Button.new()
 atlas_fast_travel_button.name="DiscoveredDungeonFastTravel"
 atlas_fast_travel_button.text="TRAVEL"
 atlas_fast_travel_button.tooltip_text="Return to a discovered dungeon entrance when out of combat."
 atlas_fast_travel_button.pressed.connect(_atlas_fast_travel_to_selected)
 travel_row.add_child(atlas_fast_travel_button)
 atlas_fast_travel_status=Label.new()
 atlas_fast_travel_status.name="DungeonDiscoveryProgress"
 atlas_fast_travel_status.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(atlas_fast_travel_status)
 var zoom_controls:HBoxContainer=HBoxContainer.new()
 layout.add_child(zoom_controls)
 for item in [{"text":"ZOOM +","multiplier":0.75},{"text":"ZOOM -","multiplier":1.333333}]:
  var zoom:Button=Button.new()
  zoom.text=str(item["text"])
  zoom.pressed.connect(_atlas_zoom.bind(float(item["multiplier"])))
  zoom_controls.add_child(zoom)
 var close:Button=Button.new()
 close.text="CLOSE"
 close.pressed.connect(func():atlas_panel.visible=false)
 buttons.add_child(close)
 atlas_panel.visible=false
 _refresh_atlas()

func _atlas_zoom(factor:float)->void:
 if not is_instance_valid(atlas_chart):return
 if atlas_dungeon_site>=0 or atlas_regional_dungeon>=0:
  atlas_dungeon_zoom_radius=clampf(atlas_dungeon_zoom_radius*factor,25.0,90.0)
 else:
  atlas_zoom_radius=clampf(atlas_zoom_radius*factor,25.0,2000.0)
 atlas_chart.set("metres_per_half_width",atlas_dungeon_zoom_radius if atlas_dungeon_site>=0 or atlas_regional_dungeon>=0 else atlas_zoom_radius)
 _refresh_atlas()

func _toggle_atlas()->void:
 if not hero_playing or menu_open or death_active or not is_instance_valid(atlas_panel):return
 atlas_panel.visible=not atlas_panel.visible
 _refresh_atlas()

func _atlas_select_waypoint(location:Vector3)->void:
 if not hero_playing or death_active or menu_open:return
 atlas_waypoint=location
 atlas_has_waypoint=true
 _refresh_atlas()

# Stage 1 is the entrance. Discovering later rooms does not silently unlock
# another dungeon, and progress is already saved via atlas_regional_seen.
# Never clear a live dropdown every 0.15s: that interrupts mouse/keyboard
# selection. Rebuild the options only when a new entrance is discovered.
func _atlas_refresh_fast_travel()->void:
 if not is_instance_valid(atlas_fast_travel_select) or not is_instance_valid(atlas_fast_travel_button):return
 var unlocked:Array[int]=[]
 for zone in range(REGIONS.size()):
  for dungeon_id in range(V321_RULES.REGIONAL_DUNGEON_COUNT):
   if atlas_regional_seen.has(_atlas_regional_room_id(zone,dungeon_id,1)):
    unlocked.append(zone*V321_RULES.REGIONAL_DUNGEON_COUNT+dungeon_id)
 if unlocked!=atlas_fast_travel_unlocked_cache:
  var previous_id:int=atlas_fast_travel_select.get_selected_id()
  atlas_fast_travel_select.clear()
  for destination_id in unlocked:
   var zone:int=int(destination_id/V321_RULES.REGIONAL_DUNGEON_COUNT)
   var dungeon_id:int=destination_id%V321_RULES.REGIONAL_DUNGEON_COUNT
   atlas_fast_travel_select.add_item("Region %d • Dungeon %d" % [zone+1,dungeon_id+1],destination_id)
  var keep:int=atlas_fast_travel_select.get_item_index(previous_id)
  if keep>=0:atlas_fast_travel_select.select(keep)
  elif atlas_fast_travel_select.item_count>0:atlas_fast_travel_select.select(0)
  atlas_fast_travel_unlocked_cache=unlocked
 var safe:bool=hero_playing and not death_active and not menu_open and not mounted and is_instance_valid(player) and not _combat_threat()
 atlas_fast_travel_button.disabled=unlocked.is_empty() or not safe
 atlas_fast_travel_select.disabled=unlocked.is_empty()
 if unlocked.is_empty():atlas_fast_travel_button.tooltip_text="Discover a dungeon entrance to unlock fast travel."
 elif not safe:atlas_fast_travel_button.tooltip_text="Fast travel requires being alive, unmounted and out of combat."
 else:atlas_fast_travel_button.tooltip_text="Travel to the selected explored dungeon entrance."
 if is_instance_valid(atlas_fast_travel_status):
  var total:int=REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT
  atlas_fast_travel_status.text="Dungeons discovered: %d / %d • Select an unlocked entrance, then TRAVEL" % [unlocked.size(),total] if not unlocked.is_empty() else "Dungeons discovered: 0 / %d • Find a dungeon entrance to unlock travel" % total

func _atlas_discovered_entrance_markers()->Array[Vector3]:
 var markers:Array[Vector3]=[]
 for zone in range(REGIONS.size()):
  var centre:Vector2=REGIONS[zone]["at"]
  for dungeon_id in range(V321_RULES.REGIONAL_DUNGEON_COUNT):
   if atlas_regional_seen.has(_atlas_regional_room_id(zone,dungeon_id,1)):
    markers.append(V321_RULES.regional_dungeon_position(centre,dungeon_id))
 return markers

func _atlas_fast_travel_to_selected()->void:
 if not hero_playing or death_active or menu_open or mounted or not is_instance_valid(player):return
 if _combat_threat():
  _show_message("Fast travel is unavailable during combat.",2.0)
  return
 if not is_instance_valid(atlas_fast_travel_select) or atlas_fast_travel_select.selected<0:return
 var destination_id:int=atlas_fast_travel_select.get_selected_id()
 if destination_id<0 or destination_id>=REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT:return
 var zone:int=int(destination_id/V321_RULES.REGIONAL_DUNGEON_COUNT)
 var dungeon_id:int=destination_id%V321_RULES.REGIONAL_DUNGEON_COUNT
 if not atlas_regional_seen.has(_atlas_regional_room_id(zone,dungeon_id,1)):
  _show_message("Discover that dungeon entrance first.",2.0)
  return
 var centre:Vector2=REGIONS[zone]["at"]
 var entrance:Vector3=V321_RULES.regional_dungeon_position(centre,dungeon_id)
 if not NAVIGATION_MAP._valid_world_point(entrance):return
 # Place the player above the known solid entrance floor, never inside a room
 # or at a remote unverified waypoint. The player body settles via physics.
 player.global_position=entrance+Vector3(0,1.25,0)
 if player is CharacterBody3D:(player as CharacterBody3D).velocity=Vector3.ZERO
 atlas_regional_dungeon=destination_id
 atlas_dungeon_site=-1
 atlas_panel.visible=false
 _show_message("Fast traveled to Region %d, Dungeon %d." % [zone+1,dungeon_id+1],2.5)

# The normal interact key can claim a nearby reward without opening a
# separate mode or requiring waves, and old world interactions still work.
func _interact()->void:
 if hero_playing and not death_active and not menu_open and not mounted and is_instance_valid(player):
  var nearby_cache:Dictionary={}
  var closest_sq:float=4.0*4.0
  for chamber in v321_region_rooms:
   var stage:int=int(chamber.get("stage",0))
   if not (stage in V321_RULES.REGIONAL_CACHE_STAGES):continue
   var node:Node3D=chamber.get("node") as Node3D
   if not is_instance_valid(node):continue
   var interact_at:Vector3=node.global_position+Vector3(2.7,0,0)
   var dist_sq:float=Vector2(player.global_position.x-interact_at.x,player.global_position.z-interact_at.z).length_squared()
   if dist_sq<closest_sq:
    nearby_cache=chamber
    closest_sq=dist_sq
  if not nearby_cache.is_empty():
   if _combat_threat():
    _show_message("Relics cannot be claimed during combat.",2.0)
    return
   var zone:int=int(nearby_cache["zone"])
   var dungeon_id:int=int(nearby_cache["dungeon"])
   var stage:int=int(nearby_cache["stage"])
   var reward_id:int=V321_RULES.regional_cache_index(zone,dungeon_id,stage)
   if reward_id<0:return
   if atlas_regional_relics_claimed.has(reward_id):
    _show_message("This dungeon relic has already been claimed.",2.0)
    return
   atlas_regional_relics_claimed.append(reward_id)
   var reward:Dictionary=V321_RULES.regional_cache_reward(zone,dungeon_id,stage,AFFIXES[(zone*5+dungeon_id)%AFFIXES.size()])
   var spawn_at:Vector3=(nearby_cache["node"] as Node3D).global_position+Vector3(2.7,0,1.0)
   _create_gear_drop(spawn_at,"Dungeon Relic Cache","elder" if stage==14 else "rare",zone,reward)
   _show_message("Dungeon relic discovered: "+str(reward["name"]),3.0)
   return
 super._interact()

# Final-chamber guardians inhabit the existing open world, not a wave mode.
# One native boss is created lazily per visited dungeon, instead of spawning
# thirty extra bosses during scene setup.
func _atlas_update_dungeon_guardians(position:Vector3)->void:
 if not hero_playing or death_active or not NAVIGATION_MAP._valid_world_point(position):return
 for zone in range(REGIONS.size()):
  var centre:Vector2=REGIONS[zone]["at"]
  for dungeon_id in range(V321_RULES.REGIONAL_DUNGEON_COUNT):
   var guardian_id:int=zone*V321_RULES.REGIONAL_DUNGEON_COUNT+dungeon_id
   if atlas_regional_guardians_cleared.has(guardian_id) or atlas_regional_guardian_actors.has(guardian_id):continue
   var room_position:Vector3=V321_RULES.regional_chamber_position(centre,dungeon_id,14)
   var horizontal:Vector2=Vector2(position.x-room_position.x,position.z-room_position.z)
   if horizontal.length_squared()>DUNGEON_GUARDIAN_TRIGGER_RADIUS_SQ:continue
   var before:int=beasts.size()
   if guardian_id>=atlas_regional_guardian_rolls.size():continue
   var rolled_index:int=atlas_regional_guardian_rolls[guardian_id]
   if rolled_index<0 or rolled_index>=V321_RULES.REGIONAL_GUARDIAN_SPECIES.size():continue
   var monster:String=V321_RULES.REGIONAL_GUARDIAN_SPECIES[rolled_index]
   if not BEASTS.has(monster):continue
   _spawn_beast(room_position,monster,zone,"elder",true)
   if beasts.size()<=before:continue
   var guardian:Dictionary=beasts[-1]
   guardian["regional_guardian_id"]=guardian_id
   guardian["home"]=room_position
   var trait_id:int=V321_RULES.regional_guardian_trait(zone,dungeon_id,rolled_index)
   guardian["regional_guardian_trait"]=trait_id
   var health_scale:float=V321_RULES.regional_guardian_health_multiplier(trait_id)
   guardian["max"]=float(guardian["max"])*health_scale
   guardian["hp"]=guardian["max"]
   guardian["base_hp"]=float(guardian["base_hp"])*health_scale
   var title:Label3D=guardian.get("nameplate") as Label3D
   if is_instance_valid(title):
    title.text="%s %s GUARDIAN • %d / 5" % [V321_RULES.REGIONAL_GUARDIAN_TRAITS[trait_id].to_upper(),monster.to_upper(),dungeon_id+1]
    title.modulate=V321_RULES.REGIONAL_GUARDIAN_TRAIT_COLORS[trait_id]
   # Later 55-profile guardian types can use approved custom images automatically.
   var approved_profile:int=int(guardian.get("regional_guardian_profile",-1))
   var guardian_art_body:CharacterBody3D=guardian.get("body") as CharacterBody3D
   if approved_profile>=0 and is_instance_valid(guardian_art_body):
    var profile_id:String="guardian_%02d"%approved_profile
    var guardian_model:Node3D=AI_APPROVED_3D.add_approved_model(guardian_art_body,"guardians",profile_id,Vector3.ZERO,3.6,true)
    if is_instance_valid(guardian_model):
     guardian["ai_art_animation_binding"]=AI_ART_ANIMATION.bind_imported(guardian_model)
    else:
     APPROVED_AI_ART.attach_approved(guardian_art_body,APPROVED_AI_ART.guardian_path(approved_profile))
   atlas_regional_guardian_actors[guardian_id]=guardian
   _show_message("A dungeon guardian defends the Eclipse Threshold!",3.0)
   return

# Dynamic pickups and newly spawned bosses also reuse their existing
# v31 art glyphs. Approved gear art is cosmetic, never equipment logic.
func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var before:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.size()<=before:return
 var body:CharacterBody3D=beasts[-1].get("body") as CharacterBody3D
 if not is_instance_valid(body):return
 var phase_crest:Sprite3D=body.get_node_or_null("Art_BossPhaseCrest") as Sprite3D
 var carrier_crest:Sprite3D=body.get_node_or_null("Art_TreasureBearerCrest") as Sprite3D
 if AI_WHOLE_GAME_ART.override_sprite(phase_crest,"glyphs","boss_phase"):ai_art_game_glyphs+=1
 if AI_WHOLE_GAME_ART.override_sprite(carrier_crest,"glyphs","treasure_bearer"):ai_art_game_glyphs+=1

func _create_gear_drop(at:Vector3,kind:String,variant:String,zone:int,existing:Dictionary={})->void:
 var before:int=gear_drops.size()
 super._create_gear_drop(at,kind,variant,zone,existing)
 if gear_drops.size()<=before:return
 var drop:Dictionary=gear_drops[-1]
 var node:Node3D=drop.get("node") as Node3D
 if not is_instance_valid(node):return
 var glyph:Sprite3D=node.get_node_or_null("Art_RareLootGlyph") as Sprite3D
 if AI_WHOLE_GAME_ART.override_sprite(glyph,"glyphs","rare_loot"):ai_art_game_glyphs+=1
 var gear:Dictionary=drop.get("data",{})
 var name:String=str(gear.get("name","")).to_lower()
 var art_kind:String=""
 for suffix in ["charm","band","cowl","boots","mantle","crest","relic","weapon"]:
  if name.contains(suffix):art_kind=suffix;break
 if art_kind.is_empty():return
 var model:Node3D=AI_APPROVED_3D.add_approved_model(node,"gear","gear_"+art_kind,Vector3(0,0.6,0),0.75)
 if is_instance_valid(model):
  ai_art_game_decorations+=1
  if is_instance_valid(glyph):glyph.visible=false
  return
 var image:Sprite3D=AI_WHOLE_GAME_ART.add_billboard(node,"gear",art_kind,Vector3(0,0.78,0),0.85)
 if not is_instance_valid(image):return
 ai_art_game_decorations+=1
 if is_instance_valid(glyph):glyph.visible=false

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 var guardian_id:int=int(beast.get("regional_guardian_id",-1))
 var first_clear:bool=bool(beast.get("alive",false)) and guardian_id>=0 and guardian_id<REGIONS.size()*V321_RULES.REGIONAL_DUNGEON_COUNT and not atlas_regional_guardians_cleared.has(guardian_id)
 var guardian_body:CharacterBody3D=beast.get("body") as CharacterBody3D
 var guardian_drop_at:Vector3=guardian_body.global_position if first_clear and is_instance_valid(guardian_body) else Vector3.ZERO
 if first_clear:atlas_regional_guardians_cleared.append(guardian_id)
 super._defeat_beast(beast,pet_kill)
 if first_clear:
  # Native animals normally respawn. Dungeon bosses remain defeated.
  beast["respawn_at"]=1.0e30
  var zone:int=int(beast.get("zone",-1))
  var dungeon_id:int=guardian_id%V321_RULES.REGIONAL_DUNGEON_COUNT
  var trait_id:int=int(beast.get("regional_guardian_trait",-1))
  if zone>=0 and zone<REGIONS.size() and trait_id>=0 and trait_id<V321_RULES.REGIONAL_GUARDIAN_TRAITS.size():
   if is_instance_valid(guardian_body) and NAVIGATION_MAP._valid_world_point(guardian_drop_at):
    var relic:Dictionary=V321_RULES.regional_guardian_relic(zone,dungeon_id,trait_id,AFFIXES[guardian_id%AFFIXES.size()])
    _create_gear_drop(guardian_drop_at,"Dungeon Guardian Crest","elder",zone,relic)
  _show_message("DUNGEON GUARDIAN DEFEATED! The final chamber is secured.",3.0)

func _atlas_mark_here()->void:
 if not hero_playing or not is_instance_valid(player):return
 atlas_waypoint=player.global_position
 atlas_has_waypoint=true
 _refresh_atlas()

func _atlas_clear()->void:
 atlas_has_waypoint=false
 _refresh_atlas()

func _atlas_record_nearby_chambers(position:Vector3)->void:
 if not NAVIGATION_MAP._valid_world_point(position):return
 for chamber in _atlas_all_site_chambers():
  var site_id:int=int(chamber.get("site",-1))
  var stage:int=int(chamber.get("stage",0))
  if site_id<0 or site_id>=rifts.size() or stage<1 or stage>14:continue
  var chamber_id:int=_atlas_room_id(site_id,stage)
  if chamber_id in atlas_chambers_seen:continue
  var node:Node3D=chamber.get("node") as Node3D
  if not is_instance_valid(node):continue
  var offset:Vector2=Vector2(node.global_position.x-position.x,node.global_position.z-position.z)
  if offset.length_squared()<=12.0*12.0:atlas_chambers_seen.append(chamber_id)

func _atlas_chamber_marker_positions()->Array[Vector3]:
 var markers:Array[Vector3]=[]
 for chamber in _atlas_all_site_chambers():
  var site_id:int=int(chamber.get("site",-1))
  var stage:int=int(chamber.get("stage",0))
  if site_id<0 or site_id>=rifts.size() or stage<1 or stage>14:continue
  if not (_atlas_room_id(site_id,stage) in atlas_chambers_seen):continue
  var chamber_node:Node3D=chamber.get("node") as Node3D
  if not is_instance_valid(chamber_node):continue
  if NAVIGATION_MAP._valid_world_point(chamber_node.global_position):markers.append(chamber_node.global_position)
 return markers

func _atlas_record_regional_chambers(position:Vector3)->void:
 if not NAVIGATION_MAP._valid_world_point(position):return
 for entry in v321_region_rooms:
  var node:Node3D=entry.get("node") as Node3D
  if not is_instance_valid(node):continue
  if Vector2(position.x-node.global_position.x,position.z-node.global_position.z).length_squared()>12.0*12.0:continue
  var room_id:int=_atlas_regional_room_id(int(entry["zone"]),int(entry["dungeon"]),int(entry["stage"]))
  if not atlas_regional_seen.has(room_id):atlas_regional_seen.append(room_id)

func _atlas_update_dungeon_context(position:Vector3)->void:
 var anchors:Array[Vector3]=[]
 for entry in rifts:
  var anchor:Vector3=entry.get("anchor",Vector3.ZERO)
  anchors.append(anchor)
 var next_site:int=NAVIGATION_MAP.dungeon_site_at(position,anchors,atlas_dungeon_site)
 var next_regional:int=-1
 var regional_distance:float=13.0*13.0
 for entry in v321_region_rooms:
  var node:Node3D=entry.get("node") as Node3D
  if not is_instance_valid(node):continue
  var distance:float=Vector2(position.x-node.global_position.x,position.z-node.global_position.z).length_squared()
  if distance<regional_distance:
   regional_distance=distance
   next_regional=int(entry["zone"])*V321_RULES.REGIONAL_DUNGEON_COUNT+int(entry["dungeon"])
 var nearest_distance:float=14.0*14.0
 for chamber in _atlas_all_site_chambers():
  var node:Node3D=chamber.get("node") as Node3D
  if not is_instance_valid(node):continue
  var distance:float=Vector2(position.x-node.global_position.x,position.z-node.global_position.z).length_squared()
  if distance<nearest_distance:
   nearest_distance=distance
   next_site=int(chamber.get("site",-1))
 if next_regional>=0:next_site=-1
 if next_site==atlas_dungeon_site and next_regional==atlas_regional_dungeon:return
 atlas_regional_dungeon=next_regional
 atlas_dungeon_site=next_site
 if is_instance_valid(atlas_panel) and atlas_panel.visible:_refresh_atlas()

func _atlas_dungeon_rooms()->Array[Vector3]:
 var rooms:Array[Vector3]=[]
 if atlas_regional_dungeon>=0:
  for entry in _atlas_regional_rooms():
   var node:Node3D=entry.get("node") as Node3D
   if is_instance_valid(node) and NAVIGATION_MAP._valid_world_point(node.global_position):rooms.append(node.global_position)
  return rooms
 if atlas_dungeon_site<0:return rooms
 for chamber in _atlas_all_site_chambers():
  if int(chamber.get("site",-1))!=atlas_dungeon_site:continue
  var node:Node3D=chamber.get("node") as Node3D
  if is_instance_valid(node) and NAVIGATION_MAP._valid_world_point(node.global_position):rooms.append(node.global_position)
 return rooms

func _atlas_dungeon_passages()->Array[Dictionary]:
 var links:Array[Dictionary]=[]
 if atlas_regional_dungeon>=0:
  var zone:int=int(atlas_regional_dungeon / V321_RULES.REGIONAL_DUNGEON_COUNT)
  var dungeon_id:int=atlas_regional_dungeon%V321_RULES.REGIONAL_DUNGEON_COUNT
  for passage in v321_region_room_passages:
   if int(passage["zone"])!=zone or int(passage["dungeon"])!=dungeon_id:continue
   var start_known:bool=false
   var end_known:bool=false
   for entry in _atlas_regional_rooms():
    var node:Node3D=entry.get("node") as Node3D
    if not is_instance_valid(node):continue
    var discovered:bool=atlas_regional_seen.has(_atlas_regional_room_id(zone,dungeon_id,int(entry["stage"])))
    if node.global_position.distance_squared_to(passage["start"])<1.0:start_known=discovered
    if node.global_position.distance_squared_to(passage["end"])<1.0:end_known=discovered
   links.append({"start":passage["start"],"end":passage["end"],"width":passage["width"],"explored":start_known and end_known})
  return links
 if atlas_dungeon_site<0:return links
 var all_passages:Array[Dictionary]=[]
 all_passages.append_array(v321_passages)
 all_passages.append_array(v321_annex_passages)
 for passage in all_passages:
  if int(passage.get("site",-1))!=atlas_dungeon_site:continue
  var start_known:bool=false
  var end_known:bool=false
  for chamber in _atlas_all_site_chambers():
   if int(chamber.get("site",-1))!=atlas_dungeon_site:continue
   var stage:int=int(chamber.get("stage",0))
   if not atlas_chambers_seen.has(_atlas_room_id(atlas_dungeon_site,stage)):continue
   var node:Node3D=chamber.get("node") as Node3D
   if not is_instance_valid(node):continue
   if node.global_position.distance_squared_to(passage["start"])<1.0:start_known=true
   if node.global_position.distance_squared_to(passage["end"])<1.0:end_known=true
  links.append({"start":passage["start"],"end":passage["end"],"width":passage["width"],"explored":start_known and end_known})
 return links

func _atlas_dungeon_floor_radii()->Array[float]:
 var radii:Array[float]=[]
 if atlas_regional_dungeon>=0:
  for entry in _atlas_regional_rooms():radii.append(6.0)
  return radii
 if atlas_dungeon_site<0:return radii
 for chamber in _atlas_all_site_chambers():
  if int(chamber.get("site",-1))!=atlas_dungeon_site:continue
  var node:Node3D=chamber.get("node") as Node3D
  if not is_instance_valid(node):continue
  var floor_name:String="RiftChamberFloor" if int(chamber.get("stage",0))<=3 else "RiftAnnexFloor"
  var floor_mesh:MeshInstance3D=node.get_node_or_null(floor_name) as MeshInstance3D
  var floor:CylinderMesh=floor_mesh.mesh as CylinderMesh if is_instance_valid(floor_mesh) else null
  radii.append(floor.top_radius if floor!=null else 0.0)
 return radii

func _atlas_discovered_dungeon_pillars()->Array[Vector3]:
 var pillars:Array[Vector3]=[]
 if atlas_regional_dungeon>=0:return pillars
 if atlas_dungeon_site<0:return pillars
 for chamber in v321_chambers:
  var site_id:int=int(chamber.get("site",-1))
  var stage:int=int(chamber.get("stage",0))
  if site_id!=atlas_dungeon_site or stage<1 or stage>3:continue
  if not atlas_chambers_seen.has(site_id*3+stage-1):continue
  var room_node:Node3D=chamber.get("node") as Node3D
  if not is_instance_valid(room_node):continue
  for pillar_index in range(3):
   var post:Node3D=room_node.get_node_or_null("RiftPillar_%d" % pillar_index) as Node3D
   if is_instance_valid(post) and NAVIGATION_MAP._valid_world_point(post.global_position):
    pillars.append(post.global_position)
 return pillars

func _refresh_atlas()->void:
 if not is_instance_valid(atlas_chart) or not is_instance_valid(atlas_readout):return
 var pos:Vector3=player.global_position if is_instance_valid(player) else Vector3.ZERO
 atlas_chart.set("metres_per_half_width",atlas_dungeon_zoom_radius if atlas_dungeon_site>=0 or atlas_regional_dungeon>=0 else atlas_zoom_radius)
 atlas_chart.call("set_dungeon_context",atlas_dungeon_site>=0 or atlas_regional_dungeon>=0,_atlas_dungeon_rooms(),_atlas_dungeon_floor_radii())
 atlas_chart.call("set_dungeon_passages",_atlas_dungeon_passages())
 if is_instance_valid(atlas_heading):
  atlas_heading.text=("REGION %d | DUNGEON %d / 5 | 14 ROOMS" % [int(atlas_regional_dungeon / V321_RULES.REGIONAL_DUNGEON_COUNT)+1,atlas_regional_dungeon%V321_RULES.REGIONAL_DUNGEON_COUNT+1]) if atlas_regional_dungeon>=0 else (("DUNGEON FLOOR PLAN: "+str(rifts[atlas_dungeon_site].get("name","Rift"))) if atlas_dungeon_site>=0 and atlas_dungeon_site<rifts.size() else "MOONFALL WORLD ATLAS | N ↑ | CLICK TO MARK")
 atlas_chart.call("set_discovered",atlas_discovered)
 var regional_markers:Array[Vector3]=[]
 if atlas_regional_dungeon>=0:
  for room in _atlas_regional_rooms():
   if not atlas_regional_seen.has(_atlas_regional_room_id(int(room["zone"]),int(room["dungeon"]),int(room["stage"]))):continue
   var node:Node3D=room.get("node") as Node3D
   if is_instance_valid(node):regional_markers.append(node.global_position)
 var map_markers:Array[Vector3]=regional_markers if atlas_regional_dungeon>=0 else _atlas_chamber_marker_positions()
 if atlas_dungeon_site<0 and atlas_regional_dungeon<0:map_markers.append_array(_atlas_discovered_entrance_markers())
 atlas_chart.call("set_chamber_markers",map_markers)
 atlas_chart.call("set_dungeon_pillars",_atlas_discovered_dungeon_pillars())
 var local_trace:Array[Vector3]=[]
 var path_key:int=_atlas_regional_trail_key(atlas_regional_dungeon) if atlas_regional_dungeon>=0 else atlas_dungeon_site
 if path_key>=0 and atlas_dungeon_paths.has(path_key):local_trace=atlas_dungeon_paths[path_key]
 atlas_chart.call("set_dungeon_trace",local_trace)
 atlas_chart.call("set_quest_markers",atlas_quest_positions)
 atlas_chart.call("update_navigation",pos,atlas_waypoint,atlas_has_waypoint)
 if is_instance_valid(player):atlas_chart.call("update_player_facing",-player.global_transform.basis.z)
 atlas_chart.call("update_dungeon_room",pos)
 _atlas_refresh_fast_travel()
 atlas_readout.text="X %.0f | Z %.0f | Trail: %d/%d | Chambers: %d/%d | Rifts: %d/%d | Quests: %d" % [pos.x,pos.z,atlas_discovered.size(),ATLAS_MAX_DISCOVERED,atlas_chambers_seen.size(),v321_chambers.size()+v321_annexes.size(),rifts_cleared.size(),rifts.size(),atlas_quest_positions.size()]
 if atlas_regional_dungeon>=0:
  var seen_count:int=0
  for room in _atlas_regional_rooms():
   if atlas_regional_seen.has(_atlas_regional_room_id(int(room["zone"]),int(room["dungeon"]),int(room["stage"]))):seen_count+=1
  atlas_readout.text+=" | This dungeon: %d/14" % seen_count
 if atlas_dungeon_site>=0:
  var local_seen:int=0
  for stage in range(1,15):
   if atlas_chambers_seen.has(_atlas_room_id(atlas_dungeon_site,stage)):local_seen+=1
  atlas_readout.text+=" | This dungeon: %d/14" % local_seen
  var current_room:int=atlas_chart.get("dungeon_current_room")
  atlas_readout.text+=(" | CURRENT ROOM: %d" % (current_room+1)) if current_room>=0 else " | Between chambers"
 if atlas_has_waypoint:
  var horizontal_distance:float=Vector2(pos.x,pos.z).distance_to(Vector2(atlas_waypoint.x,atlas_waypoint.z))
  var offset:Vector2=Vector2(atlas_waypoint.x-pos.x,atlas_waypoint.z-pos.z)
  var bearing:float=fposmod(rad_to_deg(atan2(offset.x,-offset.y)),360.0)
  atlas_readout.text+=" | Marker: %.0f m | Bearing: %.0f°" % [horizontal_distance,bearing]

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and is_instance_valid(atlas_panel) and atlas_panel.visible:
  if event.keycode==KEY_EQUAL or event.keycode==KEY_KP_ADD:
   _atlas_zoom(0.75)
   get_viewport().set_input_as_handled()
   return
  if event.keycode==KEY_MINUS or event.keycode==KEY_KP_SUBTRACT:
   _atlas_zoom(1.333333)
   get_viewport().set_input_as_handled()
   return
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_M:
  if is_instance_valid(atlas_panel) and hero_playing and not menu_open and not death_active:
   _toggle_atlas()
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
 _ai_art_track_frame_performance(delta)
 _ai_art_update_animations(delta)
 _americas_update_wildlife_visuals(delta)
 if not is_instance_valid(atlas_panel):return
 if is_instance_valid(atlas_open_button):atlas_open_button.visible=hero_playing and not death_active and not menu_open
 if not hero_playing or death_active or menu_open:
  atlas_panel.visible=false
 if hero_playing and not death_active and is_instance_valid(player):
  var here:Vector3=player.global_position
  atlas_chamber_scan_elapsed+=maxf(0.0,delta)
  if atlas_chamber_scan_elapsed>=0.25:
   atlas_chamber_scan_elapsed=0.0
   _atlas_record_nearby_chambers(here)
   _atlas_record_regional_chambers(here)
   _atlas_update_dungeon_context(here)
   _atlas_update_dungeon_guardians(here)
   if (atlas_dungeon_site>=0 or atlas_regional_dungeon>=0) and NAVIGATION_MAP._valid_world_point(here):
    var active_key:int=_atlas_regional_trail_key(atlas_regional_dungeon) if atlas_regional_dungeon>=0 else atlas_dungeon_site
    if not atlas_dungeon_paths.has(active_key):
     var empty_path:Array[Vector3]=[]
     atlas_dungeon_paths[active_key]=empty_path
    var path:Array[Vector3]=atlas_dungeon_paths[active_key]
    NAVIGATION_MAP.append_dungeon_sample(path,here)
  if NAVIGATION_MAP._valid_world_point(here):
   if atlas_discovered.is_empty() or atlas_discovered.back().distance_squared_to(Vector3(here.x,0.0,here.z))>=ATLAS_DISCOVERY_SPACING*ATLAS_DISCOVERY_SPACING:
    atlas_discovered.append(Vector3(here.x,0.0,here.z))
    if atlas_discovered.size()>ATLAS_MAX_DISCOVERED:atlas_discovered.pop_front()
 if atlas_panel.visible:
  atlas_elapsed+=delta
  if atlas_elapsed>=0.15:
   atlas_elapsed=0.0
   _refresh_atlas()

## Quest systems can register exact world-space goals without guessing region positions.
func atlas_set_quest_destination(world_position:Vector3)->void:
 if not is_finite(world_position.x) or not is_finite(world_position.y) or not is_finite(world_position.z):return
 atlas_quest_positions.clear()
 atlas_quest_positions.append(world_position)
 _refresh_atlas()


# Browse the entire curated roster without creating another gameplay mode.
func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which!="Bestiary" or not is_instance_valid(menu_body):return
 menu_body.text+="\n\n[b]WILDLIFE OF THE AMERICAS[/b] • %d habitat profiles • %d distinct species.\n" % [AMERICAS_WILDLIFE.total_profiles(),AMERICAS_WILDLIFE.distinct_species_count()]
 menu_body.text+="Native wolf, stag, boar, fox, serpent, panther, elk and owl models serve as visual stand-ins until individual species art is added. The existing ecosystem and combat rules remain intact.\n"
 for zone in range(REGIONS.size()):
  var species:Array[Dictionary]=AMERICAS_WILDLIFE.species_for_region(zone)
  menu_body.text+="\n[b]"+str(REGIONS[zone]["name"])+" | "+str(species.size())+" profiles[/b]\n"
  for animal in species:
   var taxon:String=AMERICAS_WILDLIFE.TAXON_LABELS.get(str(animal["taxon"]),"Wildlife")
   menu_body.text+=" • "+str(animal["name"])+" ("+taxon+")\n"

func atlas_clear_quest_destination()->void:
 atlas_quest_positions.clear()
 _refresh_atlas()
