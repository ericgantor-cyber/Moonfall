extends "res://scripts/native_v37_7.gd"
## v38 practical overhaul: on-demand concise status and ocean safety feedback,
## leaving the combat/loot/world/save systems untouched and inheritance intact.
const V380_RULES:=preload("res://scripts/systems/overhaul_v380.gd")
const V383_WORLD:=preload("res://scripts/systems/world_fusion_rules.gd")
const V384_WITCHWOOD:=preload("res://scripts/systems/witchwood_cinematic_v384.gd")
var v384_slice:Dictionary={}
var v384_hero_fill:OmniLight3D
var v384_visual_tick:float=0.0
var v383_event_rewards:int=0
const V382_Z_RIG:=preload("res://scenes/rigs/z_rig.tscn")
const V382_NYRA_RIG:=preload("res://scenes/rigs/nyra_rig.tscn")
var v382_rigs:Dictionary={}
var v382_active_rig:Node3D
var v382_action_until:float=0.0
var v382_animation_events:int=0
var v380_status_enabled:bool=false
var v380_status_panel:PanelContainer
var v380_status_text:Label
var v380_update_elapsed:float=0.0
var v380_status_updates:int=0

func _ready()->void:
 super._ready()
 _v380_build_status_panel()
 _v381_apply_atmosphere()
 _v382_build_rigs()
 _v384_build_witchwood()

func _build_ui()->void:
 super._build_ui()
 if not is_instance_valid(start_screen):return
 # Always show the active project release, not a legacy v37/v38 hard-coded name.
 var release:String=str(_release_label())
 for node in start_screen.find_children("*","Label",true,false):
  var title:Label=node as Label
  if is_instance_valid(title) and "MOONFALL" in title.text.to_upper():
   title.text="☾  MOONFALL "+release+" • THE LIVING REALM"
   break

func _v380_build_status_panel()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v380_status_panel=PanelContainer.new()
 v380_status_panel.name="V380_OptionalLiveStatus"
 v380_status_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 v380_status_panel.offset_left=-370.0
 v380_status_panel.offset_right=-15.0
 v380_status_panel.offset_top=20.0
 v380_status_panel.offset_bottom=100.0
 v380_status_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var background:=StyleBoxFlat.new()
 background.bg_color=Color(0.03,0.09,0.16,0.86)
 background.border_color=Color(0.31,0.67,0.84,0.9)
 background.set_border_width_all(1)
 background.set_corner_radius_all(8)
 background.content_margin_left=10.0
 background.content_margin_right=10.0
 background.content_margin_top=7.0
 background.content_margin_bottom=7.0
 v380_status_panel.add_theme_stylebox_override("panel",background)
 layer.add_child(v380_status_panel)
 v380_status_text=Label.new()
 v380_status_text.name="V380_AccessibleReadout"
 v380_status_text.add_theme_font_size_override("font_size",13)
 v380_status_text.add_theme_color_override("font_color",Color("cce9f5"))
 v380_status_text.mouse_filter=Control.MOUSE_FILTER_IGNORE
 v380_status_panel.add_child(v380_status_text)
 v380_status_panel.visible=false

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_F2 and hero_playing and not menu_open:
  v380_status_enabled=not v380_status_enabled
  v380_update_elapsed=V380_RULES.UPDATE_PERIOD
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
 _v382_tick_rig(delta)
 v384_visual_tick+=minf(delta,0.15)
 if v384_visual_tick>=0.25:
  v384_visual_tick=0.0
  _v384_update_hero_light()
 if not is_instance_valid(v380_status_panel):return
 var visible_now:bool=v380_status_enabled and hero_playing and not death_active and not menu_open
 v380_status_panel.visible=visible_now
 if not visible_now:return
 v380_update_elapsed+=minf(delta,0.15)
 if v380_update_elapsed<V380_RULES.UPDATE_PERIOD:return
 v380_update_elapsed=0.0
 var region:String="Unknown"
 if region_index>=0 and region_index<REGIONS.size():region=str(REGIONS[region_index]["name"])
 var underwater:bool=V373_RULES.truly_submerged(v373_depth)
 v380_status_text.text=V380_RULES.safe_label(region,hp,max_hp,v372_oxygen,underwater,v373_depth,current_threat)
 v380_status_text.add_theme_color_override("font_color",Color("ffb5a9") if V380_RULES.air_urgency(v372_oxygen,underwater)>0 else Color("cce9f5"))
 v380_status_updates+=1

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="System":
  menu_body.text+="\n\n[b]v38 LIVE STATUS[/b]\nF2 toggles a compact status readout showing your region, HP, underwater depth and oxygen. It updates only four times per second and stays hidden by default."
 if which=="World":
  var levels:Array=[]
  for zone in range(REGIONS.size()):levels.append(_recommended_power(zone))
  var mission:Dictionary=V383_WORLD.next_objective(REGIONS,levels,events_completed,rifts.size()-rifts_cleared.size(),_current_power())
  menu_body.text+="\n\n"+V383_WORLD.guidance(mission,_current_power(),_recommended_power(region_index))
  _atlas_action("OPEN PROFESSIONS & CRAFTING","v383:professions","Use your existing shared material inventory")
  _atlas_action("OPEN HERO SPECIALIZATIONS","v383:skills","Spend your existing hero level points")
  _atlas_action("OPEN EQUIPMENT BAG","v383:gear","Equip rewards from hunts, rifts and bosses")


