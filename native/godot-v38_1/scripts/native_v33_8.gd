extends "res://scripts/native_v33_7.gd"
## v33.8 death and resurrection update.
## Intentionally keeps save schema 2 and the source-art files unchanged.
## The survival stopwatch does not appear in the game or death screen.
const PHOENIX_FEATHER:String="Moon Phoenix Feather"
const PHOENIX_DROP_CHANCE:float=0.18
const PHOENIX_GRACE_SECONDS:float=2.0
# Moon Sigils remain normal saved materials. No new wallet or save schema.
# Legendary pieces use the existing single equipment slot and power rules.
const MOON_OATH:String="Moonward Oath"
const MOON_RECIPES := [
 {"id":"blade","name":"Moonreaver Blade","cost":{"Moon Sigil":5,"Dusk Cinder":3,"Elder Rootstone":2},"result":"Moonreaver Blade","power":64,"kind":"gear"},
 {"id":"armor","name":"Eclipse Aegis","cost":{"Moon Sigil":4,"Serpent Scale":3,"Elder Rootstone":2},"result":"Eclipse Aegis","power":58,"kind":"gear"},
 {"id":"charm","name":"Moonfang Talisman","cost":{"Moon Sigil":3,"Fine Hide":2,"Dusk Cinder":1},"result":"Moonfang Talisman","power":51,"kind":"gear"},
 {"id":"feather","name":"Moon Phoenix Feather","cost":{"Moon Sigil":7,"Elder Rootstone":3,"Dusk Cinder":2},"result":"Moon Phoenix Feather","power":0,"kind":"feather"},
 {"id":"oath","name":"Moonward Oath","cost":{"Moon Sigil":6,"Elder Rootstone":4,"Dusk Cinder":2},"result":"Moonward Oath","power":0,"kind":"oath"}
]

var death_active:bool=false
var phoenix_used_this_expedition:bool=false
var phoenix_grace_until:float=0.0
var death_overlay:Control
var death_stats:Label
var death_continue:Button
var death_fade:ColorRect
var death_overlay_tween:Tween
var death_attacker:Dictionary={}
var revenge_target_id:int=-1
var death_killer_text:String=""

# Enshrouded-inspired world depth: no terrain editing, building modes or countdowns.
const DEPTH_CORRUPTION_RADIUS_SQ:float=13.0*13.0
const DEPTH_INTERACT_RADIUS_SQ:float=3.1*3.1
const DEPTH_VISUAL_TICK:float=0.35
const DEPTH_TRAVELER_ZONES:Array[int]=[1,3,5]
const DEPTH_LORE:Array[String]=[
 "The Witchwood moon paths twist around the old rift. Watch for violet light.",
 "The quarry keeps records of lost miners beneath its lunar scars.",
 "The citadel's moon glyphs answer to patient explorers, not hammers."
]
var depth_corruption:Array[Dictionary]=[]
var depth_glyphs:Array[Dictionary]=[]
var depth_travelers:Array[Dictionary]=[]
var depth_glyph_charged:Dictionary={}
var depth_spell_charges:int=0
var depth_visual_clock:float=0.0
var depth_corruption_here:int=-1
# Optional rift surge: a fixed, visible danger zone with generous warning and dodge counterplay.
# This is a combat hazard cadence, NEVER a survival/countdown timer.
const DEPTH_SURGE_REST:float=6.0
const DEPTH_SURGE_TELEGRAPH:float=1.6
const DEPTH_SURGE_HIT_RADIUS_SQ:float=4.2*4.2
var depth_surge_nodes:Array[Dictionary]=[]
var depth_surge_site:int=-1
var depth_surge_phase:int=0 # 0 resting; 1 warning. No countdown or time limit.
var depth_surge_clock:float=0.0
var depth_surge_hits:int=0
var depth_perfect_rifts:int=0

func _build_scene()->void:
 super._build_scene()
 _depth_build_corruption()
 _depth_build_glyphs()
 _depth_build_travelers()
 _depth_build_surges()

func _depth_build_corruption()->void:
 # A handful of shared meshes: one soft warning circle per existing rift.
 var ring_mesh:=CylinderMesh.new()
 ring_mesh.top_radius=13.0
 ring_mesh.bottom_radius=13.0
 ring_mesh.height=0.025
 ring_mesh.radial_segments=32
 var mat:=StandardMaterial3D.new()
 mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 mat.albedo_color=Color(0.36,0.06,0.45,0.18)
 mat.emission_enabled=true
 mat.emission=Color(0.23,0.02,0.31)
 for site in range(rifts.size()):
  var marker:=MeshInstance3D.new()
  marker.name="DepthCorruption_%d"%site
  marker.mesh=ring_mesh
  marker.material_override=mat
  marker.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  marker.position=(rifts[site]["anchor"] as Vector3)+Vector3(0.0,0.02,0.0)
  marker.visible=false
  world.add_child(marker)
  depth_corruption.append({"site":site,"node":marker})

