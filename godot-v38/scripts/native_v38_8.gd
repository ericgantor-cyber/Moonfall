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
  # Never flash aim controls on title/menu frames before _process runs.
  aim_options.visible=false
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

# Optional WoW-inspired modifier layer. Unmodified controller buttons and
# existing mouse/keyboard bindings remain inherited and fully functional.
# Two trigger layers share the same four face buttons: no extra game mode.
func _wow_combo_action(button:int,left:bool,right:bool)->String:
 if left==right:return ""
 if left:
  match button:
   JOY_BUTTON_X:return "dash"
   JOY_BUTTON_Y:return "nova"
   JOY_BUTTON_B:return "fire"
   JOY_BUTTON_A:return "jump"
 else:
  match button:
   JOY_BUTTON_X:return "senses"
   JOY_BUTTON_Y:return "companion"
   JOY_BUTTON_B:return "mount"
   JOY_BUTTON_A:return "interact"
 return ""

func _wow_keyboard_action(key:int)->String:
 match key:
  KEY_1:return "dash"
  KEY_2:return "nova"
  KEY_3:return "fire"
  KEY_4:return "senses"
  KEY_5:return "companion"
  KEY_6:return "mount"
 return ""

func _wow_perform_action(action:String)->void:
 match action:
  "dash":_spell("dash")
  "nova":_spell("nova")
  "fire":_spell("fire")
  "jump":_jump()
  "senses":_toggle_senses()
  "companion":_cycle_companion_stance()
  "mount":_toggle_mount()
  "interact":_interact()

func _unhandled_key_input(event:InputEvent)->void:
 # Godot dispatches unhandled keyboard input before generic unhandled input.
 # This must run before the inherited handler marks all keys consumed.
 if event is InputEventKey and event.pressed and not event.echo and event.alt_pressed and not event.ctrl_pressed and hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active:
  var action:String=_wow_keyboard_action(event.keycode)
  if not action.is_empty():
   _wow_perform_action(action)
   get_viewport().set_input_as_handled()
   return
 super._unhandled_key_input(event)

func _unhandled_input(event:InputEvent)->void:
 # Intercept only explicit modifiers while playing, never menus or rest.
 if hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active:
  if event is InputEventJoypadButton and event.pressed:
   var left:bool=Input.get_joy_axis(event.device,JOY_AXIS_TRIGGER_LEFT)>0.6
   var right:bool=Input.get_joy_axis(event.device,JOY_AXIS_TRIGGER_RIGHT)>0.6
   var action:String=_wow_combo_action(event.button_index,left,right)
   if not action.is_empty():
    _wow_perform_action(action)
    get_viewport().set_input_as_handled()
    return
 # Mouse motion claims aim source even while a controller remains connected.
 if event is InputEventMouseMotion:
  aim_input_source="mouse"
 elif event is InputEventJoypadMotion and (event.axis==JOY_AXIS_RIGHT_X or event.axis==JOY_AXIS_RIGHT_Y) and absf(event.axis_value)>AIM_STICK_DEADZONE:
  aim_input_source="pad"
 super._unhandled_input(event)

# Camera preferences are only adjusted through the Atlas > System > Camera
# Settings panel. The two FOV controls are focusable buttons, not gameplay keys.
func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="System" and is_instance_valid(menu_body):
  _atlas_action("CAMERA SETTINGS • FOV, ORBIT & ACCESSIBILITY","camera_settings_open","Adjust field of view using focused buttons; mouse wheel always zooms during gameplay.")

func _v36_set_fov(value:float)->void:
 # FOV only changes through Atlas Settings; never from gameplay mouse input.
 v36_fov=clampf(value,58.0,76.0)
 if is_instance_valid(camera):camera.fov=v36_fov
 _v36_save_camera_preferences()

