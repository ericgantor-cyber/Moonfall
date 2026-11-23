extends "res://scripts/native_v38_8.gd"
## Timing feedback and posture pressure on real attacks. Original hero/boss art untouched.
const RHYTHM:=preload("res://scripts/systems/combat_rhythm.gd")
var rhythm_chain:int=0
var rhythm_last_strike_at:float=-999.0
var rhythm_strike_active:bool=false
var rhythm_strike_landed:bool=false
var rhythm_pending_stage:int=1
var rhythm_last_posture_message_at:float=-999.0
var rhythm_dodge_started_at:float=-999.0
var rhythm_dodge_rewarded:bool=false
var rhythm_counter_until:float=-999.0
const RHYTHM_COUNTER_MULTIPLIER:float=1.25
const RHYTHM_COUNTER_SECONDS:float=1.0
const MENU_HIT_FLASH_MS:int=280
var menu_hit_flash_until_ms:int=0
var menu_hit_flash_layer:CanvasLayer
var menu_hit_flash_rect:ColorRect
var menu_hit_last_hp:float=-1.0

func _ready()->void:
 super._ready()
 # Render above every existing menu (including inventory and the talent web).
 # This overlay never captures pointer, controller, or keyboard input.
 menu_hit_flash_layer=CanvasLayer.new()
 menu_hit_flash_layer.name="MenuHitFlashLayer"
 menu_hit_flash_layer.layer=40
 add_child(menu_hit_flash_layer)
 menu_hit_flash_rect=ColorRect.new()
 menu_hit_flash_rect.name="MenuHitFlash"
 menu_hit_flash_rect.mouse_filter=Control.MOUSE_FILTER_IGNORE
 menu_hit_flash_layer.add_child(menu_hit_flash_rect)
 menu_hit_flash_rect.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 menu_hit_flash_rect.color=Color(0.94,0.07,0.11,0.0)
 menu_hit_flash_rect.visible=false
 menu_hit_last_hp=hp

func _menu_hit_flash_panel_open()->bool:
 return hero_playing and not death_active and (menu_open or wait_open or wait_transition_active
  or (is_instance_valid(atlas_panel) and atlas_panel.visible)
  or (is_instance_valid(v398_panel) and v398_panel.visible))

func _menu_hit_flash_begin()->void:
 if not _menu_hit_flash_panel_open():return
 menu_hit_flash_until_ms=Time.get_ticks_msec()+MENU_HIT_FLASH_MS
 _menu_hit_flash_draw()

func _menu_hit_flash_draw()->void:
 if not is_instance_valid(menu_hit_flash_rect):return
 var remaining_ms:int=menu_hit_flash_until_ms-Time.get_ticks_msec()
 if remaining_ms<=0 or not _menu_hit_flash_panel_open():
  # Never resurrect an old hit if the player closes and reopens a menu.
  menu_hit_flash_until_ms=0
  menu_hit_flash_rect.visible=false
  return
 # A single soft fade, not a repeated strobe or warning text.
 var portion:float=clampf(float(remaining_ms)/float(MENU_HIT_FLASH_MS),0.0,1.0)
 menu_hit_flash_rect.color=Color(0.94,0.07,0.11,0.16*portion)
 menu_hit_flash_rect.visible=true

func _menu_hit_flash_reset()->void:
 menu_hit_flash_until_ms=0
 menu_hit_last_hp=hp if is_finite(hp) else -1.0
 if is_instance_valid(menu_hit_flash_rect):menu_hit_flash_rect.visible=false

func _apply_incoming_damage(raw:float)->void:
 var before_hp:float=hp
 super._apply_incoming_damage(raw)
 # Count only real HP lost, after dodge, mitigation, wards and pet interception.
 if is_finite(before_hp) and is_finite(hp) and hp<before_hp and not death_active:
  _menu_hit_flash_begin()
 menu_hit_last_hp=hp if is_finite(hp) else -1.0


func _process(delta:float)->void:
 super._process(delta)
 # Also observe direct environmental HP loss that does not enter the
 # inherited _apply_incoming_damage pipeline. Avoid duplicate hit pulses.
 if hero_playing and not death_active and is_finite(hp):
  if menu_hit_last_hp>=0.0 and hp<menu_hit_last_hp:
   _menu_hit_flash_begin()
  menu_hit_last_hp=hp
 _menu_hit_flash_draw()
 # Keep the visible and saved-in-memory combo stage in sync with its window.
 # No extra work on enemies, no extra allocations, and no input changes.
 if rhythm_chain>0 and RHYTHM.chain_expired(rhythm_chain,map_time-rhythm_last_strike_at):
  rhythm_chain=0

