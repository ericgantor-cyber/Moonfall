extends "res://scripts/native_v32_6.gd"
## v32.7: readable, bounded boss telegraph pulses using existing warning meshes.
## Cosmetic only: no combat scheduling, damage, navigation or save-schema changes.
const V327_WARNING_PULSE_RATE:float=11.0
var v327_active_warning_ticks:int=0
var v327_warning_peak:int=0

func _release_label()->String:return "v32.7"

func _v320_stream_update()->void:
 super._v320_stream_update()
 var active_count:int=0
 for cue in v320_boss_cues:
  var marker:MeshInstance3D=cue.get("node")
  if not is_instance_valid(marker):continue
  if not marker.visible:
   marker.scale=Vector3.ONE
   continue
  # The parent owns the visibility check and real combat windup timer.
  # Assign absolute scale to prevent frame-over-frame size accumulation.
  var emphasis:float=1.0+0.16*sin(map_time*V327_WARNING_PULSE_RATE)
  marker.scale=Vector3(emphasis,1.0,emphasis)
  active_count+=1
 if active_count>0:v327_active_warning_ticks+=1
 v327_warning_peak=maxi(v327_warning_peak,active_count)

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 for cue in v320_boss_cues:
  var marker:MeshInstance3D=cue.get("node")
  if is_instance_valid(marker):marker.scale=Vector3.ONE
 v327_active_warning_ticks=0
 v327_warning_peak=0

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.7"
 report["boss_warning_pulse_ticks"]=v327_active_warning_ticks
 report["peak_simultaneous_boss_warnings"]=v327_warning_peak
 return report