func _v36_show_camera_settings()->void:
 if not is_instance_valid(menu_body) or not is_instance_valid(menu_buttons):return
 current_tab="System"
 for child in menu_buttons.get_children():child.queue_free()
 var view_label:String="WIDE" if v36_fov>=72.0 else "FOCUSED" if v36_fov<=62.0 else "BALANCED"
 menu_body.text="[b]CAMERA SETTINGS[/b]\nAll camera preferences save automatically. Use buttons with mouse, keyboard focus, or controller.\n\n[b]FIELD OF VIEW[/b]: "+str(roundi(v36_fov))+"°  • "+view_label+"\nFocused 58°: larger distant objects | Balanced 66°: default | Wide 76°: more surroundings.\nCamera zoom is independent: gameplay mouse wheel always moves the camera closer or farther.\n\n[b]ORBIT SENSITIVITY[/b]: "+str(v36_orbit_sensitivity)+"\n[b]INVERT HORIZONTAL[/b]: "+("ON" if v36_invert_x else "OFF")+"\n[b]INVERT VERTICAL[/b]: "+("ON" if v36_invert_y else "OFF")+"\n\nAlt+right-drag orbits; right-click guards."
 _atlas_action("FOV − 1° • FINE","camera_fov_fine_down","Narrow the view by one degree")
 _atlas_action("FOV + 1° • FINE","camera_fov_fine_up","Widen the view by one degree")
 _atlas_action("FOV − 5° • QUICK","camera_fov_down","Narrow the view quickly")
 _atlas_action("FOV + 5° • QUICK","camera_fov_up","Widen the view quickly")
 _atlas_action("FOCUSED VIEW • 58°","camera_fov_focused","Larger distant targets")
 _atlas_action("BALANCED VIEW • 66°","camera_fov_reset","Original view")
 _atlas_action("WIDE VIEW • 76°","camera_fov_wide","See more of the surrounding world")
 _atlas_action("CYCLE ORBIT SENSITIVITY","camera_sensitivity")
 _atlas_action("TOGGLE HORIZONTAL INVERSION","camera_invert_x")
 _atlas_action("TOGGLE VERTICAL INVERSION","camera_invert_y")
 _atlas_action("RECENTER CAMERA","camera_recenter")
 _atlas_action("BACK TO SYSTEM","camera_settings_back")

func _atlas_execute(action:String)->void:
 if action.begins_with("camera_"):
  if action=="camera_settings_open":
   _v36_show_camera_settings()
   return
  if action=="camera_settings_back":
   _menu_tab("System")
   return
  match action:
   "camera_fov_fine_down":_v36_set_fov(v36_fov-1.0)
   "camera_fov_fine_up":_v36_set_fov(v36_fov+1.0)
   "camera_fov_down":_v36_set_fov(v36_fov-5.0)
   "camera_fov_up":_v36_set_fov(v36_fov+5.0)
   "camera_fov_focused":_v36_set_fov(58.0)
   "camera_fov_reset":_v36_set_fov(66.0)
   "camera_fov_wide":_v36_set_fov(76.0)
   "camera_sensitivity":v36_orbit_sensitivity=_v36_cycle_camera_setting(V36_SENSITIVITIES,v36_orbit_sensitivity)
   "camera_invert_x":v36_invert_x=not v36_invert_x
   "camera_invert_y":v36_invert_y=not v36_invert_y
   "camera_recenter":_v36_recenter_camera()
  _v36_save_camera_preferences()
  _v36_show_camera_settings()
  return
 super._atlas_execute(action)

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
 aim_marker.text="◆ LOCKED" if aim_locked_id>=0 and body.get_instance_id()==aim_locked_id else "◎ TARGET"
 aim_marker.visible=true

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 aim_target_id=-1

# Issue #477 stage 2: two actual divine world bosses reuse native boss
# combat, telegraphs, phases, reward tracking and v2 checkpoint IDs.
# Spawned after all legacy beasts to preserve historical boss IDs.
var divine_boss_ids:Dictionary={}
var divine_last_visible_zone:int=-1
const DIVINE_ENCOUNTERS:Array[Dictionary]=[
 {"id":"z","title":"Z • VOID REAPER","zone":4,"position":Vector3(11.0,0.0,44.0),"kind":"wolf"},
 {"id":"nyra","title":"NYRA • ECLIPSE QUEEN","zone":5,"position":Vector3(73.0,0.0,43.0),"kind":"panther"}
]

func _build_scene()->void:
 super._build_scene()
 for spec in DIVINE_ENCOUNTERS:
  _spawn_divine_encounter(spec)

