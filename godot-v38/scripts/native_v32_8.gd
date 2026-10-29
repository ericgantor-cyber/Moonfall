extends "res://scripts/native_v32_7.gd"
## v32.8: additional boss patterns across phases, using the existing combat engine.
## See native_v11.gd for the bounded four-pattern cycle.
func _release_label()->String:return "v32.8"
func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.8"
 report["boss_pattern_slots_phase_one"]=2
 report["boss_pattern_slots_phase_two"]=3
 report["boss_pattern_slots_phase_three"]=4
 return report