func _depth_build_glyphs()->void:
 # Optional three-glyph sequence follows the existing three dungeon chambers.
 # Rewards spell charges, not infinitely farmable permanent materials.
 var base_mesh:=CylinderMesh.new()
 base_mesh.top_radius=0.65
 base_mesh.bottom_radius=0.65
 base_mesh.height=0.055
 base_mesh.radial_segments=12
 var orb_mesh:=SphereMesh.new()
 orb_mesh.radius=0.25
 orb_mesh.height=0.5
 var mat:=StandardMaterial3D.new()
 mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 mat.emission_enabled=true
 mat.albedo_color=Color("9357c2")
 mat.emission=Color("9357c2")
 for site in range(rifts.size()):
  for stage in range(1,4):
   var holder:=Node3D.new()
   holder.name="DepthMoonGlyph_%d_%d"%[site,stage]
   holder.position=V321_RULES.room_position(rifts[site]["anchor"],stage)
   world.add_child(holder)
   var pedestal:=MeshInstance3D.new()
   pedestal.mesh=base_mesh
   pedestal.material_override=mat
   pedestal.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   holder.add_child(pedestal)
   var orb:=MeshInstance3D.new()
   orb.mesh=orb_mesh
   orb.material_override=mat
   orb.position.y=0.55
   orb.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   holder.add_child(orb)
   var glyph_label:=Label3D.new()
   glyph_label.text="MOON GLYPH • DORMANT"
   glyph_label.font_size=29
   glyph_label.pixel_size=0.006
   glyph_label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
   glyph_label.position.y=1.45
   glyph_label.modulate=Color("f2bcff")
   holder.add_child(glyph_label)
   holder.visible=false
   depth_glyphs.append({"node":holder,"label":glyph_label,"site":site,"stage":stage})

func _depth_build_travelers()->void:
 # Three lightweight non-colliding travelers, not buildable settlements.
 var body_mesh:=CapsuleMesh.new()
 body_mesh.radius=0.32
 body_mesh.height=1.45
 var robe:=StandardMaterial3D.new()
 robe.albedo_color=Color("62607e")
 robe.roughness=0.95
 for i in range(DEPTH_TRAVELER_ZONES.size()):
  var zone:int=DEPTH_TRAVELER_ZONES[i]
  var center:Vector2=REGIONS[zone]["at"]
  var root:=Node3D.new()
  root.name="WanderingMoonScribe_%d"%i
  var base:=Vector3(center.x+10.0,0.0,center.y-7.0)
  root.position=base
  world.add_child(root)
  var figure:=MeshInstance3D.new()
  figure.mesh=body_mesh
  figure.material_override=robe
  figure.position.y=0.8
  figure.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  root.add_child(figure)
  var nameplate:=Label3D.new()
  nameplate.text="MOON SCRIBE • G TO TALK"
  nameplate.font_size=32
  nameplate.pixel_size=0.006
  nameplate.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  nameplate.position.y=2.15
  nameplate.modulate=Color("dbc5ff")
  root.add_child(nameplate)
  root.visible=false
  depth_travelers.append({"node":root,"base":base,"zone":zone,"lore":DEPTH_LORE[i]})

func _depth_near_corruption()->int:
 if not is_instance_valid(player):return -1
 var here:Vector3=player.global_position
 for site in range(rifts.size()):
  if bool(rifts[site].get("cleared",false)):continue
  if here.distance_squared_to(rifts[site]["anchor"])<=DEPTH_CORRUPTION_RADIUS_SQ:return site
 return -1

func _depth_tick(delta:float)->void:
 depth_visual_clock+=clampf(delta,0.0,0.2)
 if depth_visual_clock<DEPTH_VISUAL_TICK:return
 depth_visual_clock=fmod(depth_visual_clock,DEPTH_VISUAL_TICK)
 if not is_instance_valid(player):return
 var here:Vector3=player.global_position
 for item in depth_corruption:
  var site:int=int(item["site"])
  var sign:MeshInstance3D=item["node"]
  sign.visible=hero_playing and not bool(rifts[site].get("cleared",false)) and here.distance_squared_to(sign.global_position)<50.0*50.0
 for item in depth_glyphs:
  var site:int=int(item["site"])
  var stage:int=int(item["stage"])
  var root:Node3D=item["node"]
  root.visible=hero_playing and not bool(rifts[site].get("cleared",false)) and here.distance_squared_to(root.global_position)<31.0*31.0
  if not root.visible:continue
  var label:Label3D=item["label"]
  var key:String=str(site)+":"+str(stage)
  if depth_glyph_charged.has(key):label.text="MOON GLYPH • CHARGED"
  elif active_rift==site and int(rifts[site].get("stage",0))==stage:label.text="MOON GLYPH • G TO ATTUNE"
  else:label.text="MOON GLYPH • DORMANT"
 for item in depth_travelers:
  var root:Node3D=item["node"]
  var base:Vector3=item["base"]
  root.position=base+Vector3(sin(map_time*0.28+float(item["zone"]))*1.8,0.0,cos(map_time*0.22+float(item["zone"]))*1.1)
  root.visible=hero_playing and here.distance_squared_to(root.global_position)<40.0*40.0
 var current:int=_depth_near_corruption() if hero_playing and not death_active else -1
 if current!=depth_corruption_here:
  depth_corruption_here=current
  if current>=0 and not menu_open:
   _show_message("LUNAR CORRUPTION: enemies strike harder. Moon Nova echoes here. No countdown.",3.5)