func _spawn_divine_encounter(spec:Dictionary)->void:
 var zone:int=int(spec["zone"])
 var kind:String=str(spec["kind"])
 var location:Vector3=spec["position"]
 var prior:int=beasts.size()
 # The game's current boss AI already handles attack telegraphs, phases,
 # boss HP display, defeat rewards and persistent boss IDs.
 _spawn_beast(location,kind,zone,"elder",true)
 if beasts.size()!=prior+1:return
 var actor:Dictionary=beasts[-1]
 actor["divine_id"]=str(spec["id"])
 divine_boss_ids[str(spec["id"])]=int(actor.get("boss_id",-1))
 actor["divine_title"]=str(spec["title"])
 var factor:float=2.4 if str(spec["id"])=="z" else 2.7
 actor["max"]=float(actor["max"])*factor
 actor["base_hp"]=float(actor["max"])
 actor["hp"]=float(actor["max"])
 actor["divine_next_at"]=6.0
 actor["divine_warn_until"]=0.0
 actor["divine_warning_pos"]=location
 actor["divine_pattern_seq"]=0
 actor["divine_chain_pending"]=false
 actor["divine_last_aimed"]=false
 var native_body:CharacterBody3D=actor.get("body") as CharacterBody3D
 if not is_instance_valid(native_body):return
 native_body.name="DivineBoss_"+str(spec["id"])
 # Original image bytes are reused, never recolored or modified.
 var texture:Texture2D=Z_ART if str(spec["id"])=="z" else NYRA_ART
 for child in native_body.get_children():
  if child is MeshInstance3D:
   child.hide()
 var portrait:=Sprite3D.new()
 portrait.name="OriginalDivineBossArt"
 portrait.texture=texture
 portrait.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 portrait.pixel_size=3.6/float(texture.get_height())
 portrait.position=Vector3(0,1.1,0)
 portrait.alpha_cut=SpriteBase3D.ALPHA_CUT_OPAQUE_PREPASS
 native_body.add_child(portrait)
 var label:Label3D=actor.get("nameplate") as Label3D
 if is_instance_valid(label):
  label.text=str(spec["title"])
  label.position=Vector3(0,3.4,0)
 actor["divine_visual"]=portrait
 var warning:=MeshInstance3D.new()
 warning.name="DivineSignatureAttackWarning"
 var disk:=CylinderMesh.new()
 disk.bottom_radius=3.0
 disk.top_radius=3.0
 disk.height=0.035
 disk.radial_segments=32
 warning.mesh=disk
 warning.material_override=_material(Color("e04467") if str(spec["id"])=="z" else Color("9261ed"),0.75,true)
 warning.transparency=0.45
 warning.visible=false
 world.add_child(warning)
 actor["divine_disk"]=warning

func _reset_native_beast(beast:Dictionary)->void:
 super._reset_native_beast(beast)
 if not beast.has("divine_id"):return
 var label:Label3D=beast.get("nameplate") as Label3D
 if is_instance_valid(label):label.text=str(beast.get("divine_title","DIVINE BOSS"))
 _clear_divine_warning(beast)
 beast["divine_next_at"]=map_time+6.0
 beast["divine_pattern_seq"]=0
 beast["divine_chain_pending"]=false

# Earned divine blessings use existing permanent boss defeat IDs. This avoids
# adding fragile fields to schema-2 checkpoints or the old hero identity.
const Z_VOID_DAMAGE_BONUS:float=0.15
const Z_VOID_ON_KILL_HEAL:float=3.0
const NYRA_ECLIPSE_MAX_HP_BONUS:float=30.0
const NYRA_ECLIPSE_REDUCTION:float=0.08

func _divine_blessing_earned(divine_id:String)->bool:
 # Two stable IDs are registered at spawn; avoid scanning every actor on hits.
 if not divine_boss_ids.has(divine_id):return false
 return defeated_boss_ids.has(int(divine_boss_ids[divine_id]))

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 # Shared melee, spell and projectile hits pass through this damage function.
 var boosted:float=base_damage*(1.0+Z_VOID_DAMAGE_BONUS) if _divine_blessing_earned("z") else base_damage
 super._apply_player_hit(target,boosted,stagger_strength)

