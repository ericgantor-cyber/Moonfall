extends "res://scripts/native_v38_9.gd"
## Distinct, cached boss warning labels avoid overwriting health/nameplates.
const BOSS_READOUT=preload("res://scripts/systems/boss_telegraph_readout.gd")
var boss_warning_labels:Dictionary={}

func _set_boss_telegraph(beast:Dictionary,active:bool)->void:
 super._set_boss_telegraph(beast,active)
 var id:int=int(beast.get("boss_id",-1))
 if id<0:return
 var warning:Label3D=boss_warning_labels.get(id) as Label3D
 if not active:
  if is_instance_valid(warning):warning.visible=false
  return
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 if not is_instance_valid(body):return
 if is_instance_valid(warning) and warning.get_parent()!=body:
  warning.queue_free()
  warning=null
 if not is_instance_valid(warning):
  warning=Label3D.new()
  warning.name="GuardianAttackWarning"
  warning.font_size=35
  warning.pixel_size=0.004
  warning.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  warning.modulate=Color(1.0,0.75,0.32)
  warning.position=Vector3(0,2.3,0)
  body.add_child(warning)
  boss_warning_labels[id]=warning
 var phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var pattern_count:int=mini(4,phase+1)
 var current_pattern:int=posmod(int(beast.get("boss_pattern",0))-1,pattern_count)
 var new_text:String=BOSS_READOUT.warning_text(phase,current_pattern)
 if warning.text!=new_text:warning.text=new_text
 warning.visible=true