func _process(delta:float)->void:
 super._process(delta)
 _depth_tick(delta)
 _depth_surge_tick(delta)

func _interact()->void:
 if hero_playing and not death_active and not menu_open and not mounted and is_instance_valid(player):
  for item in depth_glyphs:
   var root:Node3D=item["node"]
   if player.global_position.distance_squared_to(root.global_position)>DEPTH_INTERACT_RADIUS_SQ:continue
   var site:int=int(item["site"])
   var stage:int=int(item["stage"])
   var key:String=str(site)+":"+str(stage)
   if bool(rifts[site].get("cleared",false)) or active_rift!=site or int(rifts[site].get("stage",0))!=stage:
    _show_message("The glyph awakens during its matching rift chamber.",2.0)
    return
   if depth_glyph_charged.has(key):
    _show_message("This moon glyph is already attuned.",1.5)
    return
   depth_glyph_charged[key]=true
   depth_spell_charges=mini(3,depth_spell_charges+1)
   _show_message("GLYPH ATTUNED! Next Moon Nova gains a lunar echo ("+str(depth_spell_charges)+" charges).",3.0)
   return
  for item in depth_travelers:
   var root:Node3D=item["node"]
   if player.global_position.distance_squared_to(root.global_position)>DEPTH_INTERACT_RADIUS_SQ:continue
   if _combat_threat():_show_message("The scribe will speak when the danger passes.",2.0)
   else:_show_message("Moon Scribe: "+str(item["lore"]),5.0)
   return
 super._interact()

func _spell(which:String)->void:
 var echo:bool=which=="nova" and hero_playing and not death_active and not mounted and not menu_open and ability_cooldown<=0.0 and (depth_spell_charges>0 or _depth_near_corruption()>=0)
 super._spell(which)
 if not echo:return
 var hits:int=0
 # No new spell keybinding or full-world target scan; use the current region list.
 for beast in zone_beast_lists[region_index]:
  if hits>=3:break
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body")
  if not is_instance_valid(body):continue
  if body.global_position.distance_squared_to(player.global_position)>5.2*5.2:continue
  if not _has_clear_sight(player,body):continue
  _apply_player_hit(beast,8.0+float(hero_level)*0.65,0.08)
  hits+=1
 if depth_spell_charges>0:depth_spell_charges-=1
 if hits>0:_show_message("MOONFLARE ECHO! "+str(hits)+" corrupted enemies struck.",2.0)

func _depth_reset_glyphs()->void:
 depth_glyph_charged.clear()
 depth_spell_charges=0
 depth_corruption_here=-1
 _depth_reset_surge()

func _release_label()->String:return "v33.8 • Living Rifts"



func _depth_build_surges()->void:
 # One shared mesh and material, three small warning surfaces. No physics bodies.
 var mesh:=CylinderMesh.new()
 mesh.top_radius=4.2
 mesh.bottom_radius=4.2
 mesh.height=0.04
 mesh.radial_segments=28
 var warning:=StandardMaterial3D.new()
 warning.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 warning.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 warning.albedo_color=Color(0.95,0.09,0.16,0.60)
 warning.emission_enabled=true
 warning.emission=Color(0.7,0.04,0.12)
 for site in range(rifts.size()):
  var root:=Node3D.new()
  root.name="CorruptionSurge_%d"%site
  root.position=(rifts[site]["anchor"] as Vector3)+Vector3(5.0,0.10,3.0)
  world.add_child(root)
  var surface:=MeshInstance3D.new()
  surface.mesh=mesh
  surface.material_override=warning
  surface.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  root.add_child(surface)
  var message:=Label3D.new()
  message.text="RIFT SURGE • MOVE AWAY"
  message.font_size=30
  message.pixel_size=0.006
  message.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  message.modulate=Color("ffc4cc")
  message.position.y=1.7
  root.add_child(message)
  root.visible=false
  depth_surge_nodes.append({"node":root,"site":site})

