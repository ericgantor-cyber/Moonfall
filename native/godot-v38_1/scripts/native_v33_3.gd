extends "res://scripts/native_v33_2.gd"
## v33.3 installer metadata integrity; gameplay unchanged.
func _release_label()->String:return "v33.3"
func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v33.3"
 return report
