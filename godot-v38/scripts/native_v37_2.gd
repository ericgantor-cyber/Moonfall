extends "res://scripts/native_v37_0.gd"
## v37.2 oxygen for optional shallow dives in six existing lakes.
## No global survival clock, new save format, extra enemy AI or character art.
const V372_OXYGEN:=preload("res://scripts/systems/oxygen_v372.gd")
var v372_oxygen:float=V372_OXYGEN.MAX_OXYGEN
var v372_diving:bool=false
var v372_drowning_elapsed:float=0.0
var v372_low_air_warned:bool=false
var v372_ui_elapsed:float=0.0
var v372_oxygen_panel:PanelContainer
var v372_oxygen_label:Label
var v372_oxygen_bar:ProgressBar

func _ready()->void:
 super._ready()
 _v372_build_oxygen_hud()
 _v372_update_hud()

func _v372_build_oxygen_hud()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v372_oxygen_panel=PanelContainer.new()
 v372_oxygen_panel.name="V372_UnderwaterOxygenHUD"
 v372_oxygen_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
 v372_oxygen_panel.offset_left=18.0
 v372_oxygen_panel.offset_right=292.0
 v372_oxygen_panel.offset_top=-165.0
 v372_oxygen_panel.offset_bottom=-99.0
 v372_oxygen_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var background:=StyleBoxFlat.new()
 background.bg_color=Color(0.045,0.12,0.20,0.88)
 background.border_color=Color(0.22,0.72,0.94,0.9)
 background.set_border_width_all(2)
 background.set_corner_radius_all(9)
 background.content_margin_left=10
 background.content_margin_right=10
 background.content_margin_top=5
 background.content_margin_bottom=7
 v372_oxygen_panel.add_theme_stylebox_override("panel",background)
 layer.add_child(v372_oxygen_panel)
 var stack:=VBoxContainer.new()
 stack.add_theme_constant_override("separation",3)
 v372_oxygen_panel.add_child(stack)
 v372_oxygen_label=Label.new()
 v372_oxygen_label.add_theme_font_size_override("font_size",15)
 v372_oxygen_label.add_theme_color_override("font_color",Color("b2efff"))
 stack.add_child(v372_oxygen_label)
 v372_oxygen_bar=ProgressBar.new()
 v372_oxygen_bar.custom_minimum_size=Vector2(248.0,14.0)
 v372_oxygen_bar.min_value=0.0
 v372_oxygen_bar.max_value=V372_OXYGEN.MAX_OXYGEN
 v372_oxygen_bar.show_percentage=false
 stack.add_child(v372_oxygen_bar)
 v372_oxygen_panel.visible=false

func _v372_dive_held()->bool:
 # C on keyboard or press the right controller stick while in the lake.
 if Input.is_key_pressed(KEY_C):return true
 for pad in Input.get_connected_joypads():
  if Input.is_joy_button_pressed(pad,JOY_BUTTON_RIGHT_STICK):return true
 return false

func _physics_process(delta:float)->void:
 super._physics_process(delta)
 var enabled:bool=V372_OXYGEN.can_dive(v37_swimming,hero_playing,menu_open,mounted,death_active)
 var previous:bool=v372_diving
 v372_diving=enabled and _v372_dive_held()
 if previous and not v372_diving and v372_oxygen<=V372_OXYGEN.LOW_OXYGEN and enabled:
  _show_message("Surfaced: breathing restored.",1.6)
 if v372_diving and not previous:
  _show_message("Diving: return to the surface for air!",2.0)
 # Oxygen is a localized dive resource, never a world timer.
 if not hero_playing or death_active or not v37_swimming:
  v372_oxygen=V372_OXYGEN.MAX_OXYGEN
  v372_drowning_elapsed=0.0
  v372_low_air_warned=false
 elif not menu_open:
  v372_oxygen=V372_OXYGEN.advance(v372_oxygen,delta,v372_diving)
  if v372_diving and V372_OXYGEN.is_low(v372_oxygen) and not v372_low_air_warned:
   v372_low_air_warned=true
   _show_message("LOW OXYGEN: reach the surface for air!",2.5)
  if not V372_OXYGEN.is_low(v372_oxygen):v372_low_air_warned=false
  if v372_diving and v372_oxygen<=0.0:
   v372_drowning_elapsed+=clampf(delta,0.0,0.25)
   if v372_drowning_elapsed>=V372_OXYGEN.DROWN_TICK_SECONDS:
    v372_drowning_elapsed-=V372_OXYGEN.DROWN_TICK_SECONDS
    _apply_incoming_damage(V372_OXYGEN.damage_tick(max_hp))
  else:v372_drowning_elapsed=0.0
 v372_ui_elapsed+=minf(delta,0.2)
 if v372_ui_elapsed>=0.10 or previous!=v372_diving:
  v372_ui_elapsed=0.0
  _v372_update_hud()

func _v372_update_hud()->void:
 if not is_instance_valid(v372_oxygen_panel):return
 var show_air:bool=hero_playing and not death_active and v37_swimming and not mounted and not menu_open
 v372_oxygen_panel.visible=show_air
 if not show_air:return
 v372_oxygen_bar.value=v372_oxygen
 var amount:int=ceili(v372_oxygen)
 v372_oxygen_label.text=("OXYGEN  %d%%  •  DIVE" if v372_diving else "OXYGEN  %d%%  •  HOLD C TO DIVE")%amount
 v372_oxygen_label.add_theme_color_override("font_color",Color("ff796e") if V372_OXYGEN.is_low(v372_oxygen) else Color("b2efff"))

func _map_movement_to_camera(raw_direction:Vector3)->Vector3:
 var movement:Vector3=super._map_movement_to_camera(raw_direction)
 return movement*V372_OXYGEN.DIVE_MOVE_MULTIPLIER if v372_diving else movement

func _v37_tick_realm()->void:
 super._v37_tick_realm()
 if v372_diving and v37_swim_zone>=0 and v37_swim_zone<v37_wakes.size():
  var wake:MeshInstance3D=v37_wakes[v37_swim_zone]
  if is_instance_valid(wake):wake.visible=false

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]UNDERWATER AIR[/b]\nHold C or press the controller right stick while swimming to dive. Oxygen drains only while diving, refills at the surface, and resets on land. Return to air before it reaches zero. No survival stopwatch."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["dive_oxygen"]=v372_oxygen
 report["diving"]=v372_diving
 report["oxygen_hud_visible"]=is_instance_valid(v372_oxygen_panel) and v372_oxygen_panel.visible
 return report
