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

# Issue #477 stage 2: two actual divine world bosses reuse native boss
# combat, telegraphs, phases, reward tracking and v2 checkpoint IDs.
# Spawned after all legacy beasts to preserve historical boss IDs.
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
 actor["divine_title"]=str(spec["title"])
 var factor:float=2.4 if str(spec["id"])=="z" else 2.7
 actor["max"]=float(actor["max"])*factor
 actor["base_hp"]=float(actor["max"])
 actor["hp"]=float(actor["max"])
 actor["divine_next_at"]=6.0
 actor["divine_warn_until"]=0.0
 actor["divine_warning_pos"]=location
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
 beast["divine_warn_until"]=0.0
 beast["divine_next_at"]=map_time+6.0
 var warning:MeshInstance3D=beast.get("divine_disk") as MeshInstance3D
 if is_instance_valid(warning):warning.hide()

# Earned divine blessings use existing permanent boss defeat IDs. This avoids
# adding fragile fields to schema-2 checkpoints or the old hero identity.
const Z_VOID_DAMAGE_BONUS:float=0.15
const Z_VOID_ON_KILL_HEAL:float=3.0
const NYRA_ECLIPSE_MAX_HP_BONUS:float=30.0
const NYRA_ECLIPSE_REDUCTION:float=0.08

func _divine_blessing_earned(divine_id:String)->bool:
 for actor in beasts:
  if str(actor.get("divine_id",""))==divine_id:
   return defeated_boss_ids.has(int(actor.get("boss_id",-1)))
 return false

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
 if first_clear:
  var reward:String="+15% damage and +3 HP on kills, permanently" if divine=="z" else "+30 max HP and 8% damage reduction, permanently"
  _show_message(str(beast.get("divine_title",divine))+" DEFEATED! "+reward+".",5.0)
  _update_hud()
  # Parent autosave is cooldown-limited; divine milestones always persist.
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false
