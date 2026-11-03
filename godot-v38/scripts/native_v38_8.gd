extends "res://scripts/native_v38_7.gd"
## Lightweight aim feedback and user preference; no new mode or hero-art edits.
var aim_marker:Label3D
var aim_options:PanelContainer
var aim_toggle:CheckButton
var aim_input_source:String="mouse"
const AIM_PREFS_PATH:String="user://moonfall_aim.cfg"
var aim_target_id:int=-1

func _ready()->void:
 var config:ConfigFile=ConfigFile.new()
 if config.load(AIM_PREFS_PATH)==OK:
  aim_assist_enabled=bool(config.get_value("controls","aim_assist",true))
 super._ready()
 aim_marker=Label3D.new()
 aim_marker.name="AimTargetMarker"
 aim_marker.text="◎ TARGET"
 aim_marker.font_size=42
 aim_marker.pixel_size=0.004
 aim_marker.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 aim_marker.modulate=Color(1.0,0.8,0.25)
 aim_marker.visible=false
 add_child(aim_marker)
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if is_instance_valid(layer):
  aim_options=PanelContainer.new()
  aim_options.name="AimOptions"
  aim_options.position=Vector2(18,80)
  layer.add_child(aim_options)
  aim_toggle=CheckButton.new()
  aim_toggle.text="AIM ASSIST"
  aim_toggle.button_pressed=aim_assist_enabled
  aim_toggle.toggled.connect(_on_aim_assist_toggled)
  aim_options.add_child(aim_toggle)

func _on_aim_assist_toggled(enabled:bool)->void:
 aim_assist_enabled=enabled
 var config:ConfigFile=ConfigFile.new()
 config.set_value("controls","aim_assist",enabled)
 var result:Error=config.save(AIM_PREFS_PATH)
 if result!=OK:push_warning("Unable to save aim preference: "+str(result))

func _unhandled_input(event:InputEvent)->void:
 # Mouse motion claims aim source even while a controller remains connected.
 if event is InputEventMouseMotion:
  aim_input_source="mouse"
 elif event is InputEventJoypadMotion and (event.axis==JOY_AXIS_RIGHT_X or event.axis==JOY_AXIS_RIGHT_Y) and absf(event.axis_value)>AIM_STICK_DEADZONE:
  aim_input_source="pad"
 super._unhandled_input(event)

func _process(delta:float)->void:
 # Parent supplies aim direction and combat actions.
 super._process(delta)
 if is_instance_valid(aim_options):aim_options.visible=hero_playing and not menu_open and not death_active
 if not is_instance_valid(aim_marker):return
 aim_marker.visible=false
 if not hero_playing or menu_open or death_active or not is_instance_valid(player):return
 # Recover mouse cursor aiming when controller is connected but idle.
 if aim_input_source=="mouse" and is_instance_valid(camera):
  var cursor:Vector2=get_viewport().get_mouse_position()
  var origin:Vector3=camera.project_ray_origin(cursor)
  var ray:Vector3=camera.project_ray_normal(cursor)
  if absf(ray.y)>0.001:
   var distance:float=(player.global_position.y-origin.y)/ray.y
   if distance>0.0:
    var direction:Vector3=origin+ray*distance-player.global_position
    direction.y=0.0
    if direction.length_squared()>0.04:aim_direction=direction.normalized()
 var target:Dictionary=_aim_candidate(AIM_RANGE)
 if target.is_empty():
  if aim_locked_id>=0:aim_locked_id=-1
  return
 var body:CharacterBody3D=target.get("body") as CharacterBody3D
 if not is_instance_valid(body):return
 aim_marker.global_position=body.global_position+Vector3(0,2.2,0)
 aim_marker.text="◆ LOCKED" if aim_locked_id>=0 and beasts.find(target)==aim_locked_id else "◎ TARGET"
 aim_marker.visible=true

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 aim_target_id=-1
