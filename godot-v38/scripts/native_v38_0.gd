extends "res://scripts/native_v37_7.gd"
## v38 practical overhaul: on-demand concise status and ocean safety feedback,
## leaving the combat/loot/world/save systems untouched and inheritance intact.
const V380_RULES:=preload("res://scripts/systems/overhaul_v380.gd")
const V383_BENCHMARK:=preload("res://scripts/systems/combat_benchmark_v383.gd")
const V384_RINGS:=preload("res://scripts/systems/vitals_rings_v384.gd")
var v380_status_enabled:bool=false
var v380_status_panel:PanelContainer
var v380_status_text:Label
var v380_update_elapsed:float=0.0
var v380_status_updates:int=0
var v383_fps_samples:Array[int]=[]
var v383_combat_fps_samples:Array[int]=[]
var v383_peak_hostiles:int=0
var v384_vitals_panel:PanelContainer
var v384_vitals_label:RichTextLabel
var v384_rings:Control
var v386_orb_image:TextureRect
var v386_orb_values:Label
var v384_vitals_elapsed:float=0.0
var v385_mana:float=100.0
const V385_MAX_MANA:float=100.0
const V385_MANA_REGEN:float=7.0
const V387_MAX_STAMINA:float=150.0
const V387_STAMINA_REGEN:float=18.0
const V387_DODGE_COST:float=28.0
var v387_stamina:float=V387_MAX_STAMINA
var v387_stamina_bar:ProgressBar
var v387_fallback_stamina_bar:ProgressBar
var v387_stamina_warning:Label
var v387_stamina_warning_elapsed:float=0.0
var v387_stamina_regen_wait:float=0.0
const V387_STAMINA_REGEN_DELAY:float=0.65

func _ready()->void:
 super._ready()
 _v380_build_status_panel()
 _v384_build_vitals()

func _build_ui()->void:
 super._build_ui()
 if not is_instance_valid(start_screen):return
 for node in start_screen.find_children("*","Label",true,false):
  var title:Label=node as Label
  if is_instance_valid(title) and "MOONFALL v37.7" in title.text:
   title.text="☾  MOONFALL v38.0 • THE LIVING REALM"

func _v380_build_status_panel()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v380_status_panel=PanelContainer.new()
 v380_status_panel.name="V380_OptionalLiveStatus"
 v380_status_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 v380_status_panel.offset_left=-370.0
 v380_status_panel.offset_right=-15.0
 v380_status_panel.offset_top=20.0
 v380_status_panel.offset_bottom=150.0
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

