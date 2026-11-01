extends "res://scripts/native_v38_6.gd"
## Directional targeting extension. Existing hero art and world are untouched.
const AIM_RANGE:float=13.0
const AIM_CONE_DOT:float=0.72
const AIM_ASSIST_DOT:float=0.55
const AIM_STICK_DEADZONE:float=0.25
var aim_locked_id:int=-1
var aim_assist_enabled:bool=true
var aim_direction:Vector3=Vector3.FORWARD

func _unhandled_input(event:InputEvent)->void:
 if hero_playing and not menu_open and not death_active:
  if event is InputEventMouseButton and event.pressed and event.button_index==MOUSE_BUTTON_RIGHT:
   _toggle_aim_lock()
   get_viewport().set_input_as_handled()
   return
  if event is InputEventJoypadButton and event.pressed and event.button_index==JOY_BUTTON_RIGHT_STICK:
   _toggle_aim_lock()
   get_viewport().set_input_as_handled()
   return
 super._unhandled_input(event)

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing or menu_open or death_active or not is_instance_valid(player):return
 var pads:Array[int]=Input.get_connected_joypads()
 if not pads.is_empty():
  var pad:int=pads[0]
  var stick:Vector2=Vector2(Input.get_joy_axis(pad,JOY_AXIS_RIGHT_X),Input.get_joy_axis(pad,JOY_AXIS_RIGHT_Y))
  if stick.length()>AIM_STICK_DEADZONE and is_instance_valid(camera):
   var right:Vector3=camera.global_transform.basis.x
   var forward:Vector3=-camera.global_transform.basis.z
   right.y=0.0
   forward.y=0.0
   var direction:Vector3=right.normalized()*stick.x+forward.normalized()*-stick.y
   if direction.length_squared()>0.01:aim_direction=direction.normalized()
 elif is_instance_valid(camera):
  var cursor:Vector2=get_viewport().get_mouse_position()
  var origin:Vector3=camera.project_ray_origin(cursor)
  var ray:Vector3=camera.project_ray_normal(cursor)
  if absf(ray.y)>0.001:
   var t:float=(player.global_position.y-origin.y)/ray.y
   if t>0.0:
    var aim_point:Vector3=origin+ray*t
    var direction:Vector3=aim_point-player.global_position
    direction.y=0.0
    if direction.length_squared()>0.04:aim_direction=direction.normalized()

func _aim_candidate(radius:float)->Dictionary:
 if not is_instance_valid(player) or region_index<0 or region_index>=zone_beast_lists.size():return {}
 var locked:Dictionary={}
 var closest:Dictionary={}
 var best_score:float=-INF
 var max_distance:float=radius*radius
 for beast in zone_beast_lists[region_index]:
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body") as CharacterBody3D
  if not is_instance_valid(body):continue
  var to_enemy:Vector3=body.global_position-player.global_position
  to_enemy.y=0.0
  var distance:float=to_enemy.length_squared()
  if distance>max_distance or distance<0.001 or not _has_clear_sight(player,body):continue
  if aim_locked_id>=0 and beasts.find(beast)==aim_locked_id:locked=beast
  var alignment:float=to_enemy.normalized().dot(aim_direction)
  var threshold:float=AIM_ASSIST_DOT if aim_assist_enabled else AIM_CONE_DOT
  if alignment<threshold:continue
  var score:float=alignment-distance*0.002
  if score>best_score:
   best_score=score
   closest=beast
 # Clear stale locks on death, despawn, region changes or lost visibility.
 if aim_locked_id>=0 and locked.is_empty():aim_locked_id=-1
 return locked if not locked.is_empty() else closest

func _toggle_aim_lock()->void:
 if aim_locked_id>=0:
  aim_locked_id=-1
  _show_message("Target unlocked",0.8)
  return
 var target:Dictionary=_aim_candidate(AIM_RANGE)
 if target.is_empty():
  _show_message("No target in aim",0.8)
  return
 aim_locked_id=beasts.find(target)
 _show_message("Target locked",0.8)

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 aim_locked_id=-1

func _spell(which:String)->void:
 if which!="fire":
  super._spell(which)
  return
 if not hero_playing or menu_open or death_active or wait_open or wait_transition_active:return
 if mounted or ability_cooldown>0:return
 var target:Dictionary=_aim_candidate(AIM_RANGE)
 if target.is_empty():
  _show_message("Aim at a visible enemy for Eclipse Flame.",1.0)
  return
 ability_cooldown=3.5*maxf(0.65,1.0-0.08*float(_perk_rank("celerity")))
 var power:float=35.0*_hero_damage_multiplier()*(1.0+0.08*float(_perk_rank("focus")))
 _apply_player_hit(target,power,0.18)
 _show_message("Eclipse Flame: aimed hit!",0.9)
