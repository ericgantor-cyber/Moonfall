extends "res://scripts/native_v37_7.gd"
## v38 practical overhaul: on-demand concise status and ocean safety feedback,
## leaving the combat/loot/world/save systems untouched and inheritance intact.
const V380_RULES:=preload("res://scripts/systems/overhaul_v380.gd")
const V383_BENCHMARK:=preload("res://scripts/systems/combat_benchmark_v383.gd")
var v380_status_enabled:bool=false
var v380_status_panel:PanelContainer
var v380_status_text:Label
var v380_update_elapsed:float=0.0
var v380_status_updates:int=0
var v383_fps_samples:Array[int]=[]
var v383_peak_hostiles:int=0

func _ready()->void:
 super._ready()
 _v380_build_status_panel()

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
 v380_status_panel.offset_bottom=125.0
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
  if v380_status_enabled:
   v383_fps_samples.clear()
   v383_peak_hostiles=0
  v380_update_elapsed=V380_RULES.UPDATE_PERIOD
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
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
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 var hostile_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for creature in zone_beast_lists[region_index]:
   if bool(creature.get("alive",false)) and bool(creature.get("hostile",false)):hostile_count+=1
 v383_peak_hostiles=maxi(v383_peak_hostiles,hostile_count)
 v380_status_text.text=V380_RULES.safe_label(region,hp,max_hp,v372_oxygen,underwater,v373_depth,current_threat)
 v380_status_text.text+="\nFPS: "+str(fps_now)+"  •  10s avg/min: "+str(summary["average"])+"/"+str(summary["min"])+"  •  Enemies: "+str(hostile_count)
 v380_status_text.add_theme_color_override("font_color",Color("ffb5a9") if V380_RULES.air_urgency(v372_oxygen,underwater)>0 else Color("cce9f5"))
 v380_status_updates+=1

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="System":
  menu_body.text+="\n\n[b]v38 LIVE STATUS[/b]\nF2 toggles a compact status readout showing your region, HP, underwater depth and oxygen. It updates only four times per second and stays hidden by default. When enabled, it also shows current FPS, rolling 10-second average/minimum FPS and nearby enemy count. F9's performance report includes sample and peak enemy details."

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
 var active_hostiles:int=0
 var damage_number_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for beast in zone_beast_lists[region_index]:
   if not bool(beast.get("alive",false)):continue
   if bool(beast.get("hostile",false)):active_hostiles+=1
   var body:CharacterBody3D=beast.get("body") as CharacterBody3D
   if not is_instance_valid(body):continue
   for child in body.get_children():
    if child is Label3D and child.name.begins_with("DamageFloat"):
     if not child.is_queued_for_deletion():damage_number_count+=1
 report["combat_active_hostiles"]=active_hostiles
 report["combat_damage_floaters"]=damage_number_count
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 report["combat_sample_count"]=int(summary["count"])
 report["combat_sample_avg_fps"]=int(summary["average"])
 report["combat_sample_min_fps"]=int(summary["min"])
 report["combat_sample_max_fps"]=int(summary["max"])
 report["combat_peak_hostiles"]=v383_peak_hostiles
 return report
