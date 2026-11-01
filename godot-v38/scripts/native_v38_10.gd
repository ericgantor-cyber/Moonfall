extends "res://scripts/native_v38_9.gd"
## Distinct, cached boss warning labels avoid overwriting health/nameplates.
const BOSS_READOUT=preload("res://scripts/systems/boss_telegraph_readout.gd")
const GUARDIAN_RULES=preload("res://scripts/systems/guardian_attack_rules.gd")
var boss_warning_labels:Dictionary={}
const MAX_BOSS_WARNING_LABELS:int=24
var boss_warning_updates:int=0

func _prune_boss_warning_labels()->void:
 for key in boss_warning_labels.keys():
  var label:Variant=boss_warning_labels.get(key)
  if not is_instance_valid(label) or not is_instance_valid(label.get_parent()):
   boss_warning_labels.erase(key)


func _set_boss_telegraph(beast:Dictionary,active:bool)->void:
 super._set_boss_telegraph(beast,active)
 var id:int=int(beast.get("boss_id",-1))
 if id<0:return
 if boss_warning_labels.size()>=MAX_BOSS_WARNING_LABELS:_prune_boss_warning_labels()
 var warning:Label3D=boss_warning_labels.get(id) as Label3D
 if not active:
  if is_instance_valid(warning):warning.visible=false
  elif boss_warning_labels.has(id):boss_warning_labels.erase(id)
  return
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 if not is_instance_valid(body):
  if is_instance_valid(warning):warning.visible=false
  return
 if is_instance_valid(warning) and warning.get_parent()!=body:
  warning.queue_free()
  boss_warning_labels.erase(id)
  warning=null
 if not is_instance_valid(warning):
  if boss_warning_labels.size()>=MAX_BOSS_WARNING_LABELS:return
  warning=Label3D.new()
  warning.name="GuardianAttackWarning"
  warning.font_size=35
  warning.pixel_size=0.004
  warning.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  warning.modulate=Color(1.0,0.75,0.32)
  warning.position=Vector3(0,2.3,0)
  warning.no_depth_test=true
  body.add_child(warning)
  boss_warning_labels[id]=warning
 var phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var current_pattern:int=GUARDIAN_RULES.pattern_index(int(beast.get("boss_pattern",0))-1,phase)
 var new_text:String=BOSS_READOUT.warning_text(phase,current_pattern)
 if warning.text!=new_text:
  warning.text=new_text
  boss_warning_updates+=1
 warning.visible=true