# Dark-fantasy atmosphere builds on the existing PBR, day/night and quality
# systems. No character textures, collision, enemy cues or save data are changed.
# Recompute from the parent lighting each time, never compound adjustments.
func _v381_apply_atmosphere()->void:
 if not gfx_ready or living_environment==null:return
 var grade:String=str(gfx_settings.get("realism_grade","Natural"))
 var cinematic:bool=grade=="Cinematic"
 var natural:bool=grade=="Natural"
 # Preserve the old appearance when Classic is selected.
 if cinematic:
  living_environment.adjustment_saturation=0.84
  living_environment.tonemap_exposure=0.83
 elif natural:
  living_environment.adjustment_saturation=0.93
 if living_environment.fog_enabled and (cinematic or natural):
  living_environment.fog_density=minf(0.018,living_environment.fog_density*(1.18 if cinematic else 1.04))
  living_environment.fog_light_color=living_environment.fog_light_color.lerp(Color("555b70"),0.25 if cinematic else 0.06)
 # Volumetric fog is deliberately high-end only, and never created on headless CI.
 var high_quality:bool=int(gfx_settings.get("environment_detail","7"))>=8
 var enable_volume:bool=cinematic and high_quality and living_environment.fog_enabled and str(gfx_settings.get("shadow_quality","Off"))!="Off" and DisplayServer.get_name()!="headless"
 living_environment.volumetric_fog_enabled=enable_volume
 if enable_volume:
  living_environment.volumetric_fog_density=0.025
  living_environment.volumetric_fog_albedo=Color("8991a5")
  living_environment.volumetric_fog_anisotropy=0.4
 var moon:=get_node_or_null("TwinMoonlight") as DirectionalLight3D
 if is_instance_valid(moon) and cinematic:
  var night:bool="night" in _day_phase().to_lower() or "dusk" in _day_phase().to_lower()
  moon.light_color=moon.light_color.lerp(Color("aabbe5") if night else Color("e7d1ca"),0.24)
  moon.light_energy*=0.94 if night else 0.98

func _refresh_world_lighting()->void:
 super._refresh_world_lighting()
 _v381_apply_atmosphere()

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 _v381_apply_atmosphere()
 _v384_apply_quality()


# Art-safe character animation: the existing Sprite3D stays a whole original
# texture. Rigs animate a separate parent, with 17 virtual bones and VFX sockets.
func _v382_build_rigs()->void:
 if not is_instance_valid(v374_swim_pose_anchor) or not is_instance_valid(hero_sprite):return
 for rig_id in ["z","nyra"]:
  var packed:PackedScene=V382_NYRA_RIG if rig_id=="nyra" else V382_Z_RIG
  var rig:Node3D=packed.instantiate()
  v374_swim_pose_anchor.add_child(rig)
  rig.visible=false
  v382_rigs[rig_id]=rig
 _v382_switch_rig("z")

func _v382_switch_rig(selected:String)->void:
 var chosen:String="nyra" if selected=="nyra" else "z"
 if not v382_rigs.has(chosen) or not is_instance_valid(hero_sprite):return
 for entry in v382_rigs.values():
  var rig:Node3D=entry as Node3D
  rig.call("reset_pose")
  rig.visible=rig==v382_rigs[chosen]
 v382_active_rig=v382_rigs[chosen] as Node3D
 var art_parent:Node3D=v382_active_rig.call("get_art_pivot") as Node3D
 if is_instance_valid(art_parent) and hero_sprite.get_parent()!=art_parent:
  # Identity rig pivots preserve the sprite's existing world pose and texture.
  hero_sprite.reparent(art_parent,true)
 v382_action_until=0.0
 v382_active_rig.call("play_state","idle")

func _start_game(selected:String)->void:
 super._start_game(selected)
 _v382_switch_rig(hero)

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 _v382_switch_rig(hero)

