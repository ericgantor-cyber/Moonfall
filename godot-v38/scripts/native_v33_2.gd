extends "res://scripts/native_v33_1.gd"
## v33.2 release validation milestone; no gameplay balance changes.
func _release_label()->String:return "v33.2"
func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v33.2"
 return report
