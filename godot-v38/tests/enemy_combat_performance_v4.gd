extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var failures:int=0
func check(ok:bool,description:String)->void:
 if ok:print("PASS combat v4: "+description)
 else:
  failures+=1
  push_error("FAIL combat v4: "+description)
func _initialize()->void:call_deferred("run")
func run()->void:
 check(R.attack_timer_after_frame(-5.0,0.01)==0.0,"negative saved timer reset")
 check(R.attack_timer_after_frame(10000.0,0.0)==30.0,"corrupt huge timer capped")
 check(R.attack_timer_after_frame(NAN,0.0)==0.0,"NaN timer reset")
 check(R.attack_timer_after_frame(1.0,NAN)==1.0,"NaN delta does not corrupt timer")
 check(not R.ready_to_strike(NAN) and not R.ready_to_strike(INF),"invalid timers cannot authorize attacks")
 check(R.ready_to_strike(0.0) and not R.ready_to_strike(0.2),"attack readiness respects cooldown")
 check(not R.should_circle("wolf","strafe",NAN),"NaN distance cannot trigger circling")
 check(not R.should_circle("serpent","strafe",INF),"infinite distance cannot trigger circling")
 check(R.should_circle("wolf","strafe",3.0),"normal wolf circling preserved")
 check(load("res://scenes/main.tscn")!=null,"full game scene loads")
 quit(1 if failures else 0)