func _depth_reset_surge()->void:
 depth_surge_site=-1
 depth_surge_phase=0
 depth_surge_clock=0.0
 for hazard in depth_surge_nodes:
  var root:Node3D=hazard["node"]
  if is_instance_valid(root):root.visible=false

func _depth_surge_tick(delta:float)->void:
 # Only the current rift is considered; O(3) proximity, O(1) damage.
 if not hero_playing or death_active or menu_open or mounted or not is_instance_valid(player):
  if depth_surge_site>=0 or depth_surge_phase!=0:_depth_reset_surge()
  return
 var site:int=_depth_near_corruption()
 if site!=depth_surge_site:
  _depth_reset_surge()
  depth_surge_site=site
 if site<0:return
 depth_surge_clock+=clampf(delta,0.0,0.15)
 if depth_surge_phase==0 and depth_surge_clock>=DEPTH_SURGE_REST:
  depth_surge_phase=1
  depth_surge_clock=0.0
  (depth_surge_nodes[site]["node"] as Node3D).visible=true
  _show_message("RIFT SURGE! Leave the glowing red circle or dodge.",2.0)
 elif depth_surge_phase==1 and depth_surge_clock>=DEPTH_SURGE_TELEGRAPH:
  depth_surge_phase=0
  depth_surge_clock=0.0
  var root:Node3D=depth_surge_nodes[site]["node"]
  root.visible=false
  # A visible fixed location; moving away or dodging completely avoids damage.
  if player.global_position.distance_squared_to(root.global_position)<=DEPTH_SURGE_HIT_RADIUS_SQ:
   if dodge_time>0.0:
    _show_message("SURGE DODGED!",1.0)
   else:
    depth_surge_hits+=1
    _apply_incoming_damage(9.0+float(region_index)*1.2)
    if not death_active:_show_message("CORRUPTION SURGE! Move out of the red circle.",1.6)

func _depth_attunement_count(site:int)->int:
 if site<0 or site>=rifts.size():return 0
 var total:int=0
 for stage in range(1,4):
  if depth_glyph_charged.has(str(site)+":"+str(stage)):total+=1
 return total

func _on_rift_enemy_defeated(beast:Dictionary)->void:
 var site:int=int(beast.get("dungeon_site",-1))
 var eligible:bool=site>=0 and site<rifts.size() and not bool(rifts[site].get("cleared",false)) and _depth_attunement_count(site)==3
 super._on_rift_enemy_defeated(beast)
 # The persistent cleared flag is the anti-farm gate; no additional save fields.
 if eligible and bool(rifts[site].get("cleared",false)):
  _gain_material("Moon Sigil",2)
  _gain_material("Dusk Cinder",1)
  depth_perfect_rifts+=1
  _show_message("PERFECT RIFT! +2 Moon Sigils, +1 Dusk Cinder.",4.0)
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false


func _build_ui()->void:
 super._build_ui()
 # A single persistent overlay, not a new level, game mode, or per-death UI allocation.
 var layer:CanvasLayer=get_node("NativeAtlasUI") as CanvasLayer
 death_overlay=Control.new()
 death_overlay.name="DeathAndResurrection"
 death_overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 death_overlay.mouse_filter=Control.MOUSE_FILTER_STOP
 death_overlay.visible=false
 layer.add_child(death_overlay)
 death_fade=ColorRect.new()
 death_fade.color=Color(0.035,0.015,0.060,0.95)
 death_fade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 death_fade.mouse_filter=Control.MOUSE_FILTER_STOP
 death_overlay.add_child(death_fade)
 var center:=CenterContainer.new()
 center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 center.mouse_filter=Control.MOUSE_FILTER_PASS
 death_overlay.add_child(center)
 var panel:=PanelContainer.new()
 panel.custom_minimum_size=Vector2(560,470)
 var style:=StyleBoxFlat.new()
 style.bg_color=Color("1c101ee8")
 style.border_color=Color("b64455")
 style.set_border_width_all(3)
 style.set_corner_radius_all(14)
 style.content_margin_left=24
 style.content_margin_top=24
 style.content_margin_right=24
 style.content_margin_bottom=24
 panel.add_theme_stylebox_override("panel",style)
 center.add_child(panel)
 var content:=VBoxContainer.new()
 content.add_theme_constant_override("separation",16)
 panel.add_child(content)
 var headline:=Label.new()
 headline.text="YOU HAVE FALLEN"
 headline.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 headline.add_theme_font_size_override("font_size",38)
 headline.add_theme_color_override("font_color",Color("fa7075"))
 content.add_child(headline)
 var subtitle:=Label.new()
 subtitle.text="The moon remembers. Your journey is not over."
 subtitle.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 subtitle.add_theme_font_size_override("font_size",16)
 subtitle.add_theme_color_override("font_color",Color("e0c2db"))
 content.add_child(subtitle)
 death_stats=Label.new()
 death_stats.name="DeathStats"
 death_stats.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 death_stats.add_theme_font_size_override("font_size",20)
 death_stats.add_theme_color_override("font_color",Color("f3e2dc"))
 content.add_child(death_stats)
 var guarantee:=Label.new()
 guarantee.text="Level, equipment, materials, professions and unlocked areas are safe."
 guarantee.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 guarantee.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 guarantee.add_theme_font_size_override("font_size",14)
 guarantee.add_theme_color_override("font_color",Color("abdba9"))
 content.add_child(guarantee)
 death_continue=Button.new()
 death_continue.text="RESURRECT AT LAST SANCTUARY"
 death_continue.custom_minimum_size=Vector2(450,62)
 death_continue.add_theme_font_size_override("font_size",20)
 death_continue.pressed.connect(_death_return_to_sanctuary)
 content.add_child(death_continue)
 var title_button:=Button.new()
 title_button.text="SAVE & RETURN TO HERO SELECT"
 title_button.custom_minimum_size=Vector2(450,48)
 title_button.pressed.connect(_death_return_to_title)
 content.add_child(title_button)

