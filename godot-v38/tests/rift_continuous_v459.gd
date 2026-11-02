extends SceneTree
## Static integration check for wave-free inherited encounter behavior.
const SCRIPT=preload("res://scripts/native_v38_10.gd")
var failures:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS continuous rift: "+label)
 else:
  failures+=1
  push_error("FAIL continuous rift: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v38_10.gd")
 ck(source.contains('entry["stage"]=3'),"legacy completion stage set once")
 ck(source.contains("super._activate_rift_wave(site_id)"),"existing encounter and reward infrastructure reused")
 ck(source.contains("V321_RULES.enemy_position(entry[\"anchor\"],i+1,i)"),"residents spread across three chambers")
 ck(source.contains('resident["alert_until"]=0.0'),"no forced 90-second pursuit")
 ck(source.contains("func _set_rift_label(entry:Dictionary)"),"legacy wave label overridden")
 ck(not source.contains('label.text="WAVE '),"no wave label added")
 ck(SCRIPT!=null,"live inherited script parses")
 ck(load("res://scenes/main.tscn")!=null,"game scene loads")
 if failures==0:print("MOONFALL CONTINUOUS RIFT PASS")
 quit(1 if failures else 0)
