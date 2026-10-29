extends "res://scripts/native_v33_0.gd"
## v33.1 combat stability: correct bounded candidate inspection in the inherited AI.
func _release_label()->String:return "v33.1"
func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v33.1"
 return report