func _start_game(selected:String)->void:
 _clear_rival_mark()
 super._start_game(selected)
 _depth_reset_glyphs()
 death_active=false
 phoenix_used_this_expedition=false
 phoenix_grace_until=0.0
 revenge_target_id=-1
 death_attacker={}
 death_killer_text=""
 if is_instance_valid(death_overlay):death_overlay.hide()
 set_physics_process(true)

func _restore_data(data:Dictionary)->void:
 _clear_rival_mark()
 super._restore_data(data)
 _depth_reset_glyphs()
 death_active=false
 phoenix_used_this_expedition=false
 phoenix_grace_until=0.0
 revenge_target_id=-1
 death_attacker={}
 death_killer_text=""
 if is_instance_valid(death_overlay):death_overlay.hide()
 set_physics_process(true)

func _update_beast_step(beast:Dictionary,delta:float)->void:
 # This AI hook identifies the actual attacker without scanning the world on each hit.
 var previous:Dictionary=death_attacker
 death_attacker=beast
 super._update_beast_step(beast,delta)
 death_attacker=previous

func _apply_incoming_damage(raw:float)->void:
 # Prevent damage from firing repeatedly after a fatal hit, or during a revive grace window.
 if death_active or map_time<phoenix_grace_until:return
 super._apply_incoming_damage(raw*1.08 if _depth_near_corruption()>=0 else raw)

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 var was_alive:bool=bool(beast.get("alive",false))
 var slain_id:int=int(beast.get("boss_id",-1))
 var rival_defeated:bool=was_alive and slain_id>=0 and slain_id==revenge_target_id
 var new_guardian_victory:bool=was_alive and bool(beast.get("boss",false)) and not (slain_id in defeated_boss_ids)
 super._defeat_beast(beast,pet_kill)
 if rival_defeated:
  _clear_rival_mark()
  revenge_target_id=-1
  gold+=40+int(beast.get("zone",0))*10
  _gain_material("Moon Sigil",1)
  _show_message("VENGEANCE! Your marked rival dropped bonus coins and a Moon Sigil.",4.0)
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false
 if new_guardian_victory and rng.randf()<PHOENIX_DROP_CHANCE and int(materials.get(PHOENIX_FEATHER,0))<1:
  _gain_material(PHOENIX_FEATHER,1)
  _show_message("RARE DROP: Moon Phoenix Feather! One emergency revival.",4.0)
  # The parent can checkpoint before the reward is added; persist the feather here.
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false