func _v384_build_vitals()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v384_vitals_panel=PanelContainer.new()
 v384_vitals_panel.name="V384_Vitals"
 v384_vitals_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
 v384_vitals_panel.offset_left=16.0
 v384_vitals_panel.offset_right=435.0
 v384_vitals_panel.offset_top=-135.0
 v384_vitals_panel.offset_bottom=-14.0
 v384_vitals_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var backdrop:=StyleBoxFlat.new()
 backdrop.bg_color=Color(0.045,0.035,0.09,0.86)
 backdrop.border_color=Color(0.47,0.34,0.58,0.9)
 backdrop.set_border_width_all(1)
 backdrop.set_corner_radius_all(12)
 backdrop.set_content_margin_all(9)
 v384_vitals_panel.add_theme_stylebox_override("panel",backdrop)
 layer.add_child(v384_vitals_panel)
 var layout:=HBoxContainer.new()
 layout.add_theme_constant_override("separation",12)
 v384_vitals_panel.add_child(layout)
 v384_rings=V384_RINGS.new()
 v384_rings.name="V384_HP_XP_Rings"
 layout.add_child(v384_rings)
 v384_vitals_label=RichTextLabel.new()
 v384_vitals_label.name="V384_VitalsText"
 v384_vitals_label.bbcode_enabled=true
 v384_vitals_label.scroll_active=false
 v384_vitals_label.mouse_filter=Control.MOUSE_FILTER_IGNORE
 v384_vitals_label.add_theme_font_size_override("normal_font_size",16)
 v384_vitals_label.custom_minimum_size=Vector2(205.0,96.0)
 layout.add_child(v384_vitals_label)
 # Functional green stamina meter even before the custom art PNG is installed.
 var fallback_stamina:=ProgressBar.new()
 fallback_stamina.name="V387_FallbackStaminaBar"
 fallback_stamina.min_value=0.0
 fallback_stamina.max_value=V387_MAX_STAMINA
 fallback_stamina.value=v387_stamina
 fallback_stamina.show_percentage=false
 fallback_stamina.custom_minimum_size=Vector2(110.0,14.0)
 fallback_stamina.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var fallback_fill:=StyleBoxFlat.new()
 fallback_fill.bg_color=Color("#69e26b")
 fallback_fill.set_corner_radius_all(4)
 fallback_stamina.add_theme_stylebox_override("fill",fallback_fill)
 layout.add_child(fallback_stamina)
 v387_fallback_stamina_bar=fallback_stamina
 v387_stamina_warning=Label.new()
 v387_stamina_warning.name="V387_LowStaminaWarning"
 v387_stamina_warning.text="LOW STAMINA"
 v387_stamina_warning.add_theme_font_size_override("font_size",12)
 v387_stamina_warning.add_theme_color_override("font_color",Color("#ffd16e"))
 v387_stamina_warning.visible=false
 layout.add_child(v387_stamina_warning)
 v384_vitals_panel.visible=false
 # Use the user's HUD PNG when present; fall back to functional meters otherwise.
 var four_color_art:String="res://assets/ui/moonfall_four_resource_hud.png"
 var original_orb_art:String="res://assets/ui/moonfall_orb_hud.png"
 var selected_hud_art:String=four_color_art if ResourceLoader.exists(four_color_art) else original_orb_art
 if ResourceLoader.exists(selected_hud_art):
  # Keep the fallback alive but hidden so regressions and asset reloads stay safe.
  v384_vitals_panel.visible=false
  var plain_vitals:PanelContainer=v384_vitals_panel
  v384_vitals_panel=PanelContainer.new()
  v384_vitals_panel.name="V386_OriginalOrbHUD"
  v384_vitals_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
  v384_vitals_panel.offset_left=12.0
  v384_vitals_panel.offset_right=254.0
  v384_vitals_panel.offset_top=-335.0
  v384_vitals_panel.offset_bottom=-12.0
  v384_vitals_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
  var transparent_panel:=StyleBoxEmpty.new()
  v384_vitals_panel.add_theme_stylebox_override("panel",transparent_panel)
  var overlay:=Control.new()
  overlay.custom_minimum_size=Vector2(242,323)
  overlay.mouse_filter=Control.MOUSE_FILTER_IGNORE
  v384_vitals_panel.add_child(overlay)
  v386_orb_image=TextureRect.new()
  v386_orb_image.set_texture(load(selected_hud_art))
  v386_orb_image.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
  v386_orb_image.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
  v386_orb_image.mouse_filter=Control.MOUSE_FILTER_IGNORE
  overlay.add_child(v386_orb_image)
  v386_orb_image.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
  v386_orb_values=Label.new()
  v386_orb_values.mouse_filter=Control.MOUSE_FILTER_IGNORE
  overlay.add_child(v386_orb_values)
  v387_stamina_bar=ProgressBar.new()
  v387_stamina_bar.name="V387_StaminaBar"
  v387_stamina_bar.min_value=0.0
  v387_stamina_bar.max_value=V387_MAX_STAMINA
  v387_stamina_bar.value=v387_stamina
  v387_stamina_bar.show_percentage=false
  v387_stamina_bar.mouse_filter=Control.MOUSE_FILTER_IGNORE
  var stamina_fill:=StyleBoxFlat.new()
  stamina_fill.bg_color=Color("#69e26b")
  stamina_fill.set_corner_radius_all(5)
  v387_stamina_bar.add_theme_stylebox_override("fill",stamina_fill)
  var stamina_track:=StyleBoxFlat.new()
  stamina_track.bg_color=Color(0.04,0.14,0.06,0.85)
  stamina_track.set_corner_radius_all(5)
  v387_stamina_bar.add_theme_stylebox_override("background",stamina_track)
  overlay.add_child(v387_stamina_bar)
  v387_stamina_bar.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
  v387_stamina_bar.offset_left=24.0
  v387_stamina_bar.offset_right=-24.0
  v387_stamina_bar.offset_top=-48.0
  v387_stamina_bar.offset_bottom=-36.0
  # Keep the low-resource cue available alongside the artwork, without a HUD box.
  var low_stam_hint:=Label.new()
  low_stam_hint.name="V387_OrbLowStamina"
  low_stam_hint.text="LOW STAMINA"
  low_stam_hint.add_theme_font_size_override("font_size",12)
  low_stam_hint.add_theme_color_override("font_color",Color("#ffd16e"))
  overlay.add_child(low_stam_hint)
  low_stam_hint.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
  low_stam_hint.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
  low_stam_hint.offset_top=-70.0
  low_stam_hint.offset_bottom=-50.0
  low_stam_hint.visible=false
  v386_orb_values.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
  v386_orb_values.offset_left=6.0
  v386_orb_values.offset_right=237.0
  v386_orb_values.offset_top=-26.0
  v386_orb_values.offset_bottom=-4.0
  v386_orb_values.add_theme_font_size_override("font_size",15)
  v386_orb_values.add_theme_color_override("font_color",Color.WHITE)
  v386_orb_values.add_theme_color_override("font_shadow_color",Color(0.02,0.01,0.07,0.98))
  v386_orb_values.add_theme_constant_override("shadow_offset_x",1)
  v386_orb_values.add_theme_constant_override("shadow_offset_y",2)
  v386_orb_values.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
  layer.add_child(v384_vitals_panel)
  v384_vitals_panel.visible=false
  plain_vitals.name="V384_BackupVitals"


