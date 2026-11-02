extends "res://scripts/native_v38_8.gd"
## Cycling uses the existing beast list, visibility checks and lock-on state.
func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_TAB and hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active:
  _cycle_aim_target()
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _unhandled_input(event:InputEvent)->void:
 if hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active:
  if event is InputEventJoypadButton and event.pressed and event.button_index==JOY_BUTTON_RIGHT_SHOULDER:
   # Hold right-stick click plus right shoulder to cycle without replacing spell binding.
   if Input.is_joy_button_pressed(event.device,JOY_BUTTON_RIGHT_STICK):
    _cycle_aim_target()
    get_viewport().set_input_as_handled()
    return
 super._unhandled_input(event)

func _cycle_aim_target()->void:
 if not hero_playing or menu_open or death_active or wait_open or wait_transition_active:return
 if not is_instance_valid(player) or region_index<0 or region_index>=zone_beast_lists.size():return
 if beasts.is_empty():
  aim_locked_id=-1
  _show_message("No visible target",0.9)
  return
 if region_index>=0 and zone_beast_lists[region_index].is_empty():
  aim_locked_id=-1
  _show_message("No visible target",0.9)
  return
 var eligible:Array[int]=[]
 # Index the canonical beast list once instead of scanning it for every candidate.
 var zone_ids:Dictionary={}
 for beast in zone_beast_lists[region_index]:
  if typeof(beast)==TYPE_DICTIONARY and bool(beast.get("alive",false)) and bool(beast.get("hostile",false)):
   var body_ref:CharacterBody3D=beast.get("body") as CharacterBody3D
   if is_instance_valid(body_ref):zone_ids[body_ref.get_instance_id()]=true
 for index in range(beasts.size()):
  var beast:Dictionary=beasts[index]
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body") as CharacterBody3D
  if not is_instance_valid(body) or not zone_ids.has(body.get_instance_id()):continue
  if not body.visible:continue
  if player.global_position.distance_squared_to(body.global_position)>AIM_RANGE*AIM_RANGE:continue
  if not _has_clear_sight(player,body):continue
  eligible.append(index)
 if eligible.is_empty():
  aim_locked_id=-1
  _show_message("No visible target",0.9)
  return
 # Indices were collected in ascending canonical order; sorting is unnecessary.
 var next_index:int=eligible.bsearch(aim_locked_id + 1)
 if next_index>=eligible.size():next_index=0
 aim_locked_id=eligible[next_index]
 _show_message("Target "+str(next_index+1)+"/"+str(eligible.size())+" locked",0.8)
