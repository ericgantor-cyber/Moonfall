extends SceneTree
const R=preload("res://scripts/systems/guardian_attack_rules.gd")
var failures:int=0
func check(value:bool,label:String)->void:
 if value:print("PASS guardian: "+label)
 else:failures+=1;push_error("FAIL guardian: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(R.pattern_count(1)==2,"phase one has two attacks")
 check(R.pattern_count(2)==3,"phase two has three attacks")
 check(R.pattern_count(3)==4,"phase three has four attacks")
 check(R.next_pattern(3,3)==0,"phase three wraps")
 check(R.next_pattern_after_phase_change(1,2)==2,"phase two introduces precision after sweep")
 check(R.next_pattern_after_phase_change(2,3)==3,"phase three introduces delayed strike after precision")
 check(R.next_pattern_after_phase_change(3,3)==0,"phase transition selection wraps safely")
 check(R.unlocked_patterns(1)==[0,1],"phase one starts with focused and sweep patterns")
 check(R.unlocked_patterns(2)==[0,1,2],"phase two unlocks precision pattern")
 check(R.unlocked_patterns(3)==[0,1,2,3],"phase three unlocks delayed strike")
 check(not R.pattern_unlocked(3,1) and R.pattern_unlocked(3,3),"advanced attacks stay phase-gated")
 check(R.pattern_index(-1,3)==3,"warning reports last selected attack")
 check(R.radius(1)>R.radius(2),"wide sweep larger than precision")
 check(R.windup(3,1.25)>=0.75,"late boss phases retain dodge window")
 check(load("res://scenes/main.tscn")!=null,"whole Moonfall scene loads")
 quit(1 if failures else 0)
