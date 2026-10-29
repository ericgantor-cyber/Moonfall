extends "res://scripts/native_v37_7.gd"
## v38 practical overhaul: on-demand concise status and ocean safety feedback,
## leaving the combat/loot/world/save systems untouched and inheritance intact.
const V380_RULES:=preload("res://scripts/systems/overhaul_v380.gd")
var v380_status_enabled:bool=false
var v380_status_panel:PanelContainer
var v380_status_text:Label
var v380_update_elapsed:float=0.0
var v380_status_updates:int=0

func _ready()->void:
 super._ready()
 _v380_build_status_panel()

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

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["overhaul"]="v38.0 safer swim art pose, optional status and lean handoff"
 report["status_visible"]=v380_status_enabled
 report["status_ui_updates"]=v380_status_updates
 return report
