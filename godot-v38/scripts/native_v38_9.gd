extends "res://scripts/native_v38_8.gd"
## Timing feedback and posture pressure on real attacks. Original hero/boss art untouched.
const RHYTHM:=preload("res://scripts/systems/combat_rhythm.gd")
var rhythm_chain:int=0
var rhythm_last_strike_at:float=-999.0
var rhythm_strike_active:bool=false
var rhythm_last_posture_message_at:float=-999.0

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
  if rhythm_chain==3:_show_message("TRIPLE STRIKE!",0.65)
  elif rhythm_chain==5:_show_message("FIFTH STRIKE FINISHER!",0.75)
 else:
  rhythm_chain=previous_chain

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 # Real hostile contact only: no damage changes and no posture on spell hits.
 var before:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
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
  if map_time-rhythm_last_posture_message_at>0.65:
   rhythm_last_posture_message_at=map_time
   _show_message("POSTURE BROKEN!",0.8)

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 rhythm_chain=0
 rhythm_last_strike_at=-999.0
 rhythm_strike_active=false
 rhythm_last_posture_message_at=-999.0