func _respawn_player()->void:
 if death_active:return
 # A rare, consumed resource allows one immediate rescue per expedition.
 if not phoenix_used_this_expedition and int(materials.get(PHOENIX_FEATHER,0))>0:
  phoenix_used_this_expedition=true
  materials[PHOENIX_FEATHER]=int(materials[PHOENIX_FEATHER])-1
  # Permanent Moonward Oath strengthens a consumed feather, not an extra life.
  var oath_active:bool=int(materials.get(MOON_OATH,0))>0
  hp=maxf(1.0,max_hp*(0.45 if oath_active else 0.30))
  phoenix_grace_until=map_time+(3.0 if oath_active else PHOENIX_GRACE_SECONDS)
  dodge_time=0.0
  combat_combo=0
  mounted=false
  _show_message("PHOENIX REBIRTH! "+("45%" if oath_active else "30%")+" health, brief protection.",3.0)
  suppress_autosave_message=true
  _save_game() # Prevent a reload from restoring the spent feather.
  suppress_autosave_message=false
  return
 death_active=true
 if not death_attacker.is_empty():
  revenge_target_id=int(death_attacker.get("boss_id",-1))
  death_killer_text=("Guardian " if bool(death_attacker.get("boss",false)) else "")+str(death_attacker.get("variant","normal")).capitalize()+" "+str(death_attacker.get("kind","creature")).capitalize()
 else:
  revenge_target_id=-1
  death_killer_text="Unknown danger"
 hp=0.0
 player.velocity=Vector3.ZERO
 menu_open=true # Existing combat AI and inputs already honor this pause flag.
 if is_instance_valid(menu_panel):menu_panel.hide()
 if is_instance_valid(quick_actions):quick_actions.hide()
 if is_instance_valid(minimap):minimap.hide()
 death_stats.text=_death_stats_text()
 death_fade.color=Color(0.095,0.014,0.10,0.95) if hero=="nyra" else Color(0.065,0.012,0.035,0.95)
 death_overlay.show()
 death_overlay.modulate.a=0.0
 if is_instance_valid(death_overlay_tween):death_overlay_tween.kill()
 death_overlay_tween=create_tween()
 death_overlay_tween.tween_property(death_overlay,"modulate:a",1.0,0.42)
 death_continue.call_deferred("grab_focus")
 # Freeze physics after the fatal hit, including other inherited AI updates.
 set_physics_process(false)

func _death_stats_text()->String:
 var location:String=str(REGIONS[clampi(region_index,0,REGIONS.size()-1)]["name"])
 var lines:PackedStringArray=[
  "Fallen in "+location+"   •   Slain by: "+death_killer_text,
  "Hero: "+("Nyra" if hero=="nyra" else "Z")+"   •   Level "+str(hero_level),
  "Enemies defeated: "+str(kills_total)+"   •   Guardians: "+str(defeated_boss_ids.size()),
  "Rifts sealed: "+str(rifts_cleared.size())+"   •   Coins: "+_wallet_text()
 ]
 return "\n\n".join(lines)

func _death_return_to_sanctuary()->void:
 if not death_active:return
 if is_instance_valid(death_overlay_tween):death_overlay_tween.kill()
 death_overlay.hide()
 death_active=false
 # An unfinished rift attempt is reset, not marked as cleared or rewarded.
 _death_reset_active_rift()
 _depth_reset_glyphs()
 # The old respawn routine already honors the last activated sanctuary.
 super._respawn_player()
 # Restart an unfinished guardian fight, but never erase defeated bosses.
 for creature in beasts:
  if not bool(creature.get("boss",false)) or not bool(creature.get("alive",false)):continue
  if int(creature.get("zone",-1))!=region_index:continue
  if int(creature.get("dungeon_site",-1))>=0:continue # Rift wave state owns these spawns.
  _reset_native_beast(creature)
 _show_rival_mark()
 attack_cooldown=0.0
 ability_cooldown=0.0
 dodge_time=0.0
 dodge_cooldown=0.0
 hunter_oil_time=0.0
 combat_combo=0
 combat_combo_until=0.0
 phoenix_grace_until=map_time+PHOENIX_GRACE_SECONDS
 phoenix_used_this_expedition=false
 menu_open=false
 set_physics_process(true)
 if is_instance_valid(quick_actions):quick_actions.show()
 if is_instance_valid(minimap):minimap.visible=minimap_visible
 _update_hud()
 suppress_autosave_message=true
 _save_game() # Commit safe shrine location and all permanent progress before continuing.
 suppress_autosave_message=false

func _death_reset_active_rift()->void:
 if active_rift<0 or active_rift>=rifts.size():return
 var entry:Dictionary=rifts[active_rift]
 if bool(entry.get("cleared",false)):
  active_rift=-1
  return
 for slot in (entry.get("slots",[]) as Array):
  var index:int=int(slot)
  if index>=0 and index<beasts.size():
   _despawn_native_beast(beasts[index],0.0)
 entry["active"]=false
 entry["stage"]=0
 active_rift=-1
 _set_rift_label(entry)
 _v321_update_chambers()

func _clear_rival_mark()->void:
 if revenge_target_id<0 or revenge_target_id>=beasts.size():return
 var creature:Dictionary=beasts[revenge_target_id]
 var nameplate:Label3D=creature.get("nameplate")
 if is_instance_valid(nameplate) and nameplate.text.begins_with("MARKED RIVAL • "):
  nameplate.text=nameplate.text.trim_prefix("MARKED RIVAL • ")
  nameplate.modulate=Color("f4dcbb") if str(creature.get("variant","normal"))=="normal" else Color(str(REGIONS[int(creature.get("zone",0))]["light"]))