func _spell(which:String)->void:
 if mounted or ability_cooldown>0.0:
  super._spell(which)
  return
 var cost:float=15.0 if which=="dash" else 25.0 if which=="nova" else 30.0
 if v385_mana<cost:
  _show_message("Not enough mana! Need "+str(roundi(cost))+".",1.3)
  return
 v385_mana-=cost
 super._spell(which)

func _dodge()->void:
 if mounted or dodge_cooldown>0.0:
  super._dodge()
  return
 if v387_stamina<V387_DODGE_COST:
  _show_message("Not enough stamina to dodge.",1.0)
  return
 # Only charge stamina when a dodge can actually begin.
 if not hero_playing or menu_open or death_active:return
 v387_stamina-=V387_DODGE_COST
 v387_stamina_regen_wait=V387_STAMINA_REGEN_DELAY
 super._dodge()

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 v385_mana=V385_MAX_MANA
 v387_stamina=V387_MAX_STAMINA
 v387_stamina_regen_wait=0.0

func _checkpoint_extra_data()->Dictionary:
 return {"mana":clampf(v385_mana,0.0,V385_MAX_MANA),"stamina":clampf(v387_stamina,0.0,V387_MAX_STAMINA)}

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 v387_stamina_regen_wait=0.0
 var resources:Variant=data.get("optional_resources",{})
 # Legacy checkpoints without a mana field continue to load at full mana.
 if resources is Dictionary:
  v385_mana=clampf(float(resources.get("mana",V385_MAX_MANA)),0.0,V385_MAX_MANA)
  v387_stamina=clampf(float(resources.get("stamina",V387_MAX_STAMINA)),0.0,V387_MAX_STAMINA)
 else:
  v385_mana=V385_MAX_MANA
  v387_stamina=V387_MAX_STAMINA

