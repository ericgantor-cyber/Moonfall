extends "res://scripts/native_v32_3.gd"
## v32.4: champion windup cues, using the combat system's real windup timer.
## No new AI scheduler, save fields, combat math, collision or hero art.
const V324_CUE_INTERVAL:float=0.08
var v324_cues:Array[Dictionary]=[]
var v324_clock:float=0.0
var v324_warning_starts:int=0
var v324_cue_updates:int=0

func _release_label()->String:return "v32.4"

func _activate_rift_wave(site_id:int)->void:
 super._activate_rift_wave(site_id)
 if site_id<0 or site_id>=rifts.size():return
 var entry:Dictionary=rifts[site_id]
 if int(entry.get("stage",0))!=3:return
 for slot in (entry.get("slots",[]) as Array):
  var index:int=int(slot)
  if index<0 or index>=beasts.size():continue
  var creature:Dictionary=beasts[index]
  if not bool(creature.get("dungeon_champion",false)):continue
  var actor:CharacterBody3D=creature.get("body")
  if not is_instance_valid(actor):continue
  # Each champion gets one reused mesh, attached to the creature.
  var marker:MeshInstance3D=actor.get_node_or_null("V324_ChampionWindup") as MeshInstance3D
  if marker==null:
   marker=MeshInstance3D.new()
   marker.name="V324_ChampionWindup"
   var mesh:=CylinderMesh.new()
   mesh.top_radius=1.65
   mesh.bottom_radius=1.65
   mesh.height=0.025
   mesh.radial_segments=24
   marker.mesh=mesh
   var material:=StandardMaterial3D.new()
   material.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
   material.albedo_color=Color(1.0,0.17,0.12,0.48)
   material.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
   material.emission_enabled=true
   material.emission=Color("ff4028")
   marker.material_override=material
   marker.position.y=0.11
   marker.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   marker.visible=false
   actor.add_child(marker)
  var known:bool=false
  for cue in v324_cues:
   if cue["marker"]==marker:
    cue["creature"]=creature
    cue["active"]=false
    known=true
    break
  if not known:v324_cues.append({"creature":creature,"marker":marker,"active":false})

func _process(delta:float)->void:
 super._process(delta)
 v324_clock+=clampf(delta,0.0,0.2)
 if v324_clock<V324_CUE_INTERVAL:return
 v324_clock=fmod(v324_clock,V324_CUE_INTERVAL)
 for cue in v324_cues:
  var creature:Dictionary=cue["creature"]
  var marker:MeshInstance3D=cue["marker"]
  if not is_instance_valid(marker):continue
  var actor:CharacterBody3D=creature.get("body")
  var warning:bool=hero_playing and bool(creature.get("alive",false)) and is_instance_valid(actor) and actor.visible and float(creature.get("windup",0.0))>0.0
  if warning and not bool(cue["active"]):v324_warning_starts+=1
  cue["active"]=warning
  marker.visible=warning
  if warning:
   var phase:float=map_time*9.0
   var pulse:float=1.0+0.13*sin(phase)
   marker.scale=Vector3(pulse,1.0,pulse)
   v324_cue_updates+=1

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v324_clock=0.0
 for cue in v324_cues:
  var marker:MeshInstance3D=cue["marker"]
  if is_instance_valid(marker):marker.visible=false
  cue["active"]=false

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.4"
 report["champion_attack_warning_cues"]=v324_cues.size()
 report["champion_warning_transitions"]=v324_warning_starts
 report["champion_warning_pulse_updates"]=v324_cue_updates
 return report