func _v382_action(name:String,hold:float)->void:
 if not is_instance_valid(v382_active_rig):return
 v382_action_until=map_time+hold
 v382_active_rig.call("play_state",name)
 v382_animation_events+=1

func _attack()->void:
 var ready:bool=hero_playing and not menu_open and not mounted and attack_cooldown<=0.0
 super._attack()
 if ready and not death_active:_v382_action("attack",0.32)

func _spell(which:String)->void:
 var ready:bool=hero_playing and not menu_open and not mounted and ability_cooldown<=0.0
 super._spell(which)
 if ready and not death_active:_v382_action("dash" if which=="dash" else "cast",0.52 if which!="dash" else 0.5)

func _dodge()->void:
 var ready:bool=hero_playing and not menu_open and not mounted and dodge_cooldown<=0.0
 super._dodge()
 if ready and not death_active:_v382_action("dash",0.5)

func _apply_incoming_damage(raw:float)->void:
 var previous:float=hp
 super._apply_incoming_damage(raw)
 if hero_playing and hp<previous and not death_active:_v382_action("hurt",0.30)

func _v382_tick_rig(delta:float)->void:
 if not is_instance_valid(v382_active_rig):return
 var desired:String="idle"
 if death_active:desired="death"
 elif hero_playing and not menu_open:
  if map_time<v382_action_until:
   v382_active_rig.call("step_bones",delta)
   return
  if v37_swimming:desired="swim"
  elif not mounted and is_instance_valid(player):
   var speed_squared:float=player.velocity.length_squared()
   if speed_squared>25.0:desired="run"
   elif speed_squared>0.15:desired="walk"
 v382_active_rig.call("play_state",desired)
 v382_active_rig.call("step_bones",delta)

func _atlas_execute(action:String)->void:
 if action.begins_with("v383:"):
  var destination:String=""
  match action:
   "v383:professions":destination="Professions"
   "v383:skills":destination="Hero Skills"
   "v383:gear":destination="Inventory"
  if destination!="":
   _menu_tab(destination)
   return
 super._atlas_execute(action)

func _complete_world_event(zone:int,defer_save:bool=false)->void:
 # The inherited event flag prevents duplicates; award before its save.
 if zone>=0 and zone<world_events.size() and not bool(world_events[zone].get("completed",false)):
  var bonus:Dictionary=V383_WORLD.event_crafting_bonus(zone,water_professions)
  _gain_material(str(bonus["name"]),int(bonus["amount"]))
  v383_event_rewards+=1
 super._complete_world_event(zone,defer_save)

# AAA-inspired Witchwood vertical slice, keeping gameplay and existing hero art.
# Region visibility is managed by the parent six-region world culling.
func _v384_build_witchwood()->void:
 if region_nodes.size()<=1 or not is_instance_valid(player):return
 v384_slice=V384_WITCHWOOD.build(region_nodes[1],REGIONS[1]["at"])
 v384_hero_fill=OmniLight3D.new()
 v384_hero_fill.name="WitchwoodHeroFill"
 v384_hero_fill.light_color=Color("a2c5dc")
 v384_hero_fill.light_energy=0.32
 v384_hero_fill.omni_range=3.7
 v384_hero_fill.position=Vector3(0.0,1.35,0.5)
 v384_hero_fill.shadow_enabled=false
 player.add_child(v384_hero_fill)
 _v384_apply_quality()

func _v384_apply_quality()->void:
 if v384_slice.is_empty() or not gfx_ready:return
 V384_WITCHWOOD.apply_quality(v384_slice,gfx_settings)
 _v384_update_hero_light()

func _v384_update_hero_light()->void:
 if not is_instance_valid(v384_hero_fill):return
 var detail:int=int(gfx_settings.get("environment_detail","7"))
 v384_hero_fill.visible=hero_playing and not death_active and not mounted and region_index==1 and detail>=6 and str(gfx_settings.get("cinematic_lighting","Natural"))!="Classic"

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["overhaul"]="v38.0 safer swim art pose, optional status and lean handoff"
 report["status_visible"]=v380_status_enabled
 report["status_ui_updates"]=v380_status_updates
 report["dark_fantasy_grade"]=str(gfx_settings.get("realism_grade","Natural"))
 report["volumetric_fog"]=living_environment!=null and living_environment.volumetric_fog_enabled
 report["hero_rigs"]=v382_rigs.size()
 report["hero_rig_animation_events"]=v382_animation_events
 report["world_event_crafting_bonuses"]=v383_event_rewards
 report["witchwood_visual_batches"]=8 if not v384_slice.is_empty() else 0
 report["witchwood_hero_fill"]=is_instance_valid(v384_hero_fill) and v384_hero_fill.visible
 return report