func _v384_refresh_vitals()->void:
 if is_instance_valid(v387_stamina_bar):
  v387_stamina_bar.value=clampf(v387_stamina,0.0,V387_MAX_STAMINA)
  var orb_hint:Label=v387_stamina_bar.get_parent().get_node_or_null("V387_OrbLowStamina") as Label
  if is_instance_valid(orb_hint):orb_hint.visible=v387_stamina<V387_DODGE_COST
 if is_instance_valid(v387_fallback_stamina_bar):v387_fallback_stamina_bar.value=clampf(v387_stamina,0.0,V387_MAX_STAMINA)
 if is_instance_valid(v387_stamina_warning):v387_stamina_warning.visible=v387_stamina<V387_DODGE_COST
 if is_instance_valid(v386_orb_values):
  var hp_value:int=clampi(roundi(hp),0,maxi(1,roundi(max_hp)))
  var mana_value:int=clampi(roundi(v385_mana),0,roundi(V385_MAX_MANA))
  var xp_value:int=clampi(hero_xp,0,maxi(1,hero_xp_to_next))
  v386_orb_values.text="♥ %d/%d   ◆ %d/%d   ✦ %d/%d   STAM %d/%d" % [hp_value,roundi(max_hp),mana_value,roundi(V385_MAX_MANA),xp_value,hero_xp_to_next,roundi(v387_stamina),roundi(V387_MAX_STAMINA)]
 if not is_instance_valid(v384_vitals_label):return
 if is_instance_valid(v384_rings):
  v384_rings.call("set_vitals",hp/maxf(1.0,max_hp),float(hero_xp)/float(maxi(1,hero_xp_to_next)))
 var health:int=roundi(maxf(0.0,hp))
 var health_max:int=maxi(1,roundi(max_hp))
 var experience:int=maxi(0,hero_xp)
 var experience_target:int=maxi(1,hero_xp_to_next)
 # This display is backed by the spell-cost resource in v38.5.
 v384_vitals_label.text="[color=#ff555f]● HP   %d / %d[/color]\\n[color=#5caeff]● MANA   %d / %d[/color]\\n[color=#b786ff]● XP   %d / %d[/color]\\n[color=#69e26b]● STAM   %d / %d[/color]" % [health,health_max,roundi(v385_mana),roundi(V385_MAX_MANA),experience,experience_target,roundi(v387_stamina),roundi(V387_MAX_STAMINA)]

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_F2 and hero_playing and not menu_open:
  v380_status_enabled=not v380_status_enabled
  if v380_status_enabled:
   v383_fps_samples.clear()
   v383_combat_fps_samples.clear()
   v383_peak_hostiles=0
  v380_update_elapsed=V380_RULES.UPDATE_PERIOD
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
 if hero_playing and not death_active and not menu_open:
  v385_mana=minf(V385_MAX_MANA,v385_mana+V385_MANA_REGEN*maxf(0.0,delta))
  var stamina_step:float=maxf(0.0,delta)
  if v387_stamina_regen_wait>0.0:
   var delay_used:float=minf(v387_stamina_regen_wait,stamina_step)
   v387_stamina_regen_wait-=delay_used
   stamina_step-=delay_used
  if stamina_step>0.0:v387_stamina=minf(V387_MAX_STAMINA,v387_stamina+V387_STAMINA_REGEN*stamina_step)
 if is_instance_valid(v384_vitals_panel):
  var vitals_visible:bool=hero_playing and not death_active and not menu_open
  v384_vitals_panel.visible=vitals_visible
  if vitals_visible:
   v384_vitals_elapsed+=minf(delta,0.15)
   if v384_vitals_elapsed>=0.25:
    v384_vitals_elapsed=0.0
    _v384_refresh_vitals()
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
 var fps_now:int=maxi(0,Engine.get_frames_per_second())
 V383_BENCHMARK.record_fps(v383_fps_samples,fps_now)
 if current_threat:V383_BENCHMARK.record_fps(v383_combat_fps_samples,fps_now)
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 var combat_summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_combat_fps_samples)
 var hostile_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for creature in zone_beast_lists[region_index]:
   if bool(creature.get("alive",false)) and bool(creature.get("hostile",false)):hostile_count+=1
 v383_peak_hostiles=maxi(v383_peak_hostiles,hostile_count)
 v380_status_text.text=V380_RULES.safe_label(region,hp,max_hp,v372_oxygen,underwater,v373_depth,current_threat)
 v380_status_text.text+="\nFPS "+str(fps_now)+" (avg/min "+str(summary["average"])+"/"+str(summary["min"])+")  •  Foes "+str(hostile_count)
 if int(combat_summary["count"])>0:
  v380_status_text.text+="\nCombat FPS avg/min: "+str(combat_summary["average"])+"/"+str(combat_summary["min"])
 else:
  v380_status_text.text+="\nCombat FPS: no encounter sampled yet"
 v380_status_text.add_theme_color_override("font_color",Color("ffb5a9") if V380_RULES.air_urgency(v372_oxygen,underwater)>0 else Color("cce9f5"))
 v380_status_updates+=1

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="System":
  menu_body.text+="\n\n[b]v38 LIVE STATUS[/b]\nF2 toggles a compact status readout showing your region, HP, underwater depth and oxygen. It updates only four times per second and stays hidden by default. When enabled, it also shows FPS, rolling 10-second average/minimum, hostile count, and encounter-only FPS. F9's performance report includes sample and peak enemy details. Combat samples are recorded only while a nearby threat is active."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["overhaul"]="v38.0 safer swim art pose, optional status and lean handoff"
 report["status_visible"]=v380_status_enabled
 report["status_ui_updates"]=v380_status_updates
 # Collected only on demand when the existing profiler requests a report.
 # Never scan enemy children during normal render/physics frames.
 report["combat_fps"]=Engine.get_frames_per_second()
 report["combat_frame_process_ms"]=Performance.get_monitor(Performance.TIME_PROCESS)*1000.0
 report["combat_frame_physics_ms"]=Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)*1000.0
 # Engine counters help distinguish render load, scene size and physics pressure.
 # Query only when the existing performance report is requested, never every frame.
 report["combat_render_draw_calls"]=int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
 report["combat_render_primitives"]=int(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
 report["combat_scene_nodes"]=int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
 report["combat_physics_collision_pairs"]=int(Performance.get_monitor(Performance.PHYSICS_3D_COLLISION_PAIRS))
 var active_hostiles:int=0
 var damage_number_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for beast in zone_beast_lists[region_index]:
   if not bool(beast.get("alive",false)):continue
   if bool(beast.get("hostile",false)):active_hostiles+=1
   var body:CharacterBody3D=beast.get("body") as CharacterBody3D
   if not is_instance_valid(body):continue
   for child in body.get_children():
    if child is Label3D and (child.has_meta("damage_float_tween") or child.name.begins_with("DamageFloat")):
     if not child.is_queued_for_deletion():damage_number_count+=1
 report["combat_active_hostiles"]=active_hostiles
 report["combat_damage_floaters"]=damage_number_count
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 report["combat_sample_count"]=int(summary["count"])
 report["combat_sample_avg_fps"]=int(summary["average"])
 report["combat_sample_min_fps"]=int(summary["min"])
 report["combat_sample_max_fps"]=int(summary["max"])
 report["combat_peak_hostiles"]=v383_peak_hostiles
 var combat_summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_combat_fps_samples)
 report["combat_encounter_sample_count"]=int(combat_summary["count"])
 report["combat_encounter_avg_fps"]=int(combat_summary["average"])
 report["combat_encounter_min_fps"]=int(combat_summary["min"])
 return report
