extends "res://scripts/native_v38_8.gd"
## Cycling uses the existing beast list, visibility checks and lock-on state.
func _unhandled_input(event:InputEvent)->void:
 if hero_playing and not menu_open and not death_active and not wait_open:
  if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_TAB:
   _cycle_aim_target()
   get_viewport().set_input_as_handled()
   return
  if event is InputEventJoypadButton and event.pressed and event.button_index==JOY_BUTTON_RIGHT_SHOULDER:
   # Hold right-stick click plus right shoulder to cycle without replacing spell binding.
   if Input.is_joy_button_pressed(event.device,JOY_BUTTON_RIGHT_STICK):
    _cycle_aim_target()
    get_viewport().set_input_as_handled()
    return
 super._unhandled_input(event)

func _cycle_aim_target()->void:
 if not is_instance_valid(player) or region_index<0 or region_index>=zone_beast_lists.size():return
 var eligible:Array[int]=[]
 for beast in zone_beast_lists[region_index]:
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body") as CharacterBody3D
  if not is_instance_valid(body):continue
  if player.global_position.distance_squared_to(body.global_position)>AIM_RANGE*AIM_RANGE:continue
  if not _has_clear_sight(player,body):continue
  var index:int=beasts.find(beast)
  if index>=0:eligible.append(index)
 if eligible.is_empty():
  aim_locked_id=-1
  _show_message("No visible target",0.9)
  return
 eligible.sort()
 var next_index:int=eligible.find(aim_locked_id)+1
 if next_index>=eligible.size():next_index=0
 aim_locked_id=eligible[next_index]
 _show_message("Target "+str(next_index+1)+"/"+str(eligible.size())+" locked",0.8)