func _show_rival_mark()->void:
 if revenge_target_id<0 or revenge_target_id>=beasts.size():return
 var creature:Dictionary=beasts[revenge_target_id]
 if not bool(creature.get("alive",false)):return
 var nameplate:Label3D=creature.get("nameplate")
 if is_instance_valid(nameplate) and not nameplate.text.begins_with("MARKED RIVAL • "):
  nameplate.text="MARKED RIVAL • "+nameplate.text
  nameplate.modulate=Color("ff7777")

func _death_return_to_title()->void:
 if not death_active:return
 _death_return_to_sanctuary() # Save a safe respawn first; do not leave a fatal checkpoint.
 hero_playing=false
 menu_open=false
 start_screen.show()
 menu_panel.hide()
 if is_instance_valid(quick_actions):quick_actions.hide()
 if is_instance_valid(minimap):minimap.hide()
 _ensure_continue_button()
 if is_instance_valid(hero_start_button):hero_start_button.call_deferred("grab_focus")

func _ensure_continue_button()->void:
 for child in start_screen.get_children():
  if child is Button and str(child.text).begins_with("CONTINUE"):return
 var continue_button:=Button.new()
 continue_button.text="CONTINUE SAVED ADVENTURE"
 continue_button.position=Vector2(145,530)
 continue_button.size=Vector2(460,48)
 continue_button.pressed.connect(_load_from_main_menu)
 start_screen.add_child(continue_button)

func _unhandled_input(event:InputEvent)->void:
 if death_active:
  # Prevent Escape/Start/attack from opening the atlas behind the death menu.
  if event is InputEventKey or event is InputEventJoypadButton or event is InputEventMouseButton:
   get_viewport().set_input_as_handled()
  return
 super._unhandled_input(event)

func _unhandled_key_input(event:InputEvent)->void:
 if death_active:
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

## Moon Sigil crafting: uses saved materials, existing gear bag and existing atlas.
func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="Professions":
  menu_body.text+="\n\n[b]MOON SIGIL FORGE[/b]\nMoon Sigils come from moon creatures and marked-rival victories. Craft legendary gear and a single-use Phoenix Feather, or awaken a permanent Moonward Oath. All ingredients and items persist through death.\n"
  for recipe in MOON_RECIPES:
   var label:String=str(recipe["name"])
   var limit_reached:bool=(str(recipe["kind"])=="oath" and int(materials.get(MOON_OATH,0))>0) or (str(recipe["kind"])=="feather" and int(materials.get(PHOENIX_FEATHER,0))>0)
   menu_body.text+="\n• "+label+" • "+str(recipe["cost"])+(" [OWNED]" if limit_reached else "")
   if not limit_reached:
    _atlas_action("MOON FORGE: "+label,"moon_recipe:"+str(recipe["id"]))
  menu_body.text+="\n\n[b]MOONWARD OATH[/b]\nA permanent blessing: a consumed Phoenix Feather restores 45% health and gives 3 seconds of protection (rather than 30% / 2 seconds). No extra revives.\n"
 elif which=="Inventory" and not inventory_show_vault:
  var matches:Array[int]=INVENTORY_RULES.matching_gear_indices(gear_items,inventory_query,inventory_filter,inventory_sort)
  var first:int=inventory_page*INVENTORY_RULES.BAG_PAGE_SIZE
  var end:int=mini(first+INVENTORY_RULES.BAG_PAGE_SIZE,matches.size())
  for n in range(first,end):
   var index:int=matches[n]
   var item:Dictionary=gear_items[index]
   var rank:int=clampi(int(item.get("moon_enchant_rank",0)),0,3)
   if rank<3 and int(item.get("power",0))<250:
    _atlas_action("MOON ENCHANT: "+str(item.get("name","Gear"))+" ("+str(rank)+"/3)","moon_enchant:"+str(index))
  menu_body.text+="\n\n[b]MOON ENCHANTMENTS[/b]\nEnchantment adds up to +18 power (+6 per rank) to a specific bag item. Cost for the next rank: 2/3/4 Moon Sigils and 1/2/3 Dusk Cinders. Max 3 ranks per item; enchantments persist through death.\n"
 elif which=="Character":
  menu_body.text+="\n\n[b]MOONBOUND RESURRECTION[/b]\nReturn from death at your last sanctuary without losing permanent progression. Guardians can drop a Moon Phoenix Feather ("+str(int(materials.get(PHOENIX_FEATHER,0)))+" held). It prevents one death per expedition; marked-rival revenge grants bonus loot. No survival stopwatch."
  menu_body.text+="\n\n[b]MOONBOUND CRAFTING[/b]\nMoon Sigils: "+str(int(materials.get("Moon Sigil",0)))+"  •  Moonward Oath: "+("AWAKENED" if int(materials.get(MOON_OATH,0))>0 else "Not yet")+". Forge new items under Professions, or enchant bag gear under Inventory."
 elif which=="World":
  menu_body.text+="\n\n[b]LUNAR CORRUPTION & MOON GLYPHS[/b]\nThree rifts leak corruption into the shared world. Inside the violet zone enemies deal +8% damage, but Moon Nova echoes against up to three nearby foes. No countdown.\nAttune the moon glyph in each active rift chamber with G for up to three stored Nova echoes. Glyphs reset on death. Attune all three before sealing a rift for a ONE-TIME bonus of 2 Moon Sigils and 1 Dusk Cinder (never repeatable).\nMoon Scribes travel near Witchwood, Highcrest and Eclipse Citadel. Talk to them with G for region lore. No base building or construction system."
  menu_body.text+="\n\n[b]LIVING RIFTS[/b]\nThe red surge circle warns before it strikes. Step outside or use your dodge. No time limit.\n"
  for site in range(rifts.size()):
   var status:String="SEALED" if bool(rifts[site].get("cleared",false)) else str(_depth_attunement_count(site))+"/3 glyphs"
   menu_body.text+="\n• "+str(rifts[site]["name"])+": "+status


