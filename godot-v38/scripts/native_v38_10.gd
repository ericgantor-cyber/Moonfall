extends "res://scripts/native_v38_9.gd"
## Boss warning clarity. Reuse existing warning geometry and nameplate.
const BOSS_READOUT=preload("res://scripts/systems/boss_telegraph_readout.gd")

func _set_boss_telegraph(beast:Dictionary,active:bool)->void:
 super._set_boss_telegraph(beast,active)
 if not active:return
 var nameplate:Label3D=beast.get("nameplate") as Label3D
 if not is_instance_valid(nameplate):return
 # The scheduler increments boss_pattern after choosing the current attack.
 var phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var pattern_count:int=mini(4,phase+1)
 var current_pattern:int=posmod(int(beast.get("boss_pattern",0))-1,pattern_count)
 var label:String=BOSS_READOUT.warning_text(phase,current_pattern)
 if nameplate.text!=label:nameplate.text=label