func _apply_incoming_damage(raw:float)->void:
 var mitigated:float=raw*(1.0-NYRA_ECLIPSE_REDUCTION) if _divine_blessing_earned("nyra") else raw
 super._apply_incoming_damage(mitigated)

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 # Parent restores ordinary vigor HP first; do not allow its old cap to
 # silently erase health saved after the permanent Nyra blessing.
 if _divine_blessing_earned("nyra"):
  max_hp+=NYRA_ECLIPSE_MAX_HP_BONUS
  hp=clampf(float(data.get("hp",hp)),1.0,max_hp)
 # Pending signature strikes are transient, never saved. A load must not
 # replay an old warning against the restored player.
 _clear_all_divine_warnings()
 _update_hud()

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 var divine:String=str(beast.get("divine_id",""))
 var first_clear:bool=not divine.is_empty() and bool(beast.get("alive",false)) and not defeated_boss_ids.has(int(beast.get("boss_id",-1)))
 if _divine_blessing_earned("z") and bool(beast.get("alive",false)):
  hp=minf(max_hp,hp+Z_VOID_ON_KILL_HEAL)
 if first_clear and divine=="nyra":
  # Apply before parent victory autosave so max health and current HP persist.
  max_hp+=NYRA_ECLIPSE_MAX_HP_BONUS
  hp=minf(max_hp,hp+NYRA_ECLIPSE_MAX_HP_BONUS)
 super._defeat_beast(beast,pet_kill)
 if not divine.is_empty():_clear_divine_warning(beast)
 if first_clear:
  var reward:String="+15% damage and +3 HP on kills, permanently" if divine=="z" else "+30 max HP and 8% damage reduction, permanently"
  _show_message(str(beast.get("divine_title",divine))+" DEFEATED! "+reward+".",5.0)
  _update_hud()
  # Parent autosave is cooldown-limited; divine milestones always persist.
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false

# Signature divine attacks are additional to existing boss patterns. Each
# warning is one reusable mesh; the existing dodge and armor pipeline applies.
func _update_beast_step(beast:Dictionary,delta:float)->void:
 super._update_beast_step(beast,delta)
 if not bool(beast.get("alive",false)) or not beast.has("divine_id"):return
 if not is_instance_valid(player) or not hero_playing or menu_open or death_active or wait_open or wait_transition_active:
  _clear_divine_warning(beast)
  return
 # A distant/hidden boss must never resolve a stale marked attack after travel.
 if int(beast.get("zone",-1))!=region_index:
  _clear_divine_warning(beast)
  return
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 var warning:MeshInstance3D=beast.get("divine_disk") as MeshInstance3D
 if not is_instance_valid(body) or not is_instance_valid(warning):return
 if body.global_position.distance_squared_to(player.global_position)>25.0*25.0:
  _clear_divine_warning(beast)
  return
 var remaining:float=float(beast.get("divine_warn_until",0.0))
 if remaining>0.0:
  if map_time<remaining:
   warning.visible=true
   # Faster pulses as the tell expires communicate imminent impact.
   var urgency:float=clampf(1.0-(remaining-map_time)/1.7,0.0,1.0)
   warning.transparency=0.30+0.18*absf(sin(map_time*(6.0+urgency*8.0)))
   return
  warning.hide()
  beast["divine_warn_until"]=0.0
  var chained:bool=bool(beast.get("divine_chain_pending",false))
  beast["divine_chain_pending"]=false
  var strike:Vector3=beast.get("divine_warning_pos",body.global_position)
  var radius:float=float(beast.get("divine_attack_radius",3.0 if str(beast["divine_id"])=="z" else 4.1))
  var dist_sq:float=Vector2(player.global_position.x-strike.x,player.global_position.z-strike.z).length_squared()
  if dist_sq<radius*radius and _has_clear_sight(body,player):
   if dodge_time>0.0:
    # A timed dodge through the danger area earns a small, bounded stamina return.
    v387_stamina=minf(V387_MAX_STAMINA,v387_stamina+5.0)
    _show_message("PERFECT EVADE! +5 stamina",0.9)
   else:
    _apply_incoming_damage(15.0+float(int(beast.get("boss_phase",1)))*8.0)
    _show_message("Divine strike! Leave the glowing mark or dodge.",1.5)
  # A phase-three opening strike earns exactly one marked follow-up.
  # Reuse the single disk and give players a fresh 1.15s escape window.
  if not chained and int(beast.get("boss_phase",1))==3 and bool(beast.get("alive",false)):
   # Reverse the opening pattern: first aimed -> follow-up centered on
   # the boss; first radial -> follow-up tracks the hero's current position.
   var follow_pos:Vector3=body.global_position if bool(beast.get("divine_last_aimed",false)) else player.global_position
   var follow_radius:float=2.8 if str(beast["divine_id"])=="z" else 3.5
   beast["divine_chain_pending"]=true
   beast["divine_warning_pos"]=follow_pos
   beast["divine_attack_radius"]=follow_radius
   beast["divine_warn_until"]=map_time+1.15
   var follow_disk:CylinderMesh=warning.mesh as CylinderMesh
   if is_instance_valid(follow_disk):
    follow_disk.bottom_radius=follow_radius
    follow_disk.top_radius=follow_radius
   warning.global_position=Vector3(follow_pos.x,0.07,follow_pos.z)
   warning.visible=true
   _show_message(("VOID REBOUND!" if str(beast["divine_id"])=="z" else "ECLIPSE REBOUND!")+" DODGE THE NEW MARK!",1.15)
  return
 if body.global_position.distance_squared_to(player.global_position)>12.0*12.0:return
 if map_time<float(beast.get("divine_next_at",0.0)):return
 var phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var divine:String=str(beast["divine_id"])
 var cycle:int=int(beast.get("divine_pattern_seq",0))
 beast["divine_pattern_seq"]=cycle+1
 # Phase two adds pattern variety; phase three retains alternating signatures.
 # The warning remains stationary during its full windup for fair counterplay.
 var aimed:bool=divine=="z"
 if phase>=2 and cycle%2==1:aimed=not aimed
 beast["divine_last_aimed"]=aimed
 var strike_pos:Vector3=player.global_position if aimed else body.global_position
 var radius:float=(3.0 if divine=="z" else 4.1)+(0.7 if phase==3 and cycle%2==1 else 0.0)
 beast["divine_attack_radius"]=radius
 beast["divine_warning_pos"]=strike_pos
 beast["divine_warn_until"]=map_time+maxf(0.9,1.7-0.25*float(phase-1))
 beast["divine_next_at"]=map_time+maxf(4.8,8.0-1.0*float(phase-1))
 var cylinder:CylinderMesh=warning.mesh as CylinderMesh
 if is_instance_valid(cylinder):
  cylinder.bottom_radius=radius
  cylinder.top_radius=radius
 # Position the telegraph consistently on its local ground plane.
 warning.global_position=Vector3(strike_pos.x,body.global_position.y+0.07,strike_pos.z)
 warning.visible=true
 _show_message(("VOID JUDGMENT: MOVE!" if aimed else "VOID COLLAPSE: GET CLEAR!") if divine=="z" else ("ECLIPSE MARK: MOVE!" if aimed else "ECLIPSE PULSE: GET CLEAR!"),1.9)