func _atlas_execute(action:String)->void:
 if action.begins_with("moon_recipe:"):
  _moon_craft(action.get_slice(":",1))
  _menu_tab("Professions")
  return
 if action.begins_with("moon_enchant:"):
  _moon_enchant(int(action.get_slice(":",1)))
  _menu_tab("Inventory")
  return
 super._atlas_execute(action)

func _moon_craft(recipe_id:String)->void:
 if not hero_playing or death_active:return
 if _combat_threat():
  _show_message("Cannot forge during combat.",2.0)
  return
 for entry in MOON_RECIPES:
  var recipe:Dictionary=entry
  if str(recipe["id"])!=recipe_id:continue
  var kind:String=str(recipe["kind"])
  if kind=="oath" and int(materials.get(MOON_OATH,0))>0:
   _show_message("Moonward Oath is already awakened.",2.0)
   return
  if kind=="feather" and int(materials.get(PHOENIX_FEATHER,0))>0:
   _show_message("Already holding a Phoenix Feather.",2.0)
   return
  var issue:String=INVENTORY_RULES.craft_issue(recipe,materials,gear_items.size(),bag_capacity)
  if issue!="":
   _show_message(issue,2.0)
   return
  var result:Dictionary=INVENTORY_RULES.craft_commit(recipe,materials)
  if kind=="gear":
   var gear:Dictionary=result["gear"]
   gear["rarity"]="Legendary"
   gear["moon_enchant_rank"]=0
   gear_items.append(gear)
  elif kind=="feather":
   INVENTORY_RULES.add_material(materials,PHOENIX_FEATHER,1)
  else:
   materials[MOON_OATH]=1 # One-time, permanent and saved as a material flag.
  crafting_skill=mini(300,crafting_skill+1)
  _save_game() # Materials, gear and the oath already use checkpoint schema 2.
  _show_message("Moon forge: "+str(recipe["name"])+" crafted!",2.5)
  return

func _moon_enchant(index:int)->void:
 if not hero_playing or death_active or index<0 or index>=gear_items.size():return
 if _combat_threat():
  _show_message("Cannot enchant during combat.",2.0)
  return
 var old:Dictionary=gear_items[index]
 var rank:int=clampi(int(old.get("moon_enchant_rank",0)),0,3)
 if rank>=3 or int(old.get("power",0))>=250:
  _show_message("This item's moon enchantment is at its limit.",2.0)
  return
 var sigils:int=2+rank
 var cinders:int=1+rank
 if int(materials.get("Moon Sigil",0))<sigils or int(materials.get("Dusk Cinder",0))<cinders:
  _show_message("Needs "+str(sigils)+" Moon Sigils and "+str(cinders)+" Dusk Cinders.",2.5)
  return
 materials["Moon Sigil"]=int(materials["Moon Sigil"])-sigils
 materials["Dusk Cinder"]=int(materials["Dusk Cinder"])-cinders
 var enhanced:Dictionary=old.duplicate(true)
 enhanced["moon_enchant_rank"]=rank+1
 enhanced["power"]=mini(250,int(old.get("power",0))+6)
 gear_items[index]=enhanced
 # Equip is a saved copy; update it only if the player enchanted that same item.
 if equipped==old:equipped=enhanced.duplicate(true)
 crafting_skill=mini(300,crafting_skill+1)
 _save_game()
 _show_message("Moon enchant +"+str(rank+1)+" on "+str(enhanced.get("name","Gear"))+"!",2.5)
