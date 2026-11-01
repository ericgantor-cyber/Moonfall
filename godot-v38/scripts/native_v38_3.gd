extends "res://scripts/native_v38_2.gd"
## Optional gamepad atlas controls. Existing combat bindings remain in base scripts.
var pad_atlas_previous:Dictionary={}
var pad_atlas_cursor:Vector2=Vector2.ZERO
var pad_atlas_legend:Label
const PAD_ATLAS_DEADZONE:float=0.24

func _ready()->void:
 super._ready()
 if is_instance_valid(atlas_panel):
  pad_atlas_legend=Label.new()
  pad_atlas_legend.name="AtlasControllerHelp"
  pad_atlas_legend.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
  pad_atlas_legend.text="GAMEPAD: BACK/SELECT map • Right stick aim • A mark • B close • X clear • LB/RB zoom"
  var body:Control=atlas_panel.get_child(0) as Control
  if is_instance_valid(body):body.add_child(pad_atlas_legend)

func _pad_pressed(device:int,button:JoyButton,id:String)->bool:
 var down:bool=Input.is_joy_button_pressed(device,button)
 var key:String=str(device)+":"+id
 var was_down:bool=bool(pad_atlas_previous.get(key,false))
 pad_atlas_previous[key]=down
 return down and not was_down

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing or death_active or menu_open:return
 var devices:Array[int]=Input.get_connected_joypads()
 if devices.is_empty():return
 var device:int=devices[0]
 var toggle:bool=_pad_pressed(device,JOY_BUTTON_BACK,"map")
 if toggle:
  _toggle_atlas()
 if not is_instance_valid(atlas_panel) or not atlas_panel.visible:
  pad_atlas_cursor=Vector2.ZERO
  return
 var stick:Vector2=Vector2(Input.get_joy_axis(device,JOY_AXIS_RIGHT_X),Input.get_joy_axis(device,JOY_AXIS_RIGHT_Y))
 if stick.length()<PAD_ATLAS_DEADZONE:stick=Vector2.ZERO
 if stick!=Vector2.ZERO:
  var speed:float=atlas_zoom_radius*0.85
  pad_atlas_cursor+=stick*speed*minf(delta,0.05)
  pad_atlas_cursor=pad_atlas_cursor.limit_length(atlas_zoom_radius)
 if _pad_pressed(device,JOY_BUTTON_A,"mark") and is_instance_valid(player):
  var here:Vector3=player.global_position
  _atlas_select_waypoint(Vector3(here.x+pad_atlas_cursor.x,here.y,here.z+pad_atlas_cursor.y))
 if _pad_pressed(device,JOY_BUTTON_X,"clear"):_atlas_clear()
 if _pad_pressed(device,JOY_BUTTON_B,"close"):atlas_panel.visible=false
 if _pad_pressed(device,JOY_BUTTON_LEFT_SHOULDER,"zoom_out"):_atlas_zoom(1.333333)
 if _pad_pressed(device,JOY_BUTTON_RIGHT_SHOULDER,"zoom_in"):_atlas_zoom(0.75)
 if is_instance_valid(pad_atlas_legend):
  pad_atlas_legend.text="GAMEPAD BACK map | RIGHT STICK target (%.0f, %.0f) | A mark | B close | X clear | LB/RB zoom" % [pad_atlas_cursor.x,pad_atlas_cursor.y]