func _refresh_boss_phase(beast:Dictionary)->void:
 var previous:int=int(beast.get("boss_phase",1))
 super._refresh_boss_phase(beast)
 if not beast.has("divine_id") or not bool(beast.get("alive",false)):return
 var phase:int=int(beast.get("boss_phase",1))
 var label:Label3D=beast.get("nameplate") as Label3D
 if phase>previous:
  # A telegraphed strike from the previous phase must never resolve during
  # the transformation. This keeps boss phase changes readable and fair.
  _clear_divine_warning(beast)
  if is_instance_valid(label):
   label.text=str(beast.get("divine_title","DIVINE BOSS"))+" • PHASE "+str(phase)
  beast["divine_next_at"]=map_time+2.0
  _show_message(str(beast.get("divine_title","DIVINE BOSS"))+" AWAKENS • PHASE "+str(phase),2.5)

# The warning disks live outside region scenery so their positions remain
# fixed. Clear off-region warnings when travel switches AI simulation zones.
func _clear_divine_warning(actor:Dictionary)->void:
 actor["divine_warn_until"]=0.0
 actor["divine_chain_pending"]=false
 var disk:MeshInstance3D=actor.get("divine_disk") as MeshInstance3D
 if is_instance_valid(disk):disk.hide()

func _clear_all_divine_warnings()->void:
 for id in divine_boss_ids.values():
  var slot:int=int(id)
  if slot>=0 and slot<beasts.size():_clear_divine_warning(beasts[slot])

func _update_region_visibility()->void:
 super._update_region_visibility()
 # Region refresh runs several times per second. Avoid repeatedly traversing
 # every actor and keep on-zone warnings active until their normal resolution.
 if divine_last_visible_zone==region_index:return
 divine_last_visible_zone=region_index
 for id in divine_boss_ids.values():
  var slot:int=int(id)
  if slot<0 or slot>=beasts.size():continue
  var actor:Dictionary=beasts[slot]
  if int(actor.get("zone",-1))!=region_index or not bool(actor.get("alive",false)):
   _clear_divine_warning(actor)

func _update_hud()->void:
 super._update_hud()
 if not is_instance_valid(world_hud):return
 var buffs:String=""
 if _divine_blessing_earned("z"):buffs+="  | Z: +15% DMG / +3 KILL HP"
 if _divine_blessing_earned("nyra"):buffs+="  | NYRA: +30 HP / 8% GUARD"
 if not buffs.is_empty():world_hud.text+=buffs
