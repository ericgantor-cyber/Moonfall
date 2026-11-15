extends "res://scripts/native_v38_8.gd"
## Timing feedback and posture pressure on real attacks. Original hero/boss art untouched.
const RHYTHM:=preload("res://scripts/systems/combat_rhythm.gd")
var rhythm_chain:int=0
var rhythm_last_strike_at:float=-999.0
var rhythm_strike_active:bool=false
var rhythm_last_posture_message_at:float=-999.0
var rhythm_dodge_started_at:float=-999.0
var rhythm_dodge_rewarded:bool=false
var rhythm_counter_until:float=-999.0
const RHYTHM_COUNTER_MULTIPLIER:float=1.25
const RHYTHM_COUNTER_SECONDS:float=1.0

func _attack()->void:
 # The inherited attack owns input, cooldowns, stamina, damage, and animation.
 var eligible:bool=hero_playing and not menu_open and not death_active and not wait_open and not wait_transition_active and not mounted and attack_cooldown<=0.0
 var previous_chain:int=rhythm_chain
 if eligible:
  rhythm_chain=RHYTHM.next_chain(rhythm_chain,map_time-rhythm_last_strike_at)
 var before_cooldown:float=attack_cooldown
 rhythm_strike_active=eligible
 super._attack()
 rhythm_strike_active=false
 if eligible and attack_cooldown>before_cooldown:
  rhythm_last_strike_at=map_time
  if rhythm_chain==3:
   _show_message("TRIPLE STRIKE!",0.65)
   if is_instance_valid(player):_cue(player.global_position+Vector3(0.0,-0.35,0.0),"attack",Color("89dfff"))
  elif rhythm_chain==5:
   _show_message("FIFTH STRIKE FINISHER!",0.75)
   if is_instance_valid(player):_cue(player.global_position+Vector3(0.0,-0.35,0.0),"spell",Color("e8a2ff"))
 else:
  rhythm_chain=previous_chain

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 # Only an actual melee hit consumes a counter reward; spells retain base damage.
 var before:float=float(target.get("hp",0.0))
 var counter_ready:bool=rhythm_strike_active and map_time<=rhythm_counter_until and bool(target.get("hostile",false))
 super._apply_player_hit(target,base_damage*(RHYTHM_COUNTER_MULTIPLIER if counter_ready else 1.0),stagger_strength)
 if counter_ready and float(target.get("hp",0.0))<before:
  rhythm_counter_until=-999.0
  _show_message("DODGE COUNTER!",0.75)
 if not rhythm_strike_active or not bool(target.get("hostile",false)) or not bool(target.get("alive",false)):return
 if float(target.get("hp",0.0))>=before:return
 var maximum:float=100.0 if bool(target.get("boss",false)) else 60.0
 var current:float=float(target.get("rhythm_posture",0.0))
 var impact:float=12.0+4.0*float(clampi(rhythm_chain,1,RHYTHM.MAX_CHAIN)-1)
 var next_posture:float=RHYTHM.posture_after_hit(current,maximum,impact,v393_riposte_active)
 target["rhythm_posture"]=next_posture
 if RHYTHM.posture_broken(next_posture,maximum):
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

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 rhythm_chain=0
 rhythm_last_strike_at=-999.0
 rhythm_strike_active=false
 rhythm_last_posture_message_at=-999.0
 rhythm_dodge_started_at=-999.0
 rhythm_dodge_rewarded=false
 rhythm_counter_until=-999.0