func _attack()->void:
 # Only a landed attack advances this extra combo layer. Inherited damage,
 # cooldown and targeting still run exactly once per input.
 var eligible:bool=hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active and not mounted and attack_cooldown<=0.0
 var stage:int=RHYTHM.next_chain(rhythm_chain,map_time-rhythm_last_strike_at) if eligible else rhythm_chain
 var before_cooldown:float=attack_cooldown
 rhythm_strike_landed=false
 rhythm_pending_stage=stage
 rhythm_strike_active=eligible
 super._attack()
 rhythm_strike_active=false
 rhythm_pending_stage=1
 var accepted:bool=eligible and attack_cooldown>before_cooldown
 # A rejected input preserves the combo; an accepted miss ends it.
 rhythm_chain=RHYTHM.resolved_chain(rhythm_chain,stage,accepted,rhythm_strike_landed)
 if not accepted or not rhythm_strike_landed:return
 rhythm_last_strike_at=map_time
 if rhythm_chain==3:
  _show_message("TRIPLE STRIKE!",0.65)
  if is_instance_valid(player):_cue(player.global_position+Vector3(0.0,-0.35,0.0),"attack",Color("89dfff"))
 elif rhythm_chain==5:
  _show_message("FIFTH STRIKE FINISHER!",0.75)
  if is_instance_valid(player):_cue(player.global_position+Vector3(0.0,-0.35,0.0),"spell",Color("e8a2ff"))

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 # Only an actual melee hit consumes a counter reward; spells retain base damage.
 var before:float=float(target.get("hp",0.0))
 var was_alive:bool=bool(target.get("alive",false))
 var hostile:bool=bool(target.get("hostile",false))
 var counter_ready:bool=rhythm_strike_active and was_alive and hostile and map_time<=rhythm_counter_until
 super._apply_player_hit(target,base_damage*(RHYTHM_COUNTER_MULTIPLIER if counter_ready else 1.0),stagger_strength)
 var confirmed:bool=RHYTHM.confirmed_hostile_hit(was_alive,hostile,before,float(target.get("hp",0.0)))
 if rhythm_strike_active and confirmed:
  rhythm_strike_landed=true
 if counter_ready and confirmed:
  rhythm_counter_until=-999.0
  _show_message("DODGE COUNTER!",0.75)
 if not rhythm_strike_active or not confirmed or not bool(target.get("alive",false)):return
 var maximum:float=100.0 if bool(target.get("boss",false)) else 60.0
 var last_pressure_at:float=float(target.get("rhythm_last_pressure_at",map_time))
 var current:float=RHYTHM.recovered_posture(float(target.get("rhythm_posture",0.0)),maxf(0.0,map_time-last_pressure_at))
 target["rhythm_last_pressure_at"]=map_time
 var impact:float=RHYTHM.posture_impact(rhythm_pending_stage)
 var next_posture:float=RHYTHM.posture_after_hit(current,maximum,impact,v393_riposte_active)
 target["rhythm_posture"]=next_posture
 if RHYTHM.posture_broken(next_posture,maximum):
  if bool(target.get("boss",false)) and map_time<float(target.get("rhythm_break_immune_until",0.0)):
   target["rhythm_posture"]=maximum*0.5
   return
  if bool(target.get("boss",false)):target["rhythm_break_immune_until"]=map_time+4.0
  target["rhythm_posture"]=0.0
  target["stagger_until"]=maxf(float(target.get("stagger_until",0.0)),map_time+(0.55 if bool(target.get("boss",false)) else 0.9))
  var staggered_body:Node3D=target.get("body") as Node3D
  if is_instance_valid(staggered_body):
   _cue(staggered_body.global_position+Vector3(0.0,0.12,0.0),"spell",Color("ffc16e") if bool(target.get("boss",false)) else Color("d7c6ff"))
  if map_time-rhythm_last_posture_message_at>0.65:
   rhythm_last_posture_message_at=map_time
   _show_message("BOSS POSTURE BROKEN!" if bool(target.get("boss",false)) else "POSTURE BROKEN!",0.8)

func _dodge()->void:
 var previous:float=dodge_time
 super._dodge()
 if dodge_time>previous and dodge_time>0.0:
  rhythm_dodge_started_at=map_time
  rhythm_dodge_rewarded=false

func _rhythm_confirmed_enemy_dodge()->void:
 # Called only when the existing divine-boss attack resolves on a real
 # marked position and the player successfully dodged that attack.
 # No generic damage/hazard interception; does not alter boss damage.
 var elapsed:float=map_time-rhythm_dodge_started_at
 if not hero_playing or menu_open or death_active or mounted or wait_open or wait_transition_active:return
 if rhythm_dodge_rewarded or dodge_time<=0.0 or not RHYTHM.perfect_dodge(elapsed,true):return
 rhythm_dodge_rewarded=true
 rhythm_counter_until=map_time+RHYTHM_COUNTER_SECONDS
 _show_message("PERFECT DODGE! Counter now!",0.85)
 if is_instance_valid(player):_cue(player.global_position+Vector3(0.0,-0.45,0.0),"dodge",Color("86e9ff"))

func _reset_native_beast(beast:Dictionary)->void:
 super._reset_native_beast(beast)
 beast["rhythm_posture"]=0.0
 beast["rhythm_last_pressure_at"]=map_time
 beast["rhythm_break_immune_until"]=0.0

func _rhythm_reset_transient_combat()->void:
 # Never carry attack windows, dodge rewards, or counters across lives/saves.
 rhythm_chain=0
 rhythm_last_strike_at=-999.0
 rhythm_strike_active=false
 rhythm_strike_landed=false
 rhythm_pending_stage=1
 rhythm_last_posture_message_at=-999.0
 rhythm_dodge_started_at=-999.0
 rhythm_dodge_rewarded=false
 rhythm_counter_until=-999.0

func _start_game(selected_hero:String)->void:
 _rhythm_reset_transient_combat()
 super._start_game(selected_hero)
 _rhythm_reset_transient_combat()
 _menu_hit_flash_reset()

func _restore_data(data:Dictionary)->void:
 _rhythm_reset_transient_combat()
 super._restore_data(data)
 _rhythm_reset_transient_combat()
 _menu_hit_flash_reset()

func _respawn_player()->void:
 _rhythm_reset_transient_combat()
 super._respawn_player()
 _rhythm_reset_transient_combat()
 _menu_hit_flash_reset()

func _death_return_to_title()->void:
 _rhythm_reset_transient_combat()
 super._death_return_to_title()
 _menu_hit_flash_reset()
