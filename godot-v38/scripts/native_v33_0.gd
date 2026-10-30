extends "res://scripts/native_v32_9.gd"
## v33.0 stability milestone. Existing gameplay and visual assets preserved.
func _release_label()->String:return "v33.0"
func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v33.0"
 report["flank_candidate_cap"]=V329_MAX_CANDIDATES
 return report
