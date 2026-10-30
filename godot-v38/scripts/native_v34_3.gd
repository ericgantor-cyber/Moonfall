extends "res://scripts/native_v34_2.gd"
## v34.3 Lunar War: phase-based commanders and rewarded counterplay.
## No extra invaders, survival countdown, building, or player-art edits.
const V343_RULES:=preload("res://scripts/systems/invasion_tactics_v343.gd")
var v343_phase:int=0
var v343_dodge_chain:int=0
var v343_perfect_dodges:int=0
var v343_real_hits:int=0
var v343_exposure_triggers:int=0
var v343_clean_seals:int=0
var v343_phase_changes:int=0
var v343_poll_clock:float=0.0

func _release_label()->String:return "v34.3 • Lunar War"

func _v343_commander()->Dictionary:
 if v341_active_site<0 or v341_wave!=3:return {}
 var roster:Array=v341_sites[v341_active_site]["roster"]
 if roster.is_empty():return {}
 return beasts[int(roster[-1])]

func _v343_update_phase()->void:
 var commander:Dictionary=_v343_commander()
 if commander.is_empty() or not bool(commander.get("alive",false)):
  v343_phase=0
  return
 var phase:int=V343_RULES.commander_phase(float(commander.get("hp",0.0)),float(commander.get("max",1.0)))
 if phase==v343_phase:return
 v343_phase=phase
 v343_phase_changes+=1
 var site:int=v341_active_site
 var tag:Label3D=commander.get("nameplate")
 if is_instance_valid(tag):
  tag.text=V342_RULES.commander_title(site)+"  •  "+V343_RULES.phase_title(phase)
  tag.modulate=V343_RULES.phase_color(site,phase)
 if phase>1:
  _show_message(V342_RULES.commander_title(site)+": "+V343_RULES.phase_title(phase)+"! Watch the changing ground marks.",2.6)
 _v342_refresh_status()

func _v341_activate_wave()->void:
 super._v341_activate_wave()
 v343_dodge_chain=0
 if v341_wave==1:
  v343_perfect_dodges=0
  v343_real_hits=0
 v343_phase=0
 if v341_wave==3:_v343_update_phase()

func _v342_refresh_status()->void:
 super._v342_refresh_status()
 if not is_instance_valid(v342_status_label) or not is_instance_valid(v342_status_panel):return
 if not v342_status_panel.visible:return
 if v341_wave!=3:
  v342_status_panel.offset_bottom=219.0
  return
 var boss:Dictionary=_v343_commander()
 if boss.is_empty():return
 var total:float=maxf(1.0,float(boss.get("max",1.0)))
 var pct:int=clampi(ceili(100.0*float(boss.get("hp",0.0))/total),0,100)
 var phase:int=V343_RULES.commander_phase(float(boss.get("hp",0.0)),total)
 var exposed:bool=int(boss.get("v343_exposed_hits",0))>0
 v342_status_label.text+="\n"+V343_RULES.phase_title(phase)+"  •  HP "+str(pct)+"%"+ ("  •  EXPOSED!" if exposed else "")
 v342_status_panel.offset_bottom=260.0

func _v342_begin_strike()->void:
 _v343_update_phase()
 super._v342_begin_strike()
 if v342_phase!=1 or v342_warning_site<0 or not is_instance_valid(player):return
 var site:int=v342_warning_site
 var markers:Array=v342_warnings[site]["markers"]
 var centers:Array[Vector3]=V343_RULES.target_positions(site,player.global_position,v342_pattern,maxi(1,v343_phase))
 var count:int=mini(V343_RULES.marker_count(site,maxi(1,v343_phase)),markers.size())
 for i in range(markers.size()):
  var marker:MeshInstance3D=markers[i]
  marker.visible=i<count
  if i<count:marker.global_position=centers[i]

func _v342_resolve_strike()->void:
 # Detect a genuine hazard dodge BEFORE the inherited strike hides all markers.
 var dodged:bool=false
 if v342_phase==1 and v342_warning_site>=0 and is_instance_valid(player) and dodge_time>0.0:
  var site:int=v342_warning_site
  for node in v342_warnings[site]["markers"]:
   var marker:MeshInstance3D=node
   if marker.visible and V342_RULES.inside_warning(site,player.global_position,marker.global_position):
    dodged=true
    break
 super._v342_resolve_strike()
 if not dodged or v341_wave!=3 or v341_active_site<0:return
 v343_perfect_dodges+=1
 v343_dodge_chain+=1
 if v343_dodge_chain>=V343_RULES.EXPOSE_AFTER_DODGES:
  var boss:Dictionary=_v343_commander()
  if not boss.is_empty() and bool(boss.get("alive",false)):
   boss["v343_exposed_hits"]=1
   v343_exposure_triggers+=1
   v343_dodge_chain=0
   _show_message("LUNAR COUNTER! Commander exposed to your next successful strike.",2.4)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var exposed:bool=bool(target.get("invasion_commander",false)) and int(target.get("v343_exposed_hits",0))>0 and bool(target.get("alive",false))
 var before:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,V343_RULES.exposed_damage(base_damage,exposed),stagger_strength)
 if exposed and float(target.get("hp",0.0))<before:
  target["v343_exposed_hits"]=0
  _show_message("CRESCENT COUNTER! The commander staggers.",1.7)

func _apply_incoming_damage(raw:float)->void:
 var before:float=hp
 super._apply_incoming_damage(raw)
 if v341_wave==3 and v341_active_site>=0 and hero_playing and hp<before:
  v343_real_hits+=1
  v343_dodge_chain=0

func _v341_advance_if_cleared()->void:
 var finishing:bool=v341_active_site>=0 and v341_wave==3
 var clean:bool=finishing and V343_RULES.perfect_seal(v343_perfect_dodges,v343_real_hits)
 super._v341_advance_if_cleared()
 if finishing and v341_active_site<0 and clean:
  # Uses the existing saved Moon Sigil material, never an extra currency.
  _gain_material("Moon Sigil",V343_RULES.CLEAN_SEAL_BONUS)
  v343_clean_seals+=1
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false
  _show_message("FLAWLESS DEFENSE! +1 Moon Sigil for a clean invasion seal.",3.1)

func _v341_reset()->void:
 super._v341_reset()
 v343_phase=0
 v343_dodge_chain=0
 v343_perfect_dodges=0
 v343_real_hits=0

func _process(delta:float)->void:
 super._process(delta)
 # Poll one active commander five times a second, never scan the world.
 if v341_wave==3 and v341_active_site>=0:
  v343_poll_clock+=clampf(delta,0.0,0.20)
  if v343_poll_clock>=0.2:
   v343_poll_clock=0.0
   _v343_update_phase()
 else:v343_poll_clock=0.0

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]LUNAR WAR COMMANDERS[/b]\nEach commander has three HP-driven phases. Attack marks change as it weakens. Two accurate hazard dodges expose the boss for a counterattack. Seal an invasion with two perfect hazard dodges and no actual damage taken during the commander stage to earn +1 Moon Sigil. No building or survival time limit."

func _perf_report()->Dictionary:
 var stats:Dictionary=super._perf_report()
 stats["version"]="v34.3"
 stats["commander_phase"]=v343_phase
 stats["commander_phase_changes"]=v343_phase_changes
 stats["commander_exposures"]=v343_exposure_triggers
 stats["invasion_perfect_dodges"]=v343_perfect_dodges
 stats["clean_invasion_seals"]=v343_clean_seals
 return stats
